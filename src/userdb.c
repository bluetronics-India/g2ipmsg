/*
 *  Copyright (C) 2006 Takeharu KATO
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "common.h"

static GList *users=NULL;
static GList *waiter_windows=NULL;
GStaticMutex userdb_mutex = G_STATIC_MUTEX_INIT;
GStaticMutex win_mutex = G_STATIC_MUTEX_INIT;

static gint 
userdb_find_by_ipaddr(gconstpointer a,gconstpointer b) {
  int rc=-EINVAL;
  userdb_t *user_a,*user_b;

  if ( (!a) || (!b) ) 
    goto out;

  user_a=(userdb_t *)a;
  user_b=(userdb_t *)b;

  if (!strcmp(user_a->ipaddr,user_b->ipaddr)) 
    return 0;

  rc=-ESRCH;
 out:

  return rc; /* Not found */
}

static void
print_one_user_entry(gpointer data,gpointer user_data) {
  userdb_t *current_user;
  unsigned long skey,akey,sign;
  if (!data)
    return;

  current_user=(userdb_t *)data;
  dbg_out("NickName: %s\n",current_user->nickname);
  dbg_out("Group: %s\n",current_user->group);
  dbg_out("User: %s\n",current_user->user);
  dbg_out("Host: %s\n",current_user->host);
  dbg_out("IP Addr: %s (%s) \n",current_user->ipaddr, (current_user->pf == PF_INET) ? "IPV4" : "IPV6");
  dbg_out("Capability: %x\n",(unsigned int)current_user->cap);
  if  (current_user->cap & IPMSG_DIALUPOPT) 
    dbg_out("\t Dialup host\n");
  if  (current_user->cap & IPMSG_FILEATTACHOPT) 
    dbg_out("\t File Attach\n");
  if  (current_user->cap & IPMSG_ENCRYPTOPT) 
    dbg_out("\t Encryption\n");
  if (current_user->cap & IPMSG_UTF8OPT)
    dbg_out("\t UTF-8\n");

  if  (current_user->cap & IPMSG_ENCRYPTOPT) {
#if defined(USE_OPENSSL)
    dbg_out("\t Crypt capability\n");
    if (current_user->crypt_cap) {
      if (current_user->crypt_cap) {
	skey=get_symkey_part(current_user->crypt_cap);
	if (skey & IPMSG_RC2_40)
	  dbg_out("\t RC2 40 bits\n");
	if (skey & IPMSG_RC2_128)
	  dbg_out("\t RC2 128 bits\n");
	if (skey & IPMSG_RC2_256)
	  dbg_out("\t RC2 256 bits\n");
	if (skey & IPMSG_BLOWFISH_128)
	  dbg_out("\t Blowfish 128 bits\n");
	if (skey & IPMSG_BLOWFISH_256)
	  dbg_out("\t Blowfish 256 bits\n");
	if (skey & IPMSG_AES_128)
	  dbg_out("\t AES 128 bits\n");
	if (skey & IPMSG_AES_192)
	  dbg_out("\t AES 192 bits\n");
	if (skey & IPMSG_AES_256)
	  dbg_out("\t AES 256 bits\n");

	akey=get_asymkey_part(current_user->crypt_cap);
	if (akey & IPMSG_RSA_512)
	  dbg_out("\t RSA 512 bits\n");
	if (akey & IPMSG_RSA_1024)
	  dbg_out("\t RSA 1024 bits\n");
	if (akey & IPMSG_RSA_2048)
	  dbg_out("\t RSA 2048 bits\n");
	
	g_assert(current_user->pub_key_e);
	g_assert(current_user->pub_key_n);
	dbg_out("\tPublic key:\n");
	dbg_out("\te=%s\n",current_user->pub_key_e);
	dbg_out("\tn=%s\n",current_user->pub_key_n);

	sign=get_sign_part(current_user->crypt_cap);
	if (sign) {
	  if (sign&IPMSG_SIGN_MD5)
	    dbg_out("\t handle MD5 sign\n");
	  if (sign&IPMSG_SIGN_SHA1)
	    dbg_out("\t handle SHA1 sign\n");
	}else{
	    dbg_out("\t No sign\n");
	}
      }
    }else{
      dbg_out("\t\t Unknown\n");
    }
#else
      dbg_out("\t\t Unknown\n");
#endif
  }
  return;
}

static int
alloc_user_info(userdb_t **entry){
  userdb_t *new_user;

  if (!entry)
    return -EINVAL;

  new_user=g_slice_new(userdb_t);
  if (!new_user)
    return -ENOMEM;

  memset(new_user,0,sizeof(userdb_t));

  *entry=new_user;

  return 0;

}

static int
destroy_user_info_contents(userdb_t *entry){

  if (!entry)
    return -EINVAL;

  if (entry->user)
    g_free(entry->user);
  if (entry->host)
    g_free(entry->host);
  if (entry->group)
    g_free(entry->group);
  if (entry->nickname)
    g_free(entry->nickname);
  if (entry->ipaddr) 
    g_free(entry->ipaddr);
  if (entry->pub_key_e)
    g_free(entry->pub_key_e);
  if (entry->pub_key_n)
    g_free(entry->pub_key_n);

  memset(entry,0,sizeof(userdb_t));

  return 0;
}
static void
do_notify_change(gpointer data,gpointer user_data) {
  GtkWidget *window;

  if (!data)
    return;

  window=GTK_WIDGET(data);
  update_users_on_message_window(window,FALSE);

  return;
}
static int
notify_userdb_changed(void){

  g_static_mutex_lock(&win_mutex);
  g_list_foreach(waiter_windows,
		 do_notify_change,
		 NULL);  
  g_static_mutex_unlock(&win_mutex);

}

#define strdup_with_check(dest,src,member,err_label)	\
  ((dest)->member)=g_strdup((src)->member);		\
  if (!((dest)->member))				\
    goto err_label ;                                    
  
static int
copy_user_info(userdb_t *dest,userdb_t *src) {
  int rc=-ENOMEM;

  if ( (!dest) || (!src) )
    return -EINVAL;

  g_assert(!(dest->user));
  g_assert(!(dest->host));
  g_assert(!(dest->group));
  g_assert(!(dest->nickname));
  g_assert(!(dest->ipaddr));
  g_assert(!(dest->pub_key_e));
  g_assert(!(dest->pub_key_n));

  dest->prio=src->prio;
  dest->cap=src->cap;
  dest->crypt_cap=src->crypt_cap;
  dest->pf = src->pf;

  /*  下記はマクロであることに注意  */
  strdup_with_check(dest,src,user,error_out)
  strdup_with_check(dest,src,host,error_out)
  strdup_with_check(dest,src,group,error_out)
  strdup_with_check(dest,src,nickname,error_out)
  strdup_with_check(dest,src,ipaddr,error_out)
    if (src->pub_key_e) {
      strdup_with_check(dest,src,pub_key_e,error_out)
      }
  if (src->pub_key_n) {
    strdup_with_check(dest,src,pub_key_n,error_out)
      }

  return 0;

 error_out:
  destroy_user_info_contents(dest);
  return rc;
}

static int
fill_user_info_with_message(const udp_con_t *con, const msg_data_t *msg, userdb_t *new_user){
  int rc;
  int default_prio;
  const char *peer_addr;

  if ( (!new_user) || (!msg) )
    return -EINVAL;

  memset(new_user,0,sizeof(userdb_t));

  peer_addr = udp_get_peeraddr(con);

  rc=-ENOMEM;
  convert_string_internal(refer_user_name_from_msg(msg),(const gchar **)&(new_user->user));
  if (new_user->user == NULL) {
    ipmsg_err_dialog(_("Can not convert message from %s into ineternal representation"), peer_addr);
    goto memclear_out;
  }
  convert_string_internal(refer_host_name_from_msg(msg),(const gchar **)&(new_user->host));
  if (new_user->host == NULL) {
    ipmsg_err_dialog(_("Can not convert message from %s into ineternal representation"), peer_addr);
    goto free_user_out;
  }

  convert_string_internal(refer_group_name_from_msg(msg),(const gchar **)&(new_user->group));
  if (new_user->group == NULL) {
    ipmsg_err_dialog(_("Can not convert message from %s into ineternal representation"), peer_addr);
    goto free_host_out;
  }

  convert_string_internal(refer_nick_name_from_msg(msg),(const gchar **)&(new_user->nickname));
  if (new_user->nickname == NULL) {
    ipmsg_err_dialog(_("Can not convert message from %s into ineternal representation"), peer_addr);
    goto free_group_out;
  }

  convert_string_internal(peer_addr,(const gchar **)&(new_user->ipaddr));
  if (new_user->ipaddr == NULL) {
    ipmsg_err_dialog(_("Can not convert message from %s into ineternal representation"), peer_addr);
    goto free_nickname_out;
  }

  rc=hostinfo_get_ipmsg_ipaddr_prio(new_user->ipaddr,&default_prio);
  if (rc<0)
    default_prio=0;

  new_user->cap=msg->command_opts;
  new_user->prio=default_prio;
  new_user->pf = con->family;

  dbg_out("Fill:  %s %s %s %s %d %x\n",
	  new_user->user,
	  new_user->host,
	  new_user->group,
	  new_user->ipaddr,
	  new_user->prio,
	  (unsigned int)new_user->cap);

  return 0;

 free_ipaddr_out:
  g_free(new_user->ipaddr);
 free_nickname_out:
  g_free(new_user->nickname);
 free_group_out:
  g_free(new_user->group);
 free_host_out:
  g_free(new_user->host);
 free_user_out:
  g_free(new_user->user);
 memclear_out:
  memset(new_user,0,sizeof(userdb_t));
  return rc;
}

static int
add_with_userdb_entry(userdb_t *new_user){
  int rc;
  GList *new_entry;
  GList *current_entry=NULL;

  if (!new_user)
    return -EINVAL;

  dbg_out("Add: %s %s %s %s %x\n",
	  new_user->user,
	  new_user->host,
	  new_user->group,
	  new_user->ipaddr,
	  (unsigned int)new_user->cap);

  g_static_mutex_lock(&userdb_mutex);
  current_entry=g_list_find_custom (users,new_user,userdb_find_by_ipaddr);
  if (!current_entry){
    users=g_list_append(users,new_user);
  } else {
    rc=-EEXIST;
    goto error_out;
  }
  /*
   * 健全性チェック
   */
  new_entry=g_list_find_custom(users,new_user,userdb_find_by_ipaddr);
  g_assert(new_entry);
  g_assert(new_entry->data == new_user);
  g_static_mutex_unlock(&userdb_mutex);
  notify_userdb_changed();

  return 0;
 error_out:
  g_static_mutex_unlock(&userdb_mutex);
  destroy_user_info(new_user);
  return rc;
}

/*
 *start:length:ユーザ名:ホスト名:コマンド番号:IP アドレス:
 *ポート番号（リトルエンディアン）:ニックネーム:グループ名
 */
static int
userdb_add_hostlist_entry(const udp_con_t *con, userdb_t *entry,const char **entry_string_p, int *is_last){
  int rc;
  char *buffer=NULL;
  char *sp,*ep;
  long remains;
  int intval;
  size_t len;
  char *user=NULL;
  char *host=NULL;
  char *ipaddr=NULL;
  int port;
  char *nickname=NULL;
  char *group=NULL;
  const char *entry_string;
  if ( (!entry) || (!entry_string_p) || (!is_last) ) 
    return -EINVAL;


  entry_string=*entry_string_p;
  rc=-ENOMEM;
  buffer=g_strdup(entry_string);
  if (!buffer)
    goto free_buffer_out;

  rc=-ENOENT;
  *is_last=1;
  len=strlen(buffer);
  remains=len;
  dbg_out("Buffer:%s\n",buffer);
  /*
   * username
   */
  sp=buffer;
  ep=memchr(sp, HOSTLIST_SEPARATOR, remains);
  if (!ep)
    goto free_buffer_out;
  remains =len - ((unsigned long)ep-(unsigned long)sp);
  if (remains<0)
    goto free_buffer_out;
  *ep='\0';
  user=g_strdup(make_entry_canonical(sp));
  if (!user)
    goto free_buffer_out;
  ++ep;
  sp=ep;
  dbg_out("user:%s\n",user);

  /*
   * host
   */
  ep=memchr(sp, HOSTLIST_SEPARATOR, remains);
  if (!ep)
    goto free_user_out;
  remains =len - ((unsigned long)ep-(unsigned long)sp);
  if (remains<0)
    goto free_user_out;
  *ep='\0';
  host=g_strdup(make_entry_canonical(sp));
  if (!host)
    goto free_user_out;
  ++ep;
  sp=ep;
  dbg_out("host:%s\n",host);

  /*
   * skip command
   */
  ep=memchr(sp, HOSTLIST_SEPARATOR , remains);
  if (!ep)
    goto free_host_out;
  remains =len - ((unsigned long)ep-(unsigned long)sp);
  if (remains<0)
    goto free_host_out;
  *ep='\0';
  ++ep;
  sp=ep;

  /*
   * ipaddr
   */
  ep=memchr(sp, HOSTLIST_SEPARATOR, remains);
  if (!ep)
    goto free_host_out;
  remains =len - ((unsigned long)ep-(unsigned long)sp);
  if (remains<0)
    goto free_host_out;
  *ep='\0';
  ipaddr=g_strdup(make_entry_canonical(sp));
  if (!ipaddr)
    goto free_host_out;
  ++ep;
  sp=ep;
  dbg_out("ipaddr:%s\n",ipaddr);

  /*
   * port
   */
  ep=memchr(sp, HOSTLIST_SEPARATOR , remains);
  if (!ep)
    goto free_ipaddr_out;
  remains =len - ((unsigned long)ep-(unsigned long)sp);
  if (remains<0)
    goto free_ipaddr_out;
  *ep='\0';
  intval=strtol(sp, (char **)NULL, 10);
  ++ep;
  sp=ep;
  dbg_out("port:%d\n",intval);

  /*
   * nick name
   */
  ep=memchr(sp, HOSTLIST_SEPARATOR , remains);
  if (!ep)
    goto free_ipaddr_out;
  remains =len - ((unsigned long)ep-(unsigned long)sp);
  if (remains<0)
    goto free_ipaddr_out;
  *ep='\0';
  nickname=g_strdup(make_entry_canonical(sp));
  if (!nickname)
    goto free_ipaddr_out;
  ++ep;
  sp=ep;
  dbg_out("nickname:%s\n",nickname);

  /*
   * group
   */

  ep=memchr(sp, HOSTLIST_SEPARATOR , remains);
  if (!ep) {
    ep=memchr(sp, '\0' , remains);
    if (!ep)
      goto free_nickname_out;
  }else{
    *is_last=0;
  }
  remains =len - ((unsigned long)ep-(unsigned long)sp);
  if (remains<0)
    goto free_nickname_out;
  *ep='\0';
  group=g_strdup(make_entry_canonical(sp));
  if (!nickname)
    goto free_nickname_out;
  ++ep;
  dbg_out("group:%s\n",group);

  entry->user=user;
  entry->host=host;
  entry->group=group;
  entry->nickname=nickname;
  entry->ipaddr=ipaddr;
  entry->cap=IPMSG_DIALUPOPT; /*  ダイアルアップとみなす  */
  entry->pf = con->family;

  dbg_out("OK entry:\nuser : %s\nhost :%s\ngroup : %s\nnickname : %s\nipaddr : %s\n",  
	  entry->user=user,
	  entry->host=host,
	  entry->group=group,
	  entry->nickname=nickname,
	  entry->ipaddr=ipaddr);

  rc=add_with_userdb_entry(entry);
  if (rc == 0) {
    /* 鍵情報獲得要求を発行する  */
    ipmsg_send_getpubkey(con, ipaddr);
  }

  *entry_string_p += (ep - buffer);
  dbg_out("Return: %s (%d)\n",*entry_string_p,rc);
  if (buffer)
    g_free(buffer);
  return rc;

 free_group_out:
  if (group)
    g_free(group);
 free_nickname_out:
  if (nickname)
    g_free(nickname);
 free_ipaddr_out:
  if (ipaddr)
    g_free(ipaddr);
 free_host_out:
  if (host)
    g_free(host);
 free_user_out:
  if (user)
    g_free(user);
 free_buffer_out:
  if (buffer)
    g_free(buffer);
  return rc;
}

static int
hostlist_userinfo_add_with_answer(const udp_con_t *con, const char *message,int *next_count){
  int rc;
  int start,end;
  char *string;
  char *sp,*ep;
  long remains;
  int intval;
  size_t len;
  userdb_t *entry=NULL;
  size_t this_count;
  size_t remain_count;
  int is_last=0;

  if ( (!message) || (!next_count) ) 
    return -EINVAL;

  string=g_strdup(message);
  if (!string)
    return -ENOMEM;
  
  rc=-ENOENT;
  len=strlen(message);
  remains=len;
  this_count=0;
  *next_count=0;

  /*
   * start
   */
  sp=string;
  ep=memchr(sp, HOSTLIST_SEPARATOR, remains);
  if (!ep) 
    goto free_string_out;
  
  remains =len - ((unsigned long)ep-(unsigned long)sp);
  *ep='\0';
  start=strtol(sp, (char **)NULL, 10);
  if (start<0)
    goto free_string_out;
  ++ep;
  dbg_out("Start:%d\n",start);

  /*
   * total
   */
  sp=ep;
  ep=memchr(sp, HOSTLIST_SEPARATOR, remains);
  if (!ep)
    goto free_string_out;
  remains =len - ((unsigned long)ep-(unsigned long)sp);
  *ep='\0';
  end=strtol(sp, (char **)NULL, 10);
  if (end<0)
    goto free_string_out;
  ++ep;
  dbg_out("End:%d\n",end);

  if (end<start) 
    goto free_string_out;

  sp=ep;
  remains =len - ((unsigned long)ep-(unsigned long)sp);
  is_last=0;
  while ( (sp) && (!is_last) && (remains>0) ){
    dbg_out("Add %d th entry:%s\n",this_count+1,sp);
    rc=alloc_user_info(&entry);
    if (rc)
      goto free_string_out;
    rc=userdb_add_hostlist_entry(con, entry,(const char **)&sp,&is_last);
    if ( (rc) && (rc != -EEXIST) )
      goto free_string_out;  /*  エントリ不正時は追加処理側で開放する  */
    ++this_count;
    remains =len - ((unsigned long)ep-(unsigned long)sp);
  }

 free_string_out:
  g_free(string);


  if ( (this_count == 0) || (end < (this_count + start)) )
    *next_count=0;
  else
    *next_count=(start + this_count);
  dbg_out("Next count:%d (this,start,total)=(%d,%d,%d)\n",
	  *next_count,
	  this_count,
	  start,
	  end);

  return rc;
}
static int 
internal_refer_user_by_addr(const char *ipaddr,const userdb_t **entry_ref){
  int rc=-ESRCH;
  userdb_t srch_user;
  GList *found;

  if ( (!ipaddr) || (!entry_ref) )
    return -EINVAL;

  memset(&srch_user,0,sizeof(userdb_t));

  srch_user.ipaddr=(char *)ipaddr;

  found=g_list_find_custom (users,&srch_user,userdb_find_by_ipaddr);
  if (found) {
    g_assert(found->data);
    rc=0;
    *entry_ref=found->data;
  }

  return rc;
}
int
destroy_user_info(userdb_t *entry){
  int rc;

  if (!entry)
    return -EINVAL;

  rc=destroy_user_info_contents(entry);
  if (rc)
    return rc;

  memset(entry,0,sizeof(userdb_t));
  dbg_out("Free: %x\n",(unsigned int)entry);
  g_slice_free(userdb_t,entry);

  return 0;
}
int
userdb_hostlist_answer_add(const udp_con_t *con,const msg_data_t *msg){
  int next_count=0;
  int rc;
  gchar *internal_string=NULL;

  if ( (!con) || (!msg) )
    return -EINVAL;
  if (!(msg->message))
    return -EINVAL;

  dbg_out("Here\n");
  rc=convert_string_internal(msg->message,(const gchar **)&internal_string);
  if (rc < 0) {
    ipmsg_err_dialog(_("Can not convert message from %s into ineternal representation"), udp_get_peeraddr(con));
    return rc;
  }

  next_count=0;
  rc=hostlist_userinfo_add_with_answer(con, internal_string, &next_count);
  if ( (rc<0) && (rc != -EEXIST) ) 
    goto  free_string_out;

  if (next_count) {
    dbg_out("Send next:%d\n",next_count);
    rc=ipmsg_send_get_list(udp_con,udp_get_peeraddr(con),next_count);
  }
  if (rc<0)
    goto  free_string_out;

 free_string_out:
  if (internal_string)
    g_free(internal_string);
 error_out:
  return rc;
}
static gint 
userdb_sort_with_view_config(gconstpointer data_a,
			     gconstpointer data_b,
			     gpointer user_data){
  userdb_t *user_a=(userdb_t *)data_a;
  userdb_t *user_b=(userdb_t *)data_b;  
  gint rc;

  g_assert(user_a);
  g_assert(user_b);
 
  if (user_b->prio != user_a->prio)
    return (user_b->prio - user_a->prio);

  if ( (hostinfo_refer_ipmsg_is_sort_with_group()) &&
       ( (user_a->group) && (user_b->group) ) ) {
    rc=strcmp(user_a->group,user_b->group);
    if (rc) {
      if (hostinfo_refer_ipmsg_group_sort_order())
	return rc;
      else
	return -rc;
    }
  }
  rc=0;
  switch(hostinfo_refer_ipmsg_sub_sort_id()) {
  default:
    case SORT_TYPE_USER:
      if ((user_a->user) && (user_b->user))
	rc=strcmp(user_a->user,user_b->user);
      break;
    case SORT_TYPE_IPADDR:
      if ((user_a->ipaddr) && (user_b->ipaddr))
	rc=strcmp(user_a->ipaddr,user_b->ipaddr);
      break;
    case SORT_TYPE_MACHINE:
      if ((user_a->host) && (user_b->host))
	rc=strcmp(user_a->host,user_b->host);
      break;
  }

  if (!hostinfo_refer_ipmsg_sub_sort_order())
    return -rc;

  return rc;
}
int
update_users_on_message_window(GtkWidget *window,gboolean is_force){
  GtkWidget *view;
  GList *current_users;

  g_assert(window);

  view=lookup_widget(window,"messageUserTree");
  g_assert(view);
  dbg_out("Notify userdb change :%x\n",(unsigned int)window);
  g_static_mutex_lock(&userdb_mutex);
  current_users=g_list_copy(refer_user_list());
  if (current_users) {
    current_users=g_list_sort(current_users,(GCompareFunc)userdb_sort_with_view_config);
    update_user_entry(current_users,view,is_force);
    g_list_free(current_users);    
  }else{
    current_users=refer_user_list();
    update_user_entry(current_users,view,is_force);
  }
  g_static_mutex_unlock(&userdb_mutex);  

}

int
userdb_search_user_by_addr(const char *ipaddr,const userdb_t **entry_ref){
  int rc=-ESRCH;
  userdb_t srch_user;
  userdb_t *user_p=NULL;
    userdb_t *new_user;

  if ( (!ipaddr) || (!entry_ref) )
    return -EINVAL;

  memset(&srch_user,0,sizeof(userdb_t));
  srch_user.ipaddr=(char *)ipaddr;

  g_static_mutex_lock(&userdb_mutex);  

  rc=alloc_user_info(&new_user);
  if (rc<0) 
    goto unlock_out;

  rc=internal_refer_user_by_addr(ipaddr,(const userdb_t **)&user_p);
  if (rc<0){
    destroy_user_info(new_user);
    goto unlock_out;
  }
  rc=copy_user_info(new_user,user_p);
  if (rc<0) {
    destroy_user_info(new_user);
    goto unlock_out;
  }
  rc=0;
  *entry_ref=new_user;

 unlock_out:
  g_static_mutex_unlock(&userdb_mutex);

  return rc;
}
void
userdb_print_user_list(void){
  g_list_foreach(users,
		 print_one_user_entry,
		 NULL);  
}
static gint 
userdb_find_group(gconstpointer a,gconstpointer b) {
  int rc=-EINVAL;

  if ( (!a) || (!b) ) 
    return -EINVAL;


  dbg_out("Pointer:0x%x 0x%x\n",a,b);
  return strcmp((char *)a,(char *)b);
}
GList *
get_group_list(void){
  GList *ret=NULL;
  GList *found;
  GList *node;
  userdb_t *ref;
  gchar *string;

  g_static_mutex_lock(&userdb_mutex);
  for(node=g_list_first(users);node;node=g_list_next(node)) {
    ref=node->data;
    
    found=g_list_find_custom (ret,ref->group,userdb_find_group);
    if (found)
      continue;
    string=g_strdup(ref->group);
    if (string)
      ret=g_list_append(ret,string);
  }
  g_static_mutex_unlock(&userdb_mutex);  

  return ret;
}
void
userdb_update_group_list(GtkComboBox *widget){
  if (!widget)
    return;

  g_list_foreach(users,
                 (GFunc)update_one_group_entry,
                 widget);
}
GList *
refer_user_list(void) {
  return users;
}
int 
userdb_add_user(const udp_con_t *con,const msg_data_t *msg){
  int rc=0;
  userdb_t *new_user;

  if ( (!con) || (!msg) )
    return -EINVAL;

  if (alloc_user_info(&new_user)) 
    return -ENOMEM;

  rc=fill_user_info_with_message(con, msg, new_user);
  if (rc<0) {
    destroy_user_info(new_user);
    return rc;
  }
  rc=add_with_userdb_entry(new_user);

  return rc;
}
int 
userdb_update_user(const udp_con_t *con,const msg_data_t *msg){
  userdb_t new_user;
  userdb_t *old_user;
  userdb_t backup_user;
  GList *update_entry;
  GList *new_entry;
  int rc;

  if ( (!con) || (!msg) )
    return -EINVAL;


  memset(&new_user,0,sizeof(userdb_t));
  rc=fill_user_info_with_message(con, msg, &new_user);
  if (rc<0) {
    return rc;
  }
  g_static_mutex_lock(&userdb_mutex);
  update_entry=g_list_find_custom(users,&new_user,userdb_find_by_ipaddr);
  if (!update_entry) {
    g_static_mutex_unlock(&userdb_mutex);
    destroy_user_info_contents(&new_user);
    return -ENOMEM;
  } else {
    /*
     * 旧来の情報を削除して新しい情報で更新
     */
    old_user=update_entry->data;
    
    memset(&backup_user,0,sizeof(userdb_t));
    rc=copy_user_info(&backup_user,old_user);
    if (rc<0) {
      destroy_user_info_contents(old_user);
      goto fill_error_out;
    }
    destroy_user_info_contents(old_user);    
    rc=fill_user_info_with_message(con, msg, old_user);
    if (rc<0)
      goto fill_error_out;

    old_user->prio=backup_user.prio;
    destroy_user_info_contents(&backup_user);
    goto ok_out;

  fill_error_out:
    destroy_user_info_contents(&backup_user);
    destroy_user_info(old_user);
    users=g_list_remove_link(users,update_entry);
    g_list_free(update_entry);
    g_static_mutex_unlock(&userdb_mutex);
    return rc;
  }
 ok_out:
  /*
   * 健全性チェック
   */
  new_entry=g_list_find_custom(users,&new_user,userdb_find_by_ipaddr);
  g_assert(new_entry);
  g_assert(new_entry->data == old_user);
  destroy_user_info_contents(&new_user);
  g_static_mutex_unlock(&userdb_mutex);

  notify_userdb_changed();

  return 0;
}
int 
userdb_replace_prio_by_addr(const char *ipaddr,int prio,gboolean need_notify){
  int rc=-ESRCH;
  userdb_t srch_user;
  userdb_t *user_p=NULL;

  if  (!ipaddr)
    return -EINVAL;

  dbg_out("Here: ipaddr:%s prio:%d\n",ipaddr,prio);

  memset(&srch_user,0,sizeof(userdb_t));
  srch_user.ipaddr=(char *)ipaddr;

  g_static_mutex_lock(&userdb_mutex);  

  rc=internal_refer_user_by_addr(ipaddr,(const userdb_t **)&user_p);
  if (rc<0)
    goto unlock_out;

  user_p->prio=prio;
  rc=hostinfo_update_ipmsg_ipaddr_prio(ipaddr,prio);

 unlock_out:
  g_static_mutex_unlock(&userdb_mutex);
  if (need_notify)
    notify_userdb_changed();
  return rc;
}
void
update_all_user_list_view(void){
  notify_userdb_changed();
}
int 
userdb_del_user(const udp_con_t *con,const msg_data_t *msg){
  GList *del_entry;
  userdb_t del_user;
  userdb_t *del_user_p;
  int rc;

  if ( (!con) || (!msg) )
    return -EINVAL;

  memset(&del_user,0,sizeof(userdb_t));

  del_user.ipaddr = g_strdup(udp_get_peeraddr(con));
  if (del_user.ipaddr == NULL) {
    goto  error_out;
  }

  rc = convert_string_internal(refer_user_name_from_msg(msg), (const gchar **)&(del_user.user));

  rc = convert_string_internal(refer_host_name_from_msg(msg), (const gchar **)&(del_user.host));

  dbg_out("del user start\n");
  g_static_mutex_lock(&userdb_mutex);
  del_entry=g_list_find_custom(users,&del_user,userdb_find_by_ipaddr);
  if (!del_entry) {
    rc=-ESRCH;
    dbg_out("No such entry:%s@%s\n",del_user.user,del_user.ipaddr);
    g_static_mutex_unlock(&userdb_mutex);
    goto free_out;
  } 
  del_user_p=del_entry->data;
  dbg_out("Free: %x\n",(unsigned int)del_user_p);
  destroy_user_info(del_user_p);
  users=g_list_remove_link(users,del_entry);
  g_list_free(del_entry);
  g_static_mutex_unlock(&userdb_mutex);

  notify_userdb_changed();

  rc=0;

 free_out:
  dbg_out("del user end\n");
  g_free(del_user.ipaddr);
 free_user_out:
  g_free(del_user.user);
 error_out:
  return rc;
}

int
userdb_get_hostlist_string(int start, int *length, const char **ret_string) {
  int index;
  unsigned int last;
  unsigned int total;
  int rc;
  GList *node;
  size_t str_size=0;
  size_t len=0;
  userdb_t *data;
  char tmp_buff[IPMSG_BUFSIZ];
  unsigned long command;
  char *string;
  size_t remain_len;
  int count;
  int start_no;

  rc=-EINVAL;
  if ( (start<0) || (!length) || ((*length) < 0) || (!ret_string) )
    return rc;

  g_static_mutex_lock(&userdb_mutex);
  last=start + (*length);
  total=g_list_length(users);
  rc=-ENOENT;
  if (!total)
    goto unlock_out;
  --total;
  if ( total < (last) ) 
    last=total;
  
  memset(tmp_buff,0,IPMSG_BUFSIZ);
  snprintf(tmp_buff,IPMSG_BUFSIZ-1,"%5d%c%5d%c",start,HOSTLIST_SEPARATOR,total,HOSTLIST_SEPARATOR);
  str_size=strlen(tmp_buff)+1;

  /*
   *start:length:ユーザ名:ホスト名:コマンド番号:IP アドレス:
   *ポート番号（リトルエンディアン）:ニックネーム:グループ名
   */
  /* length チェック  */
  for(index=start,count=0;
      ( (data=(userdb_t *)g_list_nth_data(users,index)) && (index<=last) );
      ++index){
    memset(tmp_buff,0,IPMSG_BUFSIZ);
    snprintf(tmp_buff,IPMSG_BUFSIZ-1,"%s%c%s%c%d%c%s%c%d%c%s%c%s%c",
	     (data->user)?(data->user):(HOSTLIST_DUMMY),
	     HOSTLIST_SEPARATOR,
	     (data->host)?(data->host):(HOSTLIST_DUMMY),
	     HOSTLIST_SEPARATOR,
	     IPMSG_ANSLIST,
	     HOSTLIST_SEPARATOR,
	     (data->ipaddr)?(data->ipaddr):(HOSTLIST_DUMMY),
	     HOSTLIST_SEPARATOR,
	     hostinfo_refer_ipmsg_port(),
	     HOSTLIST_SEPARATOR,
	     (data->nickname)?(data->nickname):(HOSTLIST_DUMMY),
	     HOSTLIST_SEPARATOR,
	     (data->group)?(data->group):(HOSTLIST_DUMMY),
	     HOSTLIST_SEPARATOR);
    len=strlen(tmp_buff);
    if ( (len + str_size) > (_MSG_BUF_MIN_SIZE / 2) ) {
      last=index;
    }
    str_size += len+1;
    ++count;
  }
  rc = -ENOMEM;
  string = g_malloc(str_size+1);
  if (!string)
    goto unlock_out;

  /*
   * 作成
   */
  memset(string,0,str_size+1);
  if (count)
    --count;
  start_no=((start+count)==total)?(0):start+count; /* 0は終端を示す  */
  snprintf(string,IPMSG_BUFSIZ-1,"%5d%c%5d%c",start_no,HOSTLIST_SEPARATOR,total,HOSTLIST_SEPARATOR);
  dbg_out("Create from %d to %d\n",start,last);
  for(index=start,remain_len=(str_size-strlen(string));
      index<=last;
      ++index){
    data=(userdb_t *)g_list_nth_data(users,index);
    memset(tmp_buff,0,IPMSG_BUFSIZ);
    snprintf(tmp_buff,IPMSG_BUFSIZ-1,"%s%c%s%c%d%c%s%c%d%c%s%c%s%c",
	     (data->user)?(data->user):(HOSTLIST_DUMMY),
	     HOSTLIST_SEPARATOR,
	     (data->host)?(data->host):(HOSTLIST_DUMMY),
	     HOSTLIST_SEPARATOR,
	     IPMSG_ANSLIST,
	     HOSTLIST_SEPARATOR,
	     (data->ipaddr)?(data->ipaddr):(HOSTLIST_DUMMY),
	     HOSTLIST_SEPARATOR,
	     hostinfo_refer_ipmsg_port(),
	     HOSTLIST_SEPARATOR,
	     (data->nickname)?(data->nickname):(HOSTLIST_DUMMY),
	     HOSTLIST_SEPARATOR,
	     (data->group)?(data->group):(HOSTLIST_DUMMY),
	     HOSTLIST_SEPARATOR);
    len=strlen(tmp_buff);
    g_assert(len<=(remain_len-1));
    dbg_out("%d th :%s\n",index,tmp_buff);
    strncat(string,tmp_buff,remain_len);
    remain_len -= len;
  }
  
  len=strlen(string);
  if (string[len-1]==HOSTLIST_SEPARATOR)
    string[len-1]='\0';
  dbg_out("String:%s\n",string);
  rc=0;
  *length=last-start;
  *ret_string=string;
 unlock_out:
  g_static_mutex_unlock(&userdb_mutex);
  
  return rc;
}
int 
userdb_invalidate_userdb(void){
  GList *node;
  userdb_t *del_user;
  GList *new_list=NULL;

  dbg_out("Here\n");

  g_static_mutex_lock(&userdb_mutex);
  for(node=g_list_first(users);node;node=g_list_next (node)) {
    g_assert(node->data);
    del_user=(userdb_t *)node->data;
    /*  Dial Up hostは残留組となる
     *  (可達確認がブロードキャストでできないので)
     */
    if ( del_user->cap & IPMSG_DIALUPOPT) 
      new_list=g_list_append(new_list,del_user); 
    else
      destroy_user_info(del_user);
  }
  g_list_free (users);
  users=new_list;
  g_static_mutex_unlock(&userdb_mutex);

  return 0;
}
int 
userdb_send_broad_cast(const udp_con_t *con,const char *msg,size_t len) {
  GList *node;
  userdb_t *the_user;

  dbg_out("Here\n");

  g_static_mutex_lock(&userdb_mutex);
  for(node=g_list_first(users);node;node=g_list_next (node)) {
    g_assert(node->data);
    the_user=(userdb_t *)node->data;
    dbg_out("Check host:%s\n",the_user->ipaddr);
    if (the_user->cap & IPMSG_DIALUPOPT) {
      if (!the_user->ipaddr) {
	dbg_out("dialup host does not have ipaddr, ignored\n");
	print_one_user_entry((gpointer)the_user,NULL);
      }else{
	dbg_out("%s is dialup host\n",the_user->ipaddr);
	udp_send_broadcast_with_addr(con,the_user->ipaddr,msg,len);
      }
    }
  }
  g_static_mutex_unlock(&userdb_mutex);

  return 0;
}
int
userdb_add_waiter_window(GtkWidget *window){
  if (!window)
    return -EINVAL;

  dbg_out("here %x\n",(unsigned int)window);

  g_static_mutex_lock(&win_mutex);
  waiter_windows=g_list_append(waiter_windows,(gpointer)window);
  g_static_mutex_unlock(&win_mutex);

  return 0;
}
int
userdb_remove_waiter_window(GtkWidget *window){
  if (!window)
    return -EINVAL;

  dbg_out("here %x\n",(unsigned int)window);

  g_static_mutex_lock(&win_mutex);
  waiter_windows=g_list_remove(waiter_windows,(gpointer)window);
  g_static_mutex_unlock(&win_mutex);

  return 0;
}

int 
userdb_replace_public_key_by_addr(const char *ipaddr,const unsigned long peer_cap,const char *key_e,const char *key_n){
  int rc=-ESRCH;
  userdb_t srch_user;
  userdb_t *user_p=NULL;
  gchar *n_str=NULL,*e_str=NULL;

  if ( (!ipaddr) || (!key_e) || (!key_n) )
    return -EINVAL;

  dbg_out("Here: ipaddr:%s key_e:%s key_n: %s\n",ipaddr,key_e,key_n);

  memset(&srch_user,0,sizeof(userdb_t));
  srch_user.ipaddr=(char *)ipaddr;

  g_static_mutex_lock(&userdb_mutex);  

  rc=internal_refer_user_by_addr(ipaddr,(const userdb_t **)&user_p);
  if (rc<0)
    goto error_out;

  rc=-ENOMEM;
  e_str=g_strdup(key_e);
  if (!e_str)
    goto error_out;
  n_str=g_strdup(key_n);
  if (!n_str)
    goto free_e_out;
  
  if (user_p->pub_key_e)
    g_free(user_p->pub_key_e);
  if (user_p->pub_key_n)
    g_free(user_p->pub_key_n);

  user_p->crypt_cap=peer_cap;
  user_p->pub_key_e=e_str;
  user_p->pub_key_n=n_str;
  dbg_out("Register: ipaddr:%s cap:0x%x key_e:%s key_n:%s\n",
	  ipaddr,
	  user_p->crypt_cap,
	  user_p->pub_key_e,
	  user_p->pub_key_n);

 unlock_out:
  g_static_mutex_unlock(&userdb_mutex);

  return 0;

 free_n_out:
  if (n_str)
    g_free(n_str);
 free_e_out:
  if (e_str)
    g_free(e_str);
 error_out:
  g_static_mutex_unlock(&userdb_mutex);
  return rc;
}

int 
userdb_get_public_key_by_addr(const char *peer_addr,unsigned long *cap_p,char **key_e,char **key_n){
  int rc=-ESRCH;
  int i;
  userdb_t srch_user;
  userdb_t *user_p=NULL;
  char *ret_key_buff_e=NULL;
  char *ret_key_buff_n=NULL;
  GTimeVal tmout;
  gboolean status;

  if ( (!peer_addr) || (!key_e) || (!key_n) || (!cap_p) )
    return -EINVAL;

  dbg_out("Get public key: ipaddr:%s\n",peer_addr);

  memset(&srch_user,0,sizeof(userdb_t));
  srch_user.ipaddr=(char *)peer_addr;

  g_static_mutex_lock(&userdb_mutex);  
  rc=internal_refer_user_by_addr(peer_addr,(const userdb_t **)&user_p);
  if (rc<0)
    goto unlock_out;

  dbg_out("ipaddr found:%s\n",peer_addr);
  print_one_user_entry(user_p,NULL);
  if ((user_p->crypt_cap) &&
      (user_p->pub_key_e) &&
      (user_p->pub_key_n) ) {

    rc=-ENOMEM;
    ret_key_buff_e=g_strdup(user_p->pub_key_e);
    if (!ret_key_buff_e)
      goto unlock_out;
    ret_key_buff_n=g_strdup(user_p->pub_key_n);
    if (!ret_key_buff_n)
      goto free_key_e_out;

    *key_e=ret_key_buff_e;
    *key_n=ret_key_buff_n;
    *cap_p=user_p->crypt_cap;
    dbg_out("Return :%x e=%s n=%s\n",*cap_p,*key_e,*key_n);
    rc=0;
  }else{
    dbg_out("No encrypt cap %x e=%s n=%s\n",
	    user_p->crypt_cap,
	    (user_p->pub_key_e) ? (user_p->pub_key_e) : ("NONE"),
	    (user_p->pub_key_n) ? (user_p->pub_key_n) : ("NONE"));
    rc=-ENOENT;
  }

 unlock_out:
  g_static_mutex_unlock(&userdb_mutex);
 nolock_out:
  return rc;

 free_key_n_out:
  if (ret_key_buff_n)
    g_free(ret_key_buff_n);
 free_key_e_out:
  if (ret_key_buff_e)
    g_free(ret_key_buff_e);

  return rc;
}

int 
userdb_get_cap_by_addr(const char *ipaddr, unsigned long *cap_p, unsigned long *crypt_cap_p){
  int rc=-ESRCH;
  userdb_t srch_user;
  userdb_t *user_p=NULL;
  gchar *n_str=NULL,*e_str=NULL;

  if (!ipaddr)
    return -EINVAL;
  
  dbg_out("Here: ipaddr:%s \n",ipaddr);

  memset(&srch_user,0,sizeof(userdb_t));
  srch_user.ipaddr=(char *)ipaddr;

  g_static_mutex_lock(&userdb_mutex);  

  rc=internal_refer_user_by_addr(ipaddr,(const userdb_t **)&user_p);
  if (rc<0)
    goto error_out;
  rc=-ENOENT;
  
  if (cap_p != NULL)
    *cap_p       = user_p->cap;

  if (crypt_cap_p != NULL) {
    *crypt_cap_p = 0; /* 能力が無い状態をデフォルトする  */
#if defined(USE_OPENSSL)
    if ( (user_p->crypt_cap & RSA_CAPS) && (user_p->crypt_cap & SYM_CAPS) )
      *crypt_cap_p = user_p->crypt_cap; /* 暗号化が可能な能力がある  */
#endif /*  USE_OPENSSL  */
  }

  dbg_out("Return cap:%s = %x\n", ipaddr, *cap_p);
 unlock_out:
  g_static_mutex_unlock(&userdb_mutex);

  return 0;

 free_n_out:
  if (n_str)
    g_free(n_str);
 free_e_out:
  if (e_str)
    g_free(e_str);
 error_out:
  g_static_mutex_unlock(&userdb_mutex);
  return rc;
}


int
userdb_wait_public_key(const char *peer_addr,unsigned long *cap_p,char **key_e,char **key_n){
  int rc=-ESRCH;
  int i;
  userdb_t srch_user;
  userdb_t *user_p=NULL;
  GTimeVal tmout;
  gboolean status;
  GTimer *wait_timer=NULL;
  gulong elapsed;

  if ( (!peer_addr) || (!key_e) || (!key_n) || (!cap_p) )
    return -EINVAL;

  dbg_out("Wait public key: ipaddr:%s\n",peer_addr);

  memset(&srch_user,0,sizeof(userdb_t));
  srch_user.ipaddr=(char *)peer_addr;


  rc=userdb_get_public_key_by_addr(peer_addr,cap_p,(char **)key_e,(char **)key_n);
  if (!rc) 
    goto out;

  rc=ipmsg_send_getpubkey(udp_con,peer_addr);
  if (rc)
    goto out;

  wait_timer=g_timer_new();  
  if (!wait_timer)
    goto out;

  for(i=0;i<PUBKEY_MAX_RETRY;++i) {
    for(elapsed=0;elapsed<PUBKEY_WAIT_MICRO_SEC;g_timer_elapsed(wait_timer,&elapsed)) {
      ipmsg_update_ui(); /* udpパケットを処理  */
      rc=userdb_get_public_key_by_addr(peer_addr,cap_p,(char **)key_e,(char **)key_n);
      if (!rc)  /* 見付けた */
	goto free_timer_out;
    }
    /*
     * ある程度待っても更新されなければ,鍵要求送信を再度投げる
     */
    rc=ipmsg_send_getpubkey(udp_con,peer_addr);
    if (rc)
      goto free_timer_out;
    g_timer_start(wait_timer);
  }

 free_timer_out:
  g_assert(wait_timer);
  g_timer_destroy(wait_timer);
 out:
  return rc;
}

int 
userdb_refer_proto_family(const char *ipaddr, int *family) {
	int             rc = -ESRCH;
	userdb_t   *user_p = NULL;
	userdb_t srch_user;

	if  ( (ipaddr == NULL) ||  (family == NULL) )
		return -EINVAL;

	dbg_out("Here: ipaddr:%s \n", ipaddr);

	memset(&srch_user, 0, sizeof(userdb_t));
	srch_user.ipaddr = (char *)ipaddr;

	g_static_mutex_lock(&userdb_mutex);  

	rc = internal_refer_user_by_addr(ipaddr, (const userdb_t **)&user_p);
	if (rc < 0)
		goto unlock_out;

	/*
	 * プロトコルファミリを返却
	 */
	*family = (user_p->pf == PF_INET6) ? (PF_INET6) : (PF_INET);
	
	rc = 0;

unlock_out:
	g_static_mutex_unlock(&userdb_mutex);

  return rc;
}

int
userdb_init_userdb(void){
  users=NULL;

  return 0;
}
int 
userdb_cleanup_userdb(void){

}

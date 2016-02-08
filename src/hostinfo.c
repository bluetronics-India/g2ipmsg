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
 *
 *  2007/06/02 Fix some problems occur when it is installed as new installation
 *             by Yuhei Matsunaga
 *
 */

#include "common.h"

static GConfClient *client=NULL;
static gchar *groupname=NULL;
static gchar *nickname=NULL;
static gchar *hostname=NULL;
static gchar *logfile_string=NULL;
static gchar *cite_string=NULL;
static gboolean current_absent_state=FALSE;
static int absent_id=-1;
static GSList *cached_prio;

GStaticMutex hostinfo_mutex = G_STATIC_MUTEX_INIT;
static gchar *pubkey_pass_string = NULL;
static gchar *lock_pass_string = NULL;
static gchar *encoding_string = NULL;

static const char *keys[] = {
  HOSTINFO_KEY_PORT,
  HOSTINFO_KEY_GROUP,
  HOSTINFO_KEY_NICKNAME,
  HOSTINFO_KEY_MSGSEC,
  HOSTINFO_KEY_CONFIRM_MSG,
  HOSTINFO_KEY_BROADCASTS,
  HOSTINFO_KEY_POPUP,
  HOSTINFO_KEY_SOUND,
  HOSTINFO_KEY_ENCLOSE,
  HOSTINFO_KEY_CITATION,
  HOSTINFO_KEY_LOGFILEPATH,
  HOSTINFO_KEY_ENABLE_LOG,
  HOSTINFO_KEY_LOG_NAME,
  HOSTINFO_KEY_LOG_IPADDR,
  HOSTINFO_KEY_CITE_STRING,
  HOSTINFO_KEY_ABS_TITLE,
  HOSTINFO_KEY_ABS_MSGS,
  HOSTINFO_KEY_DEBUG,
  HOSTINFO_KEY_MSGWIN_WIDTH,
  HOSTINFO_KEY_MSGWIN_HEIGHT,
  HOSTINFO_KEY_RECVWIN_WIDTH,
  HOSTINFO_KEY_RECVWIN_HEIGHT,
  HOSTINFO_KEY_ATTACHWIN_WIDTH,
  HOSTINFO_KEY_ATTACHWIN_HEIGHT,
  HOSTINFO_KEY_ABSENCE_WIDTH,
  HOSTINFO_KEY_ABSENCE_HEIGHT,
  HOSTINFO_KEY_IPV6,
  HOSTINFO_KEY_DIALUP,
  HOSTINFO_KEY_GET_HLIST,
  HOSTINFO_KEY_ALLOW_HLIST,
  HOSTINFO_KEY_USE_SYSTRAY,
  HOSTINFO_KEY_HEADER_VISIBLE,
  HOSTINFO_KEY_HEADER_ORDER,
  HOSTINFO_KEY_SORT_GROUP,
  HOSTINFO_KEY_SUB_SORT_ID,
  HOSTINFO_KEY_SORT_GROUP_DESCENDING,
  HOSTINFO_KEY_SUB_SORT_DESCENDING,
  HOSTINFO_KEY_CRYPT_SPEED,
  HOSTINFO_KEY_CIPHER,
  HOSTINFO_KEY_ENCRYPT_PUBKEY,
  HOSTINFO_KEY_LOCK,
  HOSTINFO_KEY_ENC_PASSWD,
  HOSTINFO_KEY_LOCK_PASSWD,
  HOSTINFO_KEY_LOCK_MSGLOG,
  HOSTINFO_KEY_ICONIFY_DIALOGS,
  HOSTINFO_KEY_EXTERNAL_ENCODING,
  NULL
};

gboolean
hostinfo_refer_debug_state(void) {
  if (!client)
    return FALSE;
  return gconf_client_get_bool(client,HOSTINFO_KEY_DEBUG,NULL);
}

int
hostinfo_send_broad_cast(const udp_con_t *con,const char *msg,size_t len){
  GSList *addr_top,*addr_list;

  udp_send_broadcast(con,msg,len);

  addr_top=addr_list=hostinfo_get_ipmsg_broadcast_list();
  while(addr_list != NULL) {
      gchar *element = addr_list->data;

      dbg_out("Send Broad cast:%s\n",element);
      udp_send_broadcast_with_addr(con,element,msg,len);
      g_free(element);
      addr_list=g_slist_next(addr_list);
  }
  if (addr_top)
    g_slist_free(addr_top);

  return 0;
}
int
hostinfo_set_ipmsg_broadcast_list(GSList *list){
  if (!list){
  	gconf_client_unset(client, HOSTINFO_KEY_BROADCASTS, NULL);
  }
  else{
    gconf_client_set_list(client,HOSTINFO_KEY_BROADCASTS,GCONF_VALUE_STRING,list,NULL);
    gconf_client_clear_cache(client);
  }
  return 0;
}
GSList*
hostinfo_get_ipmsg_broadcast_list(void){
  return gconf_client_get_list(client,HOSTINFO_KEY_BROADCASTS,GCONF_VALUE_STRING,NULL);
}
int 
hostinfo_refer_absent_length(int *max_index){
  int rc=-ESRCH;
  GSList *title_top,*title_list;

  if (!max_index)
    return -EINVAL;

  title_top=title_list=gconf_client_get_list(client,HOSTINFO_KEY_ABS_TITLE,GCONF_VALUE_STRING,NULL);

  if (title_top) {
    *max_index=g_slist_length(title_top);
    rc=0;
  }

  while(title_list != NULL) {
      gchar *element = title_list->data;
      dbg_out("free absent title:%s\n",element);
      g_free(element);
      title_list=g_slist_next(title_list);
  }
  if (title_top)
    g_slist_free(title_top);

  return rc;
}
int
hostinfo_get_absent_title(int index,const char **title){
  int rc=0;
  GSList *title_top,*title_list;
  GSList *title_ref;
  
  title_top=title_list=gconf_client_get_list(client,HOSTINFO_KEY_ABS_TITLE,GCONF_VALUE_STRING,NULL);
  if (!title_top)
    return -ESRCH;

  title_ref=g_slist_nth(title_top,index);
  if (title_ref) {
    *title=(title_ref->data)?(title_ref->data):(_("Undefined"));
    title_top=g_slist_remove_link(title_top,title_ref);
    g_slist_free_1(title_ref);
  }else{
    rc=-ESRCH;
  }
  title_list=title_top;
  while(title_list != NULL) {
      gchar *element = title_list->data;
      dbg_out("Free absent title:%s\n",element);
      g_free(element);
      title_list=g_slist_next(title_list);
  }
  if (title_top)
    g_slist_free(title_top);

  return rc;
}
int
hostinfo_set_ipmsg_absent_title(int index,const char *title){
  int rc=0;
  GSList *title_top,*title_list;
  GSList *title_ref;

  if (!title)
    return -EINVAL;

  title_top=title_list=gconf_client_get_list(client,HOSTINFO_KEY_ABS_TITLE,GCONF_VALUE_STRING,NULL);
  if (!title_top)
    return -ESRCH;

  rc=-ESRCH;
  title_ref=g_slist_nth(title_top,index);
  if (!title_ref) 
    goto free_list_out;

  g_free(title_ref->data);
  title_ref->data=g_strdup(title);
  g_assert(title_ref->data); /* ここで続行すると設定の整合性がとれなくなる  */
  if (!(title_ref->data)) {
    rc=-ENOMEM;
    title_top=g_slist_remove_link(title_top,title_ref);
    g_slist_free_1(title_ref);
    goto free_list_out;
  }

  gconf_client_set_list(client,HOSTINFO_KEY_ABS_TITLE,GCONF_VALUE_STRING,title_top,NULL);
  gconf_client_clear_cache(client);
  rc=0;

 free_list_out:
  title_list=title_top;
  while(title_list != NULL) {
      gchar *element = title_list->data;
      dbg_out("Free absent title:%s\n",element);
      g_free(element);
      title_list=g_slist_next(title_list);
  }
  if (title_top)
    g_slist_free(title_top);

  return rc;
}
/* absent msgs */
int 
hostinfo_refer_absent_message_slots(int *max_index){
  int rc=-ESRCH;
  GSList *message_top,*message_list;

  if (!max_index)
    return -EINVAL;

  message_top=message_list=gconf_client_get_list(client,HOSTINFO_KEY_ABS_MSGS,GCONF_VALUE_STRING,NULL);

  if (message_top) {
    *max_index=g_slist_length(message_top);
    rc=0;
  }

  while(message_list != NULL) {
      gchar *element = message_list->data;
      dbg_out("free absent message:%s\n",element);
      g_free(element);
      message_list=g_slist_next(message_list);
  }
  if (message_top)
    g_slist_free(message_top);

  return rc;
}
int
hostinfo_get_absent_message(int index, char **message){
	int              rc = 0;
	GSList *message_top = NULL, *message_list = NULL;
	char   *ret_message = NULL;
	char   *current_message = NULL;

	message_top = 
		message_list=gconf_client_get_list(client, 
		    HOSTINFO_KEY_ABS_MSGS, GCONF_VALUE_STRING, NULL);

	if (message_top == NULL)
		return -ESRCH;

	current_message = (char *)g_slist_nth_data(message_top, index);
	if (current_message != NULL) 
		ret_message = g_strdup(current_message);
	else
		ret_message = g_strdup(_("Undefined"));

	message_list = message_top;
	while(message_list != NULL) {
		gchar *element = message_list->data;
		if (element == current_message) {
			dbg_out("returned original  message:%s\n", element);
		}
		dbg_out("Free absent message:%s\n", element);
		g_free(element);
		message_list = g_slist_next(message_list);
	}

	if (message_top != NULL)
		g_slist_free(message_top);

	
	rc = 0;
	*message = ret_message;

	dbg_out("Absence Message:%s\n", *message);

error_out:
	return rc;
}
int
hostinfo_set_ipmsg_absent_message(int index,const char *message){
  int rc=0;
  GSList *message_top,*message_list;
  GSList *message_ref;

  if (!message)
    return -EINVAL;

  message_top=message_list=gconf_client_get_list(client,HOSTINFO_KEY_ABS_MSGS,GCONF_VALUE_STRING,NULL);
  if (!message_top)
    return -ESRCH;

  rc=-ESRCH;
  message_ref=g_slist_nth(message_top,index);
  if (!message_ref) 
    goto free_list_out;

  g_free(message_ref->data);
  message_ref->data=g_strdup(message);
  g_assert(message_ref->data); /* ここで続行すると設定の整合性がとれなくなる  */
  if (!(message_ref->data)) {
    rc=-ENOMEM;
    message_top=g_slist_remove_link(message_top,message_ref);
    g_slist_free_1(message_ref);
    goto free_list_out;
  }

  gconf_client_set_list(client,HOSTINFO_KEY_ABS_MSGS,GCONF_VALUE_STRING,message_top,NULL);
  gconf_client_clear_cache(client);
  rc=0;

 free_list_out:
  message_list=message_top;
  while(message_list != NULL) {
      gchar *element = message_list->data;
      dbg_out("Free absent message:%s\n",element);
      g_free(element);
      message_list=g_slist_next(message_list);
  }
  if (message_top)
    g_slist_free(message_top);

  return rc;
}
/* ids */
int 
hostinfo_get_absent_id(int *index){
  int max_index;
  int max_slots;

  if (!index)
    return -EINVAL;

  if (hostinfo_refer_absent_length(&max_index))
    return -EINVAL;

  if (hostinfo_refer_absent_message_slots(&max_slots))
    return -EINVAL;

  g_assert(max_index == max_slots);

  if (absent_id < 0)
    return -EINVAL;

  if (absent_id > max_index)
    return -EINVAL;

  *index=absent_id;
  return 0;
}
int 
hostinfo_set_absent_id(int index){
  int max_index;
  int max_slots;

  if (index<0)
    return -EINVAL;
  if (hostinfo_refer_absent_length(&max_index))
    return -EINVAL;
  if (hostinfo_refer_absent_message_slots(&max_slots))
    return -EINVAL;
  g_assert(max_index==max_slots);

  if (index>max_index)
    return -EINVAL;
  absent_id=index;
  return 0;
}

/** 現在の不在通知文を獲得する
 *  @param[in]  message_p    通知分返却領域を差すポインタのアドレス.
 *  @retval     0            正常終了
 *  @retval    -EINVAL       引数異常
 *  @retval    -ENOMEM       メモリ不足
 */
int
hostinfo_get_current_absence_message(char **message_p) {
	int               rc  = 0;
	int             index = 0;
	char *absence_message = NULL;

	/*
	 * 引数異常
	 */
	if (message_p == NULL) {
		rc = -EINVAL;
		goto no_free_out;
	}


	if ( hostinfo_is_ipmsg_absent() ) {
		/*
		 * 現在の不在設定値を獲得する.
		 */
		rc = hostinfo_get_absent_id(&index);
		if ( rc != 0 ) {
			goto no_free_out;
		}

		/*
		 * 不在通知文を獲得する.
		 */
		rc = hostinfo_get_absent_message(index, &absence_message);
		if (rc != 0) {
			g_assert(absence_message == NULL);
			goto no_free_out;
		}
	} else {
		/*
		 * 不在中でなければ, 在席通知を返却する
		 */
		absence_message = g_strdup(_("I am here."));
		if (absence_message == NULL) {
			rc = -ENOMEM;
			goto no_free_out;
		}
	}
		
	*message_p = absence_message;

	rc = 0; /* 正常終了 */
		
no_free_out:
	return rc;
}

gboolean
hostinfo_is_ipmsg_absent(void) {

  return current_absent_state;
}
gboolean
hostinfo_set_ipmsg_absent(gboolean state) {
  current_absent_state=state;
  return current_absent_state;
}

gboolean
hostinfo_refer_ipmsg_default_secret(void) {

  return gconf_client_get_bool(client,HOSTINFO_KEY_MSGSEC,NULL);
}
gboolean
hostinfo_set_ipmsg_default_secret(gboolean val) {

  gconf_client_clear_cache(client);
  return gconf_client_set_bool(client,HOSTINFO_KEY_MSGSEC,val,NULL);
}
gboolean
hostinfo_refer_ipmsg_default_confirm(void) {

  return gconf_client_get_bool(client,HOSTINFO_KEY_CONFIRM_MSG,NULL);
}
gboolean
hostinfo_set_ipmsg_default_confirm(gboolean val) {

  gconf_client_clear_cache(client);
  return gconf_client_set_bool(client,HOSTINFO_KEY_CONFIRM_MSG,val,NULL);
}
gboolean
hostinfo_refer_ipmsg_default_popup(void) {

  return gconf_client_get_bool(client,HOSTINFO_KEY_POPUP,NULL);
}
gboolean
hostinfo_set_ipmsg_default_popup(gboolean val) {

  gconf_client_clear_cache(client);
  return gconf_client_set_bool(client,HOSTINFO_KEY_POPUP,val,NULL);
}

gboolean
hostinfo_refer_ipmsg_default_sound(void) {

  return gconf_client_get_bool(client,HOSTINFO_KEY_SOUND,NULL);
}
gboolean
hostinfo_set_ipmsg_default_sound(gboolean val) {

  gconf_client_clear_cache(client);
  return gconf_client_set_bool(client,HOSTINFO_KEY_SOUND,val,NULL);
}
gboolean
hostinfo_refer_ipmsg_default_enclose(void) {

  return gconf_client_get_bool(client,HOSTINFO_KEY_ENCLOSE,NULL);
}
gboolean
hostinfo_set_ipmsg_default_enclose(gboolean val) {

  gconf_client_clear_cache(client);
  return gconf_client_set_bool(client,HOSTINFO_KEY_ENCLOSE,val,NULL);
}

gboolean
hostinfo_refer_ipmsg_default_citation(void) {

  return gconf_client_get_bool(client,HOSTINFO_KEY_CITATION,NULL);
}
gboolean
hostinfo_set_ipmsg_default_citation(gboolean val) {

  gconf_client_clear_cache(client);
  return gconf_client_set_bool(client,HOSTINFO_KEY_CITATION,val,NULL);
}

gboolean
hostinfo_refer_ipmsg_ipaddr_logging(void) {

  return gconf_client_get_bool(client,HOSTINFO_KEY_LOG_IPADDR,NULL);
}
gboolean
hostinfo_set_ipmsg_ipaddr_logging(gboolean val) {

  gconf_client_clear_cache(client);
  return gconf_client_set_bool(client,HOSTINFO_KEY_LOG_IPADDR,val,NULL);
}

gboolean
hostinfo_refer_ipmsg_logname_logging(void) {

  return gconf_client_get_bool(client,HOSTINFO_KEY_LOG_NAME,NULL);
}
gboolean
hostinfo_set_ipmsg_logname_logging(gboolean val) {

  gconf_client_clear_cache(client);
  return gconf_client_set_bool(client,HOSTINFO_KEY_LOG_NAME,val,NULL);
}

gboolean
hostinfo_refer_ipmsg_enable_log(void) {

  return gconf_client_get_bool(client,HOSTINFO_KEY_ENABLE_LOG,NULL);
}

gboolean
hostinfo_set_ipmsg_enable_log(gboolean val) {

  gconf_client_clear_cache(client);
  return gconf_client_set_bool(client,HOSTINFO_KEY_ENABLE_LOG,val,NULL);
}

gboolean
hostinfo_refer_ipmsg_ipv6_mode(void) {

  return gconf_client_get_bool(client,HOSTINFO_KEY_IPV6,NULL);
}
gboolean
hostinfo_set_ipmsg_ipv6_mode(gboolean val) {

  return gconf_client_set_bool(client,HOSTINFO_KEY_IPV6,val,NULL);
}

gboolean
hostinfo_refer_ipmsg_is_get_hlist(void) {

  return gconf_client_get_bool(client,HOSTINFO_KEY_GET_HLIST,NULL);
}
gboolean
hostinfo_set_ipmsg_is_get_hlist(gboolean val) {

  gconf_client_clear_cache(client);
  return gconf_client_set_bool(client,HOSTINFO_KEY_GET_HLIST,val,NULL);
}

gboolean
hostinfo_refer_ipmsg_is_allow_hlist(void) {

  return gconf_client_get_bool(client,HOSTINFO_KEY_ALLOW_HLIST,NULL);
}
gboolean
hostinfo_set_ipmsg_is_allow_hlist(gboolean val) {

  gconf_client_clear_cache(client);
  return gconf_client_set_bool(client,HOSTINFO_KEY_ALLOW_HLIST,val,NULL);
}

gboolean
hostinfo_refer_ipmsg_dialup_mode(void) {

  return gconf_client_get_bool(client,HOSTINFO_KEY_DIALUP,NULL);
}
gboolean
hostinfo_set_ipmsg_dialup_mode(gboolean val) {

  gconf_client_clear_cache(client);
  return gconf_client_set_bool(client,HOSTINFO_KEY_DIALUP,val,NULL);
}
gboolean
hostinfo_refer_ipmsg_is_sort_with_group(void) {

  return gconf_client_get_bool(client,HOSTINFO_KEY_SORT_GROUP,NULL);
}
gboolean
hostinfo_set_ipmsg_sort_with_group(gboolean val) {

  gconf_client_clear_cache(client);
  return gconf_client_set_bool(client,HOSTINFO_KEY_SORT_GROUP,val,NULL);
}

gint
hostinfo_refer_ipmsg_sub_sort_id(void) {

  return gconf_client_get_int(client,HOSTINFO_KEY_SUB_SORT_ID,NULL);
}
gboolean
hostinfo_set_ipmsg_sub_sort_id(gint val) {

  gconf_client_clear_cache(client);
  return gconf_client_set_int(client,HOSTINFO_KEY_SUB_SORT_ID,val,NULL);
}
gboolean
hostinfo_refer_ipmsg_group_sort_order(void) {

  return !(gconf_client_get_bool(client,HOSTINFO_KEY_SORT_GROUP_DESCENDING,NULL));
}
gboolean
hostinfo_set_ipmsg_group_sort_order(gboolean val) {

  gconf_client_clear_cache(client);
  return gconf_client_set_bool(client,HOSTINFO_KEY_SORT_GROUP_DESCENDING,(!(val)),NULL);
}
gboolean
hostinfo_refer_ipmsg_sub_sort_order(void) {

  return !(gconf_client_get_bool(client,HOSTINFO_KEY_SUB_SORT_DESCENDING,NULL));
}
gboolean
hostinfo_set_ipmsg_sub_sort_order(gboolean val) {

  gconf_client_clear_cache(client);
  return gconf_client_set_bool(client,HOSTINFO_KEY_SUB_SORT_DESCENDING,(!(val)),NULL);
}

int 
hostinfo_set_ipmsg_logfile(const char *file) {
  gchar *string;
  int rc;

  if (!file)
    return -EINVAL;

  rc=logfile_reopen_logfile(file);
  if (rc)
    return rc;

  g_static_mutex_lock(&hostinfo_mutex);  

  if (logfile_string)
    g_free(logfile_string);

  logfile_string=NULL;

  gconf_client_set_string(client,HOSTINFO_KEY_LOGFILEPATH,file,NULL);

  string=gconf_client_get_string(client,HOSTINFO_KEY_LOGFILEPATH,NULL);
  if (string) {
    dbg_out("gconf return logfile %s\n",string);
    g_static_mutex_unlock(&hostinfo_mutex);  
    logfile_string=string;
    gconf_client_clear_cache(client);
    return 0;
  }
  g_static_mutex_unlock(&hostinfo_mutex);  
  return -ENOMEM;
}
const char *
hostinfo_refer_ipmsg_logfile(void) {
  char *string;

  g_static_mutex_lock(&hostinfo_mutex);  
  if (logfile_string)
    goto unlock_out;

  string=gconf_client_get_string(client,HOSTINFO_KEY_LOGFILEPATH,NULL);

  if (string) {
    dbg_out("gconf return logfile %s\n",string);
    g_static_mutex_unlock(&hostinfo_mutex);  
    logfile_string=string;
  }  
 unlock_out:
  g_static_mutex_unlock(&hostinfo_mutex);  
  return (logfile_string)?(logfile_string):("");
}

const char *
hostinfo_refer_ipmsg_cite_string(void) {
  char *string;

  if (cite_string)
    return cite_string;

  g_static_mutex_lock(&hostinfo_mutex);
  string=gconf_client_get_string(client,HOSTINFO_KEY_CITE_STRING,NULL);

  if (string) {
    dbg_out("gconf return cite string:%s\n",string);
    cite_string=string;
    g_static_mutex_unlock(&hostinfo_mutex);  
    return (const char *)cite_string;
  }
  g_static_mutex_unlock(&hostinfo_mutex);  
  return ">";
}
int
hostinfo_refer_ipmsg_port(void){
  int port;

  g_static_mutex_lock(&hostinfo_mutex);  
  port=gconf_client_get_int(client,HOSTINFO_KEY_PORT,NULL);
  dbg_out("gconf return port:%d\n",port);
  g_static_mutex_unlock(&hostinfo_mutex);  
  if (!port) 
    return DEFAULT_PORT;
  
  return port;
}
int
hostinfo_set_ipmsg_port(int port){
  int new;

  g_static_mutex_lock(&hostinfo_mutex);  
  gconf_client_set_int(client,HOSTINFO_KEY_PORT,port,NULL);
  gconf_client_clear_cache(client);
  new=gconf_client_get_int(client,HOSTINFO_KEY_PORT,NULL);
  dbg_out("gconf return port:%d\n",new);
  g_static_mutex_unlock(&hostinfo_mutex);  
  if (!new)
    return DEFAULT_PORT;

  return port;
}

const char *
hostinfo_refer_user_name(void) {
  struct passwd *ref_p;

  g_static_mutex_lock(&hostinfo_mutex);  
  ref_p=getpwuid(getuid());

  if (!ref_p) {
    err_out("Can not get pwent:%s (%d)\n",strerror(errno),errno);
    g_static_mutex_unlock(&hostinfo_mutex);  
    return _("UnKnown");
  }
  dbg_out("UserName:%s\n",ref_p->pw_name);
  g_static_mutex_unlock(&hostinfo_mutex);  
  return ref_p->pw_name;
}

const char *
hostinfo_refer_group_name(void) {

  g_static_mutex_lock(&hostinfo_mutex);  
  if (!groupname) {
    groupname=gconf_client_get_string(client,HOSTINFO_KEY_GROUP,NULL);
    dbg_out("gconf return groupname:%s\n",groupname);
  }
  g_static_mutex_unlock(&hostinfo_mutex);  
  return (groupname)?(groupname):(_("Default Group"));
}
int
hostinfo_set_group_name(const char *groupName) {

  if (!groupName)
    return -EINVAL;

  dbg_out("set groupname:%s\n",groupName);

  g_static_mutex_lock(&hostinfo_mutex);  
  if (groupname)
    g_free(groupname);

  g_assert(client);

  gconf_client_set_string(client,HOSTINFO_KEY_GROUP,groupName,NULL);
  gconf_client_clear_cache(client);
  groupname=gconf_client_get_string(client,HOSTINFO_KEY_GROUP,NULL);
  dbg_out("gconf return groupname:%s\n",groupname);
  g_static_mutex_unlock(&hostinfo_mutex);  
  return 0;
}

int
hostinfo_set_nick_name(const char *nickName) {

  if (!nickName)
    return -EINVAL;

  g_static_mutex_lock(&hostinfo_mutex);  
  dbg_out("set nickName:%s\n",nickName);

  g_assert(client);

  if (nickname)
    g_free(nickname);

  gconf_client_set_string(client,HOSTINFO_KEY_NICKNAME,nickName,NULL);
  gconf_client_clear_cache(client);
  nickname=gconf_client_get_string(client,HOSTINFO_KEY_NICKNAME,NULL);
  dbg_out("gconf return nick name:%s\n",nickname);
  g_static_mutex_unlock(&hostinfo_mutex);  
  return 0;
}

const char *
hostinfo_refer_nick_name(void) {

  g_static_mutex_lock(&hostinfo_mutex);  
  if (!nickname) {
    nickname=gconf_client_get_string(client,HOSTINFO_KEY_NICKNAME,NULL);
    dbg_out("gconf return nick name:%s\n",nickname);
  }
  g_static_mutex_unlock(&hostinfo_mutex);  
  return (nickname)?(nickname):(_("Default Nick Name"));
}

const char *
hostinfo_refer_host_name(void){

	if (hostname != NULL)
		goto ret;

	g_static_mutex_lock(&hostinfo_mutex);  
	hostname = g_malloc(128);

	if (gethostname(hostname, 128) < 0) {
		g_free(hostname);
		hostname = NULL;
		g_static_mutex_unlock(&hostinfo_mutex);  
		return _("UnKnown");
	}
	hostname[127]=0;
	g_static_mutex_unlock(&hostinfo_mutex);  

ret:
	return hostname;
}


static void
hostinfo_gconf_notify_func(GConfClient *client, guint cnxn_id, GConfEntry  *entry,
			   gpointer user_data){
  dbg_out("Configuration is changed: key=%s\n",(char *)user_data);
}
unsigned long 
hostinfo_get_normal_send_flags(void){
  unsigned long flags=0;

  g_static_mutex_lock(&hostinfo_mutex);  
  flags=G2IPMSG_DEFAULT_SEND_FLAGS; /* 受信確認をデフォルト  */

  if (gconf_client_get_bool(client,HOSTINFO_KEY_MSGSEC,NULL))
    flags|=IPMSG_SECRETOPT;

  if (gconf_client_get_bool(client,HOSTINFO_KEY_CONFIRM_MSG,NULL))
    flags|=IPMSG_SENDCHECKOPT;

  dbg_out("gconf return flags:%x\n",flags);
  g_static_mutex_unlock(&hostinfo_mutex);  
  return flags;
}
unsigned long 
hostinfo_get_normal_entry_flags(void){
  unsigned long flags=G2IPMSG_DEFAULT_ENTRY_FLAGS;

  g_static_mutex_lock(&hostinfo_mutex);  
  if (hostinfo_refer_ipmsg_dialup_mode())
    flags |= IPMSG_DIALUPOPT;

  if (hostinfo_is_ipmsg_absent()) {
    flags |= IPMSG_ABSENCEOPT;
    dbg_out("Inabsense mode :%x\n",flags);
  }
  dbg_out("gconf return flags:%x\n",flags);
  g_static_mutex_unlock(&hostinfo_mutex);  
  return flags;
}
static int 
hostinfo_get_ipmsg_window_size(const gchar *wkey,const gchar *hkey,gint *width,gint *height){

  dbg_out("here\n");

  if ( (!wkey) || (!hkey) ||  (!width) || (!height) )
    return -EINVAL;

  g_static_mutex_lock(&hostinfo_mutex);
  *width=gconf_client_get_int(client,wkey,NULL);
  *height=gconf_client_get_int(client,hkey,NULL);
  g_static_mutex_unlock(&hostinfo_mutex);

  dbg_out("gconf return size:(%s,%s)=(%d,%d)\n",wkey,hkey,*width,*height);

  return 0;
}
static int 
hostinfo_set_ipmsg_window_size(const gchar *wkey,const gchar *hkey,gint width,gint height){
  int rwidth,rheight;

  dbg_out("here\n");

  if ( (!wkey) || (!hkey) )
    return -EINVAL;

  dbg_out("gconf set size:(%s,%s)=(%d,%d)\n",wkey,hkey,width,height);


  g_static_mutex_lock(&hostinfo_mutex);

  gconf_client_set_int(client,wkey,width,NULL);
  gconf_client_set_int(client,hkey,height,NULL);

  gconf_client_clear_cache(client);

  rwidth=gconf_client_get_int(client,wkey,NULL);
  rheight=gconf_client_get_int(client,hkey,NULL);

  g_static_mutex_unlock(&hostinfo_mutex);


  dbg_out("gconf get size:(%s,%s)=(%d,%d)\n",wkey,hkey,rwidth,rheight);

  return 0;
}

int 
hostinfo_get_ipmsg_message_window_size(gint *width,gint *height){
  int rc;
  if ( (!width) || (!height) )
    return -EINVAL;

  rc=hostinfo_get_ipmsg_window_size(HOSTINFO_KEY_MSGWIN_WIDTH,
				 HOSTINFO_KEY_MSGWIN_HEIGHT,
				 width,
				 height);
  dbg_out("gconf return size:(%d,%d)\n",*width,*height);

  return rc;
}
int 
hostinfo_set_ipmsg_message_window_size(gint width,gint height){
  int rc;

  dbg_out("gconf set size:(%d,%d)\n",width,height);
  rc=hostinfo_set_ipmsg_window_size(HOSTINFO_KEY_MSGWIN_WIDTH,
				 HOSTINFO_KEY_MSGWIN_HEIGHT,
				 width,
				 height);
  return rc;
}
int 
hostinfo_get_ipmsg_recv_window_size(gint *width,gint *height){
  int rc;
  if ( (!width) || (!height) )
    return -EINVAL;

  rc=hostinfo_get_ipmsg_window_size(HOSTINFO_KEY_RECVWIN_WIDTH,
				 HOSTINFO_KEY_RECVWIN_HEIGHT,
				 width,
				 height);
  dbg_out("gconf return size:(%d,%d)\n",*width,*height);

  return rc;
}
int 
hostinfo_set_ipmsg_recv_window_size(gint width,gint height){
  int rc;

  dbg_out("gconf set size:(%d,%d)\n",width,height);
  rc=hostinfo_set_ipmsg_window_size(HOSTINFO_KEY_RECVWIN_WIDTH,
				 HOSTINFO_KEY_RECVWIN_HEIGHT,
				 width,
				 height);
  return rc;
}

int 
hostinfo_get_ipmsg_attach_editor_size(gint *width,gint *height){
  int rc;
  if ( (!width) || (!height) )
    return -EINVAL;

  rc=hostinfo_get_ipmsg_window_size(HOSTINFO_KEY_ATTACHWIN_WIDTH,
				 HOSTINFO_KEY_ATTACHWIN_HEIGHT,
				 width,
				 height);
  dbg_out("gconf return size:(%d,%d)\n",*width,*height);

  return rc;
}
int 
hostinfo_set_ipmsg_attach_editor_size(gint width,gint height){
  int rc;

  dbg_out("gconf set size:(%d,%d)\n",width,height);
  rc=hostinfo_set_ipmsg_window_size(HOSTINFO_KEY_ATTACHWIN_WIDTH,
				 HOSTINFO_KEY_ATTACHWIN_HEIGHT,
				 width,
				 height);
  return rc;
}

int 
hostinfo_get_ipmsg_absence_editor_size(gint *width,gint *height){
  int rc;
  if ( (!width) || (!height) )
    return -EINVAL;

  rc=hostinfo_get_ipmsg_window_size(HOSTINFO_KEY_ABSENCE_WIDTH,
				 HOSTINFO_KEY_ABSENCE_HEIGHT,
				 width,
				 height);
  dbg_out("gconf return size:(%d,%d)\n",*width,*height);

  return rc;
}
int 
hostinfo_set_ipmsg_absence_editor_size(gint width,gint height){
  int rc;

  dbg_out("gconf set size:(%d,%d)\n",width,height);
  rc=hostinfo_set_ipmsg_window_size(HOSTINFO_KEY_ABSENCE_WIDTH,
				 HOSTINFO_KEY_ABSENCE_HEIGHT,
				 width,
				 height);
  return rc;
}

unsigned long 
hostinfo_get_ipmsg_crypt_capability(void){
  int rc;
  unsigned long state;

  rc=hostinfo_refer_ipmsg_cipher(&state);

  if (!rc)
    return state;

  return 0;
}
int 
hostinfo_get_ipmsg_system_addr_family(void){
  if (gconf_client_get_bool(client,HOSTINFO_KEY_IPV6,NULL))
    return AF_INET6;
  else
    return AF_INET; /*  デフォルトはIPV4 */
}

gboolean 
is_sound_system_available(void){
#if defined(HAVE_GST)
  return TRUE;
#else
  return FALSE;
#endif  /*  HAVE_GST  */
}
static int 
update_cached_prio(void) {
  GSList *conf_top,*conf_list;
  GSList *conf_ref;

  conf_list=cached_prio;
  while(conf_list != NULL) {
      gchar *element = conf_list->data;
   
      if (element) 
	g_free(element);
      conf_list=g_slist_next(conf_list);
  }
  if (cached_prio)
    g_slist_free(conf_top);

  cached_prio=conf_list=gconf_client_get_list(client,HOSTINFO_KEY_DEFAULT_PRIO,GCONF_VALUE_STRING,NULL);
}
int
hostinfo_update_ipmsg_ipaddr_prio(const char *ipaddr,int prio){
  int rc=0;
  GSList *conf_top,*conf_list;
  GSList *conf_ref;
  gchar *addr;
  gchar *prio_str;
  gchar buff[1024];

  if (!ipaddr)
    return -EINVAL;

  conf_top=conf_list=gconf_client_get_list(client,HOSTINFO_KEY_DEFAULT_PRIO,GCONF_VALUE_STRING,NULL);

  memset(buff,0,1024);
  snprintf(buff,1023,"%d%c%s",prio,HOSTINFO_PRIO_SEPARATOR,ipaddr);
  buff[1023]='\0';

  for(conf_ref=conf_top;conf_ref;conf_ref=g_slist_next(conf_ref)) {
    if (conf_ref->data) {
      prio_str=conf_ref->data;
      /* 優先度@アドレスの形式 */
      dbg_out("Addr-info:%s\n",prio_str);
      addr=strchr(prio_str,HOSTINFO_PRIO_SEPARATOR);
      g_assert(addr);
      ++addr;
      if (!strcmp(addr,ipaddr)) {
	g_free(conf_ref->data);
	conf_ref->data=g_strdup(buff);
	if (!conf_ref->data) {
	  rc=-ENOMEM;
	  goto free_list_out;
	}
	dbg_out("Replace prio:%s\n",conf_ref->data);
	goto start_config;
      }
    }
  }

  prio_str=g_strdup(buff);
  if (!prio_str) {
    rc=-ENOMEM;
    goto free_list_out;
  }
  conf_top=g_slist_append(conf_top,prio_str);

 start_config:
  gconf_client_set_list(client,HOSTINFO_KEY_DEFAULT_PRIO,GCONF_VALUE_STRING,conf_top,NULL);
  gconf_client_clear_cache(client);
  rc=0;
  update_cached_prio();

 free_list_out:
  conf_list=conf_top;
  while(conf_list != NULL) {
      gchar *element = conf_list->data;
   
      if (element) 
	g_free(element);
      conf_list=g_slist_next(conf_list);
  }
  if (conf_top)
    g_slist_free(conf_top);



  return rc;
}
int
hostinfo_get_ipmsg_ipaddr_prio(const char *ipaddr,int *prio){
  GSList *conf_top,*conf_list;
  GSList *conf_ref;
  gchar *addr;
  gchar *tmp_str;
  gchar *prio_str;
  int val=0;

  if ( (!ipaddr) || (!prio) )
    return -EINVAL;

  if (!cached_prio)
    update_cached_prio();

  conf_top=cached_prio;

  for(conf_ref=conf_top;conf_ref;conf_ref=g_slist_next(conf_ref)) {
    if (conf_ref->data) {
      prio_str=g_strdup(conf_ref->data);
      if (!prio_str) {
	*prio=0; /* FIXME */
	return -ENOMEM;
      }
      /* 優先度@アドレスの形式 */
      dbg_out("Addr-info:%s\n",prio_str);

      addr=strchr(prio_str,HOSTINFO_PRIO_SEPARATOR);
      g_assert(addr);
      *addr='\0';
      ++addr;
      if (!strcmp(addr,ipaddr)) {
	val=strtol(prio_str, (char **)NULL, 10);
	if (prio_is_valid(val)) {
	  dbg_out("default prio:%d (%s)\n",val,addr);
	} else {
	  dbg_out("default prio invalid :%s\n",addr);
	  val=0;
	}
	g_free(prio_str);
	break;
      }
      g_free(prio_str);
    }
  }
  *prio=val;

  return 0;
}
gboolean 
hostinfo_refer_enable_systray(void){
  if (!client)
    return FALSE;
  return gconf_client_get_bool(client,HOSTINFO_KEY_USE_SYSTRAY,NULL);
}
gboolean 
hostinfo_set_enable_systray(gboolean val){
  gboolean rc;
  uint state;

  g_static_mutex_lock(&hostinfo_mutex);

  rc=gconf_client_set_bool(client,HOSTINFO_KEY_HEADER_VISIBLE,val,NULL);

  gconf_client_clear_cache(client);
  
  state=gconf_client_get_int(client,HOSTINFO_KEY_HEADER_VISIBLE,NULL);

  dbg_out("Set header:%x \n",state);

  g_static_mutex_unlock(&hostinfo_mutex);

  return rc;
}
guint
hostinfo_refer_header_state(void){
  guint state;

  g_static_mutex_lock(&hostinfo_mutex);

  state=gconf_client_get_int(client,HOSTINFO_KEY_HEADER_VISIBLE,NULL);

  dbg_out("Header:%x \n",state);

  g_static_mutex_unlock(&hostinfo_mutex);

  return state;
}
gboolean 
hostinfo_set_header_state(guint val){
  guint state;

  g_static_mutex_lock(&hostinfo_mutex);
  gconf_client_set_int(client,HOSTINFO_KEY_HEADER_VISIBLE,val,NULL);
  gconf_client_clear_cache(client);
  g_static_mutex_unlock(&hostinfo_mutex);

}
int
hostinfo_get_header_order(int index,int *col_id){
  int rc=0;
  GSList *header_top;
  GSList *header_ref;
  GSList *tmplist = NULL;

  if ( (index<0) || (index>=MAX_VIEW_ID) )
    return -EINVAL;
  if (!col_id)
    return -EINVAL;

  header_top=gconf_client_get_list(client,HOSTINFO_KEY_HEADER_ORDER,GCONF_VALUE_INT,NULL);

  if (!header_top){
    tmplist = g_slist_append(tmplist, GINT_TO_POINTER(index));
    gconf_client_set_list(client, HOSTINFO_KEY_HEADER_ORDER, GCONF_VALUE_INT,
			  tmplist, NULL);
    gconf_client_clear_cache(client);
  } else if (g_slist_find(header_top,   GINT_TO_POINTER(index)) == NULL) {

    header_top = g_slist_append(header_top, GINT_TO_POINTER(index));
    gconf_client_set_list(client, HOSTINFO_KEY_HEADER_ORDER, 
			  GCONF_VALUE_INT, header_top, NULL);
    gconf_client_clear_cache(client);
  }

  rc=-ESRCH;
  header_ref=g_slist_nth(header_top,index);
  if (!header_ref) 
    goto free_list_out;

  *col_id=(int)header_ref->data;

  gconf_client_set_list(client,HOSTINFO_KEY_HEADER_ORDER,GCONF_VALUE_INT,header_top,NULL);
  gconf_client_clear_cache(client);
  rc=0;

 free_list_out:
  if (header_top)
    g_slist_free(header_top);

  return rc;
}
int
hostinfo_set_ipmsg_header_order(int index,int col_id){
  int rc=0;
  GSList *header_top;
  GSList *header_ref;

  if ( (index<0) || (index>=MAX_VIEW_ID) )
    return -EINVAL;

  header_top=gconf_client_get_list(client,HOSTINFO_KEY_HEADER_ORDER,GCONF_VALUE_INT,NULL);
  if (!header_top)
    return -ESRCH;

  rc=-ESRCH;
  header_ref=g_slist_nth(header_top,index);
  if (header_ref) {
    header_ref->data=(gpointer)col_id;

    gconf_client_set_list(client,HOSTINFO_KEY_HEADER_ORDER,GCONF_VALUE_INT,header_top,NULL);
    gconf_client_clear_cache(client);
    rc=0;
  }
  if (header_top)
    g_slist_free(header_top);

  return rc;
}
gboolean
hostinfo_refer_ipmsg_crypt_policy_is_speed(void) {
  return gconf_client_get_bool(client,HOSTINFO_KEY_CRYPT_SPEED,NULL);
}
int 
hostinfo_set_ipmsg_crypt_policy_as_speed(gboolean val){
  gconf_client_clear_cache(client);
  return gconf_client_set_bool(client,HOSTINFO_KEY_CRYPT_SPEED,val,NULL);
}
gboolean 
hostinfo_refer_ipmsg_encrypt_public_key(void){
  return gconf_client_get_bool(client,HOSTINFO_KEY_ENCRYPT_PUBKEY,NULL);
}
int 
hostinfo_set_ipmsg_encrypt_public_key(gboolean val){
  gconf_client_clear_cache(client);
  return gconf_client_set_bool(client,HOSTINFO_KEY_ENCRYPT_PUBKEY,val,NULL);
}
gboolean 
hostinfo_refer_ipmsg_use_lock(void){
  return gconf_client_get_bool(client,HOSTINFO_KEY_LOCK,NULL);
}
int 
hostinfo_set_ipmsg_use_lock(gboolean val){
  gconf_client_clear_cache(client);
  return gconf_client_set_bool(client,HOSTINFO_KEY_LOCK,val,NULL);
}

int
hostinfo_refer_ipmsg_cipher(unsigned long *cipher) {
  unsigned long cipher_state;

  if (!cipher)
    return -EINVAL;

#if !defined(USE_OPENSSL)
  return -ENOSYS;  /* Not supported  */
#endif  /* !USE_OPENSSL  */

  cipher_state = gconf_client_get_int(client,HOSTINFO_KEY_CIPHER,NULL);
  if ( ( (cipher_state & RSA_CAPS) == 0)  ||
       ( (cipher_state & SYM_CAPS) == 0) ){
	 war_out("Invalid enc-capabirity:%x\n", cipher_state);
	 cipher_state = G2IPMSG_CRYPTO_CAP;
	 war_out("Force to set:%x\n", cipher_state);
	 hostinfo_set_ipmsg_cipher(cipher_state);
       }

  *cipher=cipher_state;

  return 0;
}

int 
hostinfo_set_ipmsg_cipher(unsigned long val){
  gconf_client_clear_cache(client);
  
  return gconf_client_set_int(client,HOSTINFO_KEY_CIPHER,val,NULL);
}

static void
hostinfo_update_internal_password(int type) {
  g_static_mutex_lock(&hostinfo_mutex);  

  switch(type) {
  case HOSTINFO_PASSWD_TYPE_ENCKEY:
	  if (pubkey_pass_string == NULL) {
		  pubkey_pass_string = 
			  gconf_client_get_string(client, HOSTINFO_KEY_ENC_PASSWD, NULL);
		  dbg_out("gconf return encoded password for keys:%s\n", 
		      (pubkey_pass_string != NULL) ? (pubkey_pass_string) : ("Not set yet"));
	  }
	  break;
  case HOSTINFO_PASSWD_TYPE_LOCK:
	  if (lock_pass_string == NULL) {
		  lock_pass_string = 
			  gconf_client_get_string(client, HOSTINFO_KEY_LOCK_PASSWD, NULL);
		  dbg_out("gconf return encoded password for lock:%s\n", 
		      (lock_pass_string != NULL) ? (lock_pass_string) : ("Not set yet"));
	  }
	  break;
  default:
	  g_assert_not_reached();
	  break;
  }
  g_static_mutex_unlock(&hostinfo_mutex);
}

static void
hostinfo_register_password(int type, const char *encoded_password) {
	
	g_assert(encoded_password != NULL);

	dbg_out("set encryption key password(encoded):%s\n", encoded_password);

	g_static_mutex_lock(&hostinfo_mutex);  

	g_assert(client);

	switch(type) {
	case HOSTINFO_PASSWD_TYPE_ENCKEY:
		if (pubkey_pass_string != NULL) {
			g_free(pubkey_pass_string);
			pubkey_pass_string = NULL;
		}

		/*
		 * Update internal cache
		 */
		gconf_client_set_string(client, HOSTINFO_KEY_ENC_PASSWD, encoded_password, NULL);
		gconf_client_clear_cache(client);
		pubkey_pass_string =
			gconf_client_get_string(client, HOSTINFO_KEY_ENC_PASSWD, NULL);
		break;
	case HOSTINFO_PASSWD_TYPE_LOCK:
		if (lock_pass_string != NULL) {
			g_free(lock_pass_string);
			lock_pass_string = NULL;
		}

		/*
		 * Update internal cache
		 */
		gconf_client_set_string(client, HOSTINFO_KEY_LOCK_PASSWD, encoded_password, NULL);
		gconf_client_clear_cache(client);
		lock_pass_string =
			gconf_client_get_string(client, HOSTINFO_KEY_LOCK_PASSWD, NULL);
		break;
	default:
		g_assert_not_reached();
		break;
	}

	g_static_mutex_unlock(&hostinfo_mutex);  
}

const char *
hostinfo_refer_encryption_key_password(void) {

	hostinfo_update_internal_password(HOSTINFO_PASSWD_TYPE_ENCKEY);

	return (pubkey_pass_string != NULL) ? (pubkey_pass_string) : "";
}

int
hostinfo_set_encryption_key_password(const char *encoded_password) {

	if (encoded_password == NULL)
		return -EINVAL;

	hostinfo_register_password(HOSTINFO_PASSWD_TYPE_ENCKEY, encoded_password);

	return 0;
}

const char *
hostinfo_refer_lock_password(void) {

	hostinfo_update_internal_password(HOSTINFO_PASSWD_TYPE_LOCK);

	return (lock_pass_string != NULL) ? (lock_pass_string) : "";
}

int
hostinfo_set_lock_password(const char *encoded_password) {

	if (encoded_password == NULL)
		return -EINVAL;

	hostinfo_register_password(HOSTINFO_PASSWD_TYPE_LOCK, encoded_password);

	return 0;
}

gboolean
hostinfo_refer_ipmsg_iconify_dialogs(void) {

  return gconf_client_get_bool(client, HOSTINFO_KEY_ICONIFY_DIALOGS, NULL);
}

gboolean
hostinfo_set_ipmsg_iconify_dialogs(gboolean val) {

  gconf_client_clear_cache(client);
  return gconf_client_set_bool(client, HOSTINFO_KEY_ICONIFY_DIALOGS, val, NULL);
}

gboolean
hostinfo_refer_ipmsg_log_locked_message_handling(void) {

  return gconf_client_get_bool(client, HOSTINFO_KEY_LOCK_MSGLOG, NULL);
}

gboolean
hostinfo_set_log_locked_message_handling(gboolean val) {

  gconf_client_clear_cache(client);
  return gconf_client_set_bool(client, HOSTINFO_KEY_LOCK_MSGLOG, val, NULL);
}

int
hostinfo_set_encoding(const char *encoding) {

  if (encoding == NULL)
    return -EINVAL;

  g_static_mutex_lock(&hostinfo_mutex);  
  dbg_out("set encoding : %s\n", encoding);

  g_assert(client);

  if (encoding_string != NULL)
    g_free(encoding_string);

  gconf_client_set_string(client, HOSTINFO_KEY_EXTERNAL_ENCODING, 
			  encoding, NULL);
  gconf_client_clear_cache(client);

  encoding_string = 
    gconf_client_get_string(client, HOSTINFO_KEY_EXTERNAL_ENCODING, NULL);

  dbg_out("gconf return encoding:%s\n", encoding_string);
  g_static_mutex_unlock(&hostinfo_mutex);  

  return 0;
}

const char *
hostinfo_refer_encoding(void) {

  g_static_mutex_lock(&hostinfo_mutex);  
  if (encoding_string == NULL) {

    do {

      encoding_string = 
	gconf_client_get_string(client, HOSTINFO_KEY_EXTERNAL_ENCODING, NULL);

      if (encoding_string == NULL) /* 未登録の場合は、デフォルトを登録  */
	hostinfo_set_encoding(IPMSG_PROTO_CODE);

    } while (encoding_string == NULL);

    dbg_out("gconf return encoding name:%s\n", encoding_string);
  }
  g_static_mutex_unlock(&hostinfo_mutex);  

  g_assert(encoding_string != NULL);

  return encoding_string;
}


int
hostinfo_init_hostinfo(void){
  int i;

  client = gconf_client_get_default ();

  gconf_client_add_dir (client,
			PATH,
			GCONF_CLIENT_PRELOAD_NONE,
			NULL);
  /*
   * Add keys
   */
  for(i=0;keys[i];++i) {
    dbg_out("Add %s\n",keys[i]);
    gconf_client_notify_add (client,
			     keys[i],
			     hostinfo_gconf_notify_func,
			     (gpointer)keys[i],
			     NULL,
			     NULL);
  }
  return 0;
}
void
hostinfo_cleanup_hostinfo(void){
  if (groupname)
    g_free(groupname);
  if (nickname)
    g_free(nickname);
  if (hostname)
    g_free(hostname);
  if (cite_string)
    g_free(cite_string);
}

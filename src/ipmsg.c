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

/*
 * Initial main.c file generated by Glade. Edit as required.
 * Glade will not overwrite this file.
 */

#include "common.h"

#define GLADE_HOOKUP_OBJECT(component,widget,name) \
  g_object_set_data_full (G_OBJECT (component), name, \
    gtk_widget_ref (widget), (GDestroyNotify) gtk_widget_unref)

#define GLADE_HOOKUP_OBJECT_NO_REF(component,widget,name) \
  g_object_set_data (G_OBJECT (component), name, widget)

udp_con_t *udp_con;
static udp_con_t con;

void
read_message(gpointer data,
	     gint source,
	     GdkInputCondition condition) {
  int         rc = 0;
  char *msg_buff = NULL;
  size_t     len = 0;
  udp_con_t *con = NULL;

  con = (udp_con_t *) data;
  if (con == NULL)
    return;

  gdk_input_remove(con->target_tag);
  len=0;
  msg_buff=NULL;
  udp_recv_message(udp_con,&msg_buff,&len);
      if (len>0) {
	msg_data_t msg;

	init_message_data(&msg);
	dbg_out("Message arrive\n");
	rc = parse_message(udp_get_peeraddr(udp_con), &msg, msg_buff, len);
	g_free(msg_buff);
	msg_buff=NULL;
	if (rc == 0)
	  ipmsg_dispatch_message(udp_con,&msg);
	release_message_data(&msg);
      }
  dbg_out("Add socket:%d\n",udp_con->soc);
  udp_con->target_tag = gdk_input_add(udp_con->soc,GDK_INPUT_READ,read_message, con);
  
}

int
ipmsg_send_broad_cast(const udp_con_t *con,const char *msg,size_t len){
  if ( (!con) || (!msg) )
    return -EINVAL;

  userdb_send_broad_cast(con,msg,len);
  hostinfo_send_broad_cast(con,msg,len);

  return 0;
}

int 
ipmsg_send_message(const udp_con_t *con, const char *ipaddr, const char *msg,size_t len) {
	int rc = 0;

	if (ipaddr != NULL) {
		/* ユニキャスト */
		rc = udp_send_message(con, ipaddr, hostinfo_refer_ipmsg_port(), 
				      msg, len);
		if (rc >0 )
		  rc = 0;
	} else {
		/* ブロードキャスト */
		userdb_send_broad_cast(con,msg,len);
		hostinfo_send_broad_cast(con,msg,len);
	}

	return rc;
}

int
internal_extend_memory(void **buff_p,size_t new_size,size_t orig_size,gboolean is_clear) {
  void *new_buff;
  void *orig_buff;

  if ( (!buff_p) || (!(*buff_p)) )
    return -EINVAL;

  orig_buff=*buff_p;  
  new_buff=g_malloc(new_size);
  if (!new_buff)
    return -ENOMEM;

  if (is_clear)
    memset(new_buff,0,new_size);

  memmove(new_buff,orig_buff,orig_size);

  g_free(orig_buff);
  *buff_p=new_buff;

  return 0;
}

int
create_lock_file(void){
  int             fd = 0;
  int             rc = 0;

  fd = open(IPMG_LOCK_FILE,O_RDWR|O_EXCL|O_CREAT,(S_IRUSR|S_IWUSR));
  if (fd < 0) {
    fd = open(IPMG_LOCK_FILE, O_RDWR);
    if (fd < 0) {
      rc = -errno;
      goto error_out;
    }
  }

  rc = flock(fd, LOCK_EX|LOCK_NB);
  if (rc < 0) {
    rc = -errno;
    goto error_out;
  }

  rc = 0;

 error_out:
  if (rc < 0) {
    ipmsg_err_dialog_mordal("%s:%s %s : %d (%s)\n", 
		     _("Can not setup lock file"), IPMG_LOCK_FILE, 
			_("errno"), rc, strerror(errno));
  }

 success_out:
  return rc;
}
int
release_lock_file(void){
  int fd;
  int rc;

  fd=open(IPMG_LOCK_FILE,O_RDWR);
  if (fd<0)
    return -errno;
  rc=flock(fd,LOCK_UN);
  if (rc<0)
    return -errno;
  rc=unlink(IPMG_LOCK_FILE);
  if (rc<0)
    return -errno;

  return 0;
}
int
init_ipmsg(void){
  int rc;
  char *cwd=NULL;

  rc=get_envval("HOME",&cwd);  
  if (!rc)
    rc=chdir(cwd);
  else {
    rc=get_envval("TMPDIR",&cwd);  
    if (!rc)
      rc=chdir(cwd);
    else
      rc=chdir("/tmp");
  }

  if (rc) {
    rc=-errno;
    goto error_out;
  }
  userdb_init_userdb();
  logfile_init_logfile();
  init_message_info_manager();
  init_sound_system(PACKAGE);

#if defined(USE_OPENSSL)
  pcrypt_crypt_init_keys();
#endif  /*  USE_OPENSSL  */

  //udp_server_setup(hostinfo_refer_ipmsg_port(), PF_UNSPEC);

  memset(&con,0,sizeof(udp_con_t));
  rc=udp_setup_server(&con,hostinfo_refer_ipmsg_port(),hostinfo_get_ipmsg_system_addr_family());
  if (rc<0)
    goto error_out;

  rc=udp_enable_broadcast(&con);
  if (rc<0)
    goto error_out;

  udp_con=&con;

  dbg_out("Add socket:%d\n", udp_con->soc);
  udp_con->target_tag = 
    gdk_input_add(udp_con->soc, GDK_INPUT_READ, read_message, udp_con);
  ipmsg_send_br_entry(udp_con,0);

  rc=0;

 error_out:
  if (cwd)
    g_free(cwd);

  return rc;
}

void
cleanup_ipmsg(void){
  ipmsg_send_br_exit(udp_con,hostinfo_get_normal_send_flags());
  udp_release_connection(udp_con);
  logfile_shutdown_logfile();
  dbg_out("UI Thread ended\n");
  cleanup_sound_system();
#if defined(USE_OPENSSL)
  pcrypt_crypt_release_keys();
#endif  /*  USE_OPENSSL  */
  userdb_cleanup_userdb();
  hostinfo_cleanup_hostinfo();
}


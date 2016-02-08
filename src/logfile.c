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

static int handle=-1;
GStaticMutex logfile_mutex = G_STATIC_MUTEX_INIT;
static int
open_log_file(const char *filepath,int *new_fd) {
  int fd;

  if ( (!filepath) || (!new_fd) )
    return -EINVAL;

  g_assert(filepath);
  fd=open(filepath,O_APPEND|O_CREAT|O_WRONLY,S_IRUSR|S_IWUSR);
  if (fd < 0) {
    err_out("open fail:[%s] %s(%d)\n",filepath,strerror(errno),errno);
    return -errno;
  }
  
  *new_fd=fd;
  return 0;
}
static int
internal_reopen_logfile(const char *filepath){
  int new_fd;
  int rc;
  
  if (g_static_mutex_trylock(&logfile_mutex)) {
    g_static_mutex_unlock(&logfile_mutex);
    g_assert_not_reached(); /* ロックチェック失敗 */
  }

  g_assert(filepath);

  rc=open_log_file(filepath,&new_fd);

  if (rc<0)
    return rc;

  if (handle>0) {

    rc=0;
    if (close(handle)<0) {
      rc=-errno;
      close(new_fd);
      return rc;
    }
  }
  dbg_out("Change log into %s(fd=%d)\n",filepath,new_fd);
  handle=new_fd;

  return 0;
}
int 
logfile_init_logfile(void){
  const gchar *filepath;
  int new_fd;
  int rc;

  filepath=hostinfo_refer_ipmsg_logfile();
  if (!filepath)
    return -EINVAL;
  if (handle>0)
    return -EEXIST;

  rc = open_log_file(filepath,&new_fd);

  if(rc == 0){
    dbg_out("Init log into %s(fd=%d)\n",filepath,new_fd);
    handle=new_fd;
  }

  return 0;
}
int
logfile_reopen_logfile(const char *filepath){
  int new_fd;
  int rc;
  
  if (!filepath)
    return -EINVAL;

  g_static_mutex_lock(&logfile_mutex);
  rc=internal_reopen_logfile(filepath);
  g_static_mutex_unlock(&logfile_mutex);

  return rc;
}
/*
 *この関数はログファイルロックを獲得してから呼び出すこと.
 */
static int 
confirm_file_existence(void){
  char *fpath;
  struct stat buf;
  struct stat fd_buf;
  int rc;
  int rc_fd;
  int new_fd;

  if (g_static_mutex_trylock(&logfile_mutex)) {
    g_static_mutex_unlock(&logfile_mutex);
    g_assert_not_reached(); /* ロックチェック失敗 */
  }

  fpath=(char *)hostinfo_refer_ipmsg_logfile();
  if (!fpath)
    return -EINVAL;

  memset(&buf,0,sizeof(buf));
  memset(&fd_buf,0,sizeof(fd_buf));
  rc=stat(fpath,&buf);
  rc_fd=fstat(handle, &fd_buf);
  if ( (rc<0) || (rc_fd<0) || (buf.st_ino != fd_buf.st_ino) || (buf.st_dev != fd_buf.st_dev) ) {
    /*  ファイルが消された場合 再度オープンする*/
    /*  ファイル記述子を握っているので,
     *  ファイルを移動/削除して同じ名前のファイルを作成した場合は,
     *  別のinodeになるので異常を検出するはず.
     */
    dbg_out("Stat fail:%s  so force reopen:%s (%d)\n",
	    fpath,
	    strerror(errno),
	    errno);

    rc=internal_reopen_logfile(fpath);
  }

  return rc;
}

/*
 *この関数はログファイルロックを獲得してから呼び出すこと.
 */
static int 
write_one_line(const char *string) {
  int rc;
  size_t len;

  if (!string)
    return -EINVAL;

  if (handle<0)
    return -ENOENT;

  if (g_static_mutex_trylock(&logfile_mutex)) {
    g_static_mutex_unlock(&logfile_mutex);
    g_assert_not_reached(); /* ロックチェック失敗 */
  }

  confirm_file_existence();
  len=strlen(string);
  rc=write(handle,string,len);
  if (rc<0) {
    dbg_out("write fail:%s(%d)\n",strerror(errno),errno);
    goto error_out;
  }
  if  (len != rc) {
    dbg_out("Partial write: len=%d rc=%d\n",len,rc);
    goto error_out;
  }

  len=strlen(LOGFILE_NEW_LINE);
  rc=write(handle,LOGFILE_NEW_LINE,len);
  if (rc<0) {
    dbg_out("write fail:%s(%d)\n",strerror(errno),errno);
    goto error_out;
  }
  if  (len != rc) {
    dbg_out("Partial write: len=%d rc=%d\n",len,rc);
    goto error_out;
  }
  rc=0;
 error_out:
  return rc;
}
int 
logfile_write_log(const char *direction,const char *ipaddr,const char *message){
  char buffer[LOGFILE_MAX_LINE_LEN];
  char logname[64];
  char loged_ipaddr[64];
  userdb_t *user_info=NULL;
  int rc;
  struct timeval tv;

  g_assert(direction);

  if ( (!ipaddr) || (!message) )
    return -EINVAL;

  dbg_out("logging: direction: %s addr : %s message:%s\n",
	  direction,
	  ipaddr,
	  message);

  rc=userdb_search_user_by_addr(ipaddr,(const userdb_t **)&user_info);
  if (rc)
    return rc;

  if (handle<0)
    return -ENOENT;

  rc=flock(handle,LOCK_EX|LOCK_NB);
  if (rc<0)
    return -errno;

  rc=lseek(handle, 0,SEEK_END);
  if (rc<0)
    return -errno;

  logname[0]=loged_ipaddr[0]='\0';

  if (hostinfo_refer_ipmsg_logname_logging())
    snprintf(logname,63,"[%s]",user_info->user);

  if (hostinfo_refer_ipmsg_ipaddr_logging())
    snprintf(loged_ipaddr,63,"/%s",ipaddr);

  logname[64-1]=loged_ipaddr[64-1]='\0';

  g_static_mutex_lock(&logfile_mutex);
  write_one_line(LOGFILE_START_HEADER);
  snprintf(buffer,LOGFILE_MAX_LINE_LEN-1,LOGFILE_FMT,
	   direction,
	   user_info->nickname,
	   logname,
	   user_info->group,
	   user_info->host,
	   loged_ipaddr);
  write_one_line(buffer);
  gettimeofday(&tv,NULL);
  ctime_r(&(tv.tv_sec),buffer);
  write_one_line(buffer);
  write_one_line(LOGFILE_END_HEADER);

  write_one_line(message);
  write_one_line("");
  rc=0;
  g_static_mutex_unlock(&logfile_mutex);
  destroy_user_info(user_info);
  
  rc=flock(handle,LOCK_UN);
  if (rc<0)
    return -errno;

  return rc;
}
int 
logfile_send_log(const char *ipaddr, const ipmsg_send_flags_t flags, const char *message){

  dbg_out("send log: addr : %s message:%s\n",
	  ipaddr,
	  message);

  if (!hostinfo_refer_ipmsg_enable_log())
    return -ENOENT;

  return logfile_write_log(LOGFILE_TO_STR,ipaddr,message);
}
int 
logfile_recv_log(const char *ipaddr, const ipmsg_send_flags_t flags, const char *message) {
  int rc = 0;

  dbg_out("recv log: addr : %s message:%s\n", ipaddr,  message);

  if (!hostinfo_refer_ipmsg_enable_log())
    return -ENOENT;

  /*
   * 施錠に関する処理
   */
  if  ( ( flags & IPMSG_PASSWORDOPT ) && 
	( hostinfo_refer_ipmsg_log_locked_message_handling() )  ) {
    return -EPERM; /* 施錠付きの場合, 後で開封時にロギングする  */
  }

  rc = logfile_write_log(LOGFILE_FROM_STR, ipaddr, message);

  return rc;
}
int 
logfile_shutdown_logfile(void){
  if (handle>0)
    close(handle);
  return 0;
}

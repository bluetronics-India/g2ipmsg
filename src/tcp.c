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

static int
destroy_tcp_connection(tcp_con_t *con) {

  dbg_out("Here\n");
  if (!con)
    return -EINVAL;

  if (con->peer_info)
    freeaddrinfo(con->peer_info);
  con->peer_info=NULL;

  close(con->soc);

  return 0;
}

static int
parse_request(request_msg_t *req,const char *request_string){
  size_t len;
  long int_val;
  char *sp=NULL;
  char *ep=NULL;
  char *buffer;
  size_t remains;
  gboolean has_offset=FALSE;
  int rc=0;

  if  ( (!request_string) || (!req) )
    return -EINVAL;

  dbg_out("Here:%s\n",request_string);

  buffer=g_strdup(request_string);
  if (!buffer)
    return -ENOMEM;

  len=strlen(request_string);
  _assert(len>0);
  remains=len;
  /*
   * パケット番号
   */
  sp=buffer;
  ep=memchr(sp, ':', remains);
  if (!ep) {
    rc=-EINVAL;
    goto error_out;
  }

  *ep='\0';
  remains =len - ((unsigned long)ep-(unsigned long)buffer);
  if (remains<=0) {
    rc=-EINVAL;
    goto error_out;
  }
  ++ep;
  int_val=strtol(sp, (char **)NULL, 16);
  req->pkt_no=int_val;
  dbg_out("pktno:%ld\n",req->pkt_no);
  sp=ep;

  /*
   * ファイルID
   */
  ep=memchr(sp, ':', remains);
  if (ep) {
    has_offset=TRUE;
    *ep='\0';
    remains =len - ((unsigned long)ep-(unsigned long)buffer);
    if (remains<=0) {
      rc=-EINVAL;
      goto error_out;
    }
    ++ep;
  }
  int_val=strtol(sp, (char **)NULL, 16);
  req->fileid=int_val;
  dbg_out("fileid:%d(%x)\n",req->fileid,req->fileid);
  sp=ep;

  /*
   * オフセット
   */
  if (has_offset)
    int_val=strtol(sp, (char **)NULL, 16);
  else
    int_val=0; /* 仮に0にする */
  req->offset=int_val;
  dbg_out("offset:%d(%x)\n",req->offset,req->offset);
error_out:
  g_free(buffer);
  return rc;
}
static int
tcp_ipmsg_finalize_connection(tcp_con_t *con){
  int rc=0;
  char dummy[1];

  g_assert(con);
  sock_set_buffer(con->soc, _TCP_BUF_MIN_SIZE, _TCP_BUF_SIZE);
  dbg_out("Wait for closing by peer.\n");
  sock_set_recv_timeout(con->soc,TCP_CLOSE_WAIT_SEC);
  rc=wait_socket(con->soc,WAIT_FOR_READ,TCP_SELECT_SEC);
  if (rc<0) {
    dbg_out("OK timeout select :%s(%d)\n",strerror(errno),errno);
    return -errno;
  }
  memset(dummy, 0, sizeof(dummy));
  rc=recv(con->soc, dummy,sizeof(dummy),0); /* 相手のクローズ検出 */

  if (rc<0)
    dbg_out("OK timeout:%s(%d)\n",strerror(errno),errno);
  else
    rc=0;

  return rc;
}
static int
tcp_transfer_file(tcp_con_t *con,const char *path,const off_t size, off_t offset){
  int fd;
  int rc;
  char *buff = NULL;
  off_t read_len;
  off_t file_remains;
  off_t soc_remains;
  off_t write_len;
  off_t total_write;
  char *wp;
  struct sigaction saved_act;

  if ( (!con) || (!path) )
    return -EINVAL;

  buff = g_malloc(TCP_FILE_BUFSIZ);
  if (buff == NULL)
    return -ENOMEM;

  fd=open(path,O_RDONLY);
  if (fd<0)
    return -errno;

  rc=lseek(fd, offset, SEEK_SET);
  if (rc<0) {
    rc=-errno;
    goto close_out;
  }
  total_write=0;
  file_remains=size;

  while(file_remains>0) {
    read_len=read(fd,buff,TCP_FILE_BUFSIZ);
    if (read_len<0) {
      rc=-errno;
      err_out("Can not read file %s %d\n",strerror(errno),errno);
      goto close_out;
    }
    file_remains -= read_len;
    soc_remains = read_len;
    wp = buff;

    if (wait_socket(con->soc,WAIT_FOR_WRITE,TCP_SELECT_SEC)<0) {
      err_out("Can not send socket\n");
      goto close_out;
    }

    dbg_out("sock remains:%d\n",soc_remains);
    disable_pipe_signal(&saved_act);
    while(soc_remains > 0) {
      write_len = write(con->soc, wp, soc_remains);
      if (write_len<0) {
	if (errno==EINTR)
	  continue;
	err_out("Can not send %s %d\n",strerror(errno),errno);
	enable_pipe_signal(&saved_act);
	goto close_out;
      }
      dbg_out("write len :%d\n",write_len);
      wp += write_len;
      total_write += write_len;
      soc_remains -= write_len;
    }
    enable_pipe_signal(&saved_act);
    dbg_out("transfer %s %d/%d(%d)\n",path,total_write,size,file_remains);
  }
  rc=0;
 close_out:
  close(fd);
  if (rc<0)
    dbg_out("Can not send file:%s %s %d\n",path,strerror(errno),errno);
  if (buff != NULL)
    g_free(buff);
  return rc;
}
static int
create_response(unsigned long type,const char *name,const size_t size,const char *dir,char **response){
  int rc=0;
  char *buff=NULL;
  size_t act_len;
  gchar *res;
  size_t local_size;
  int msg_fid;
  char *local_name_p;

  dbg_out("here:\n");
  if ( ( (!name) && (type != IPMSG_FILE_RETPARENT) ) || (!dir) || (!response) )
    return -EINVAL;
  if (type != IPMSG_FILE_RETPARENT)
    dbg_out("file:%s size:%d(%x)\n",
	  name,size,size);
  else
    dbg_out("Return to parent.\n");

  switch(type)
    {
    case IPMSG_FILE_REGULAR:
      msg_fid=1;
      local_size=size;
      local_name_p=(char *)name;
      break;
    case IPMSG_FILE_DIR:
      msg_fid=2;
      local_size=0;
      local_name_p=(char *)name;
      break;
    case IPMSG_FILE_RETPARENT:
      msg_fid=3;
      local_size=0;
      local_name_p=".";
      break;
    default:
      dbg_out("Invalid type:%d\n",type);
      return -EINVAL;
      break;
  }
  /*
   *メッセージ形成
   */
  buff=g_malloc(IPMSG_BUFSIZ);
  if (!buff)
    return -ENOMEM;
  snprintf(buff,IPMSG_BUFSIZ-1,"xxxx:%s:%x:%d:",
	   local_name_p,local_size,msg_fid); /* 仮の長さを算出  */
  buff[IPMSG_BUFSIZ-1]='\0';
  act_len=strlen(buff);
  snprintf(buff,IPMSG_BUFSIZ-1,"%04x:%s:%x:%d:",
	   act_len,local_name_p,local_size,msg_fid); /* メッセージ作成  */
  buff[IPMSG_BUFSIZ-1]='\0';

  rc=-ENOSPC;
  if (act_len != strlen(buff)) {
    err_out("Message too long:%s\n",buff);
    goto error_out;
  }

  *response = buff;

  rc=0;

  goto no_free_out;

 error_out:
  if (buff)
    g_free(buff);
 no_free_out:
  return rc;
}

static const char *
tcp_get_peeraddr(const tcp_con_t *con) {
  int rc = 0;
  struct addrinfo *info = NULL;

  g_assert(con);

  info=con->peer_info;
  g_assert(info);

  rc = netcommon_get_peeraddr(info, (char *)con->peer);

  return (const char *)con->peer;
}

static int
send_header(tcp_con_t *con,const char *response){
  int rc;
  size_t soc_remains;
  ssize_t send_len;
  char *wp;
  int buff_len;
  const char *peer_addr=NULL;
  struct sigaction saved_act;
  gchar *send_buffer = NULL;

  if ( (!con) || (!response) )
    return -EINVAL;

  peer_addr = tcp_get_peeraddr(con);

  rc = ipmsg_convert_string_external(peer_addr, response, (const char **)&send_buffer); 
  if (rc != 0) {
    ipmsg_err_dialog("%s\n", 
		     _("Can not convert header into external representation"));
    goto no_free_out;
  }

  soc_remains = strlen(send_buffer);
  if (wait_socket(con->soc, WAIT_FOR_WRITE, TCP_SELECT_SEC) < 0) {
    err_out("Can not send socket\n");
    goto error_out;
  }

  wp=(char *)send_buffer;
  dbg_out("Send:%s(%d, %x)\n",wp,soc_remains,soc_remains);
  while(soc_remains>0) {
    disable_pipe_signal(&saved_act);
    send_len=send(con->soc,wp,soc_remains,0);
    enable_pipe_signal(&saved_act);
    if (send_len<0) {
      if (errno == EINTR)
	continue;
      rc=-errno;
      err_out("Error:%s (%d)\n",strerror(errno),errno);
      goto error_out;
    }
    wp += send_len;
    soc_remains -= send_len;
    dbg_out("soc remains:%d\n",soc_remains);
  }

  rc=0;

 error_out:
  if (send_buffer != NULL)
    g_free(send_buffer);

 no_free_out:
  return rc; 
}
static int
read_directory_files(tcp_con_t *con,const char *parent,GnomeVFSFileInfo *info) {
  int rc;
  char *rel_path;
  char *full_path;
  char *res_message;

  if ( (!con) || (!parent) || (!info) )
    return -EINVAL;

  if ( (!strcmp("..",info->name)) || (!strcmp(".",info->name) ) )
    return -EINVAL;

  if (info->type==GNOME_VFS_FILE_TYPE_REGULAR) {
    rc=create_response(IPMSG_FILE_REGULAR,info->name,info->size,parent,&res_message);
    if (rc<0)
      return rc;
    dbg_out("Send file:%s\n",res_message);
    rc=send_header(con,res_message);
    g_free(res_message);
    if (rc<0)
      return rc;
  }

  rc=-ENOMEM;
  full_path=g_build_filename(parent,info->name,NULL);
  if (!full_path) 
    return rc;
  /* send file */
  tcp_transfer_file(con,full_path,info->size,0);
  g_free(full_path);
  return 0;
}
static int
send_directory(tcp_con_t *con,const char *top_dir,const char *basename,GnomeVFSFileInfo *info){
  int rc;
  char *uri;
  GnomeVFSResult res;
  GnomeVFSDirectoryHandle *handle;
  char *next_dir;
  char *res_message;

  if ( (!top_dir) || (!info) || (!basename) )
    return -EINVAL;

  rc=create_response(IPMSG_FILE_DIR,info->name,info->size,top_dir,&res_message);
  
  if (rc<0)
    return rc;
  dbg_out("Send dir:%s (%s)\n",res_message,top_dir);
  rc=send_header(con,res_message);
  g_free(res_message);
  if (rc<0)
    return rc;

      
  uri=gnome_vfs_get_uri_from_local_path(top_dir);
  if (!uri)
    return -ENOMEM;
  /*
   *ファイルを送付
   */
  res=gnome_vfs_directory_open(&handle,uri,GNOME_VFS_FILE_INFO_FOLLOW_LINKS);
  res=gnome_vfs_directory_read_next(handle,info);
  while (res==GNOME_VFS_OK) {
    read_directory_files(con,top_dir,info);
    res=gnome_vfs_directory_read_next(handle,info);
  }
  gnome_vfs_directory_close(handle); 
  /*
   *ディレクトリを送付
   */
  res=gnome_vfs_directory_open(&handle,uri,GNOME_VFS_FILE_INFO_FOLLOW_LINKS);
  if (res!=GNOME_VFS_OK) {
    err_out("Can not open dir:%s %s (%d)\n",
	    top_dir,
	    gnome_vfs_result_to_string(res),
	    res);
    goto error_out;
  }
  res=gnome_vfs_directory_read_next(handle,info);
  if (res!=GNOME_VFS_OK) {
    if (res != GNOME_VFS_ERROR_EOF)
      err_out("Can not read next dir:%s %s (%d)\n",
	      top_dir,
	      gnome_vfs_result_to_string(res),
	      res);
    goto error_out;
  }
  while (res==GNOME_VFS_OK) {
    if ( (info->type==GNOME_VFS_FILE_TYPE_DIRECTORY) &&
	 ( (strcmp(info->name,"..")) && (strcmp(info->name,"."))) ){
      dbg_out("dir:%s\n",info->name);
      next_dir=g_build_filename(top_dir,info->name,NULL);
      if (!next_dir)
	goto error_out;
      rc=send_directory(con,next_dir,info->name,info);
      g_free(next_dir);
      if (rc<0)
	return rc;
      rc=create_response(IPMSG_FILE_RETPARENT,info->name,info->size,top_dir,&res_message);
      if (rc<0)
	return rc;
      dbg_out("Send ret parent:%s\n",res_message);
      rc=send_header(con,res_message);
      g_free(res_message);
      if (rc<0)
	return rc;
    }
    res=gnome_vfs_directory_read_next(handle,info);
    if (res!=GNOME_VFS_OK) {
      if (res != GNOME_VFS_ERROR_EOF)
	err_out("Can not read next dir:%s %s (%d)\n",
		top_dir,
		gnome_vfs_result_to_string(res),
		res);
      goto error_out;
    }
  }

 error_out:
  gnome_vfs_directory_close(handle); 
  g_free(uri);
  return rc;
}
static int
finalize_send_directory(tcp_con_t *con,const char *top_dir,GnomeVFSFileInfo *info){
  int rc;
  char *res_message;

  if ( (!top_dir) || (!info) )
    return -EINVAL;
  dbg_out("Here\n",res_message,top_dir);
  rc=create_response(IPMSG_FILE_RETPARENT,info->name,info->size,top_dir,&res_message);
  if (rc<0) {
    err_out("finalizesend directory fail %s (%d)\n",
	    strerror(-rc),rc);
    return rc;
  }
    
  dbg_out("Send return to top dir:%s (%s)\n",res_message,top_dir);
  rc=send_header(con,res_message);
  g_free(res_message);

  return rc;
}


static int
tcp_transfer_dir(tcp_con_t *con,const char *path){
  int rc;
  char *basename;
  GnomeVFSFileInfo *info;
  GnomeVFSResult res;
  gchar *dir_uri;

  if ( (!con) || (!path) )
    return -EINVAL;
  info=gnome_vfs_file_info_new();
  dir_uri=gnome_vfs_get_uri_from_local_path(path);

  rc=-ENOMEM;
  if (!dir_uri) 
    goto free_info_out;

  res=gnome_vfs_get_file_info(dir_uri,info,GNOME_VFS_FILE_INFO_FOLLOW_LINKS);
  if (res != GNOME_VFS_OK) {
    rc=-res;
    goto free_info_out;
  }
  basename=g_path_get_basename(path);
    rc=-ENOMEM;
  if (!basename)
    goto free_info_out;

  rc=send_directory(con,path,basename,info);
  g_free(basename);
  if (!rc)
    rc=finalize_send_directory(con,path,info);
  else
    err_out("Send directory fail %s (%d)\n",
	    strerror(-rc),rc);
 free_info_out:
  gnome_vfs_file_info_unref(info);
  return rc;
}
int
disable_pipe_signal(struct sigaction *saved_act){
  int rc;
  struct sigaction new_act;
  
  if (!saved_act)
    return -EINVAL;

  memset(&new_act,0,sizeof(struct sigaction));

  new_act.sa_handler=SIG_IGN;
  rc=sigaction(SIGPIPE,&new_act,saved_act);
  g_assert(!rc);
  
  return rc;
}
int
enable_pipe_signal(struct sigaction *saved_act){
  int rc;
  
  if (!saved_act)
    return -EINVAL;

  rc=sigaction(SIGPIPE,saved_act,NULL);
  g_assert(!rc);
  
  return rc;
  
}
int
tcp_enable_keepalive(const tcp_con_t *con){
  int rc;
  int flag;

  if (!con) 
    return -EINVAL;

  flag=1;
  rc=setsockopt(con->soc, SOL_SOCKET, SO_KEEPALIVE, (void*)&flag, sizeof(int));
  if (rc<0) {
    err_out("Can not set keepalive:%s(%d)\n",strerror(errno),errno);
    return -errno;
  }

  return 0;
}

int
tcp_setup_client(int family,const char *ipaddr,int port,tcp_con_t *con){
  int rc;
  int sock=-1;
  struct sockaddr_in server;
  struct hostent *host;
  struct addrinfo *info=NULL;

  if ( (!ipaddr) || (!con) )
    return -EINVAL;

  rc=setup_addr_info(&info, ipaddr,port,SOCK_STREAM,family);
  if (rc<0)
    return rc;

  sock=socket(info->ai_family, info->ai_socktype,info->ai_protocol);

  if (sock<0) {
    err_out("Can not create socket: %s(errno:%d)\n",strerror(errno),errno);
    rc=-errno;
    goto error_out;    
  }

#ifdef IPV6_V6ONLY
  if (info->ai_family == AF_INET6) {
    int v6only=1;

    rc=setsockopt(sock,IPPROTO_IPV6,IPV6_V6ONLY,(void *)&v6only,sizeof(v6only));
    if (rc) {
      goto error_out;
    }
  }
#endif /*  IPV6_V6ONLY  */

  if (connect(sock, info->ai_addr, info->ai_addrlen)<0){
    err_out("Can not connect addr:%s port:%d %s (errno:%d)\n",
	    ipaddr,
	    port,
	    strerror(errno),
	    errno);
    rc=-errno;
    goto error_out;    
  }

  rc =  sock_set_buffer(sock, _TCP_BUF_MIN_SIZE, _TCP_BUF_SIZE);
  if (rc<0) {
    err_out("Can not set socket buffer: %s (%d)\n",strerror(errno),errno);
    return -errno;
  }
  /* 受信時はタイムアウト待ちを行うのでここでは, ノンブロックにしないこと  */
  con->soc=sock;
  con->family=info->ai_family;
  con->peer_info=info;

  return 0;
 error_out:
  if (info)
    freeaddrinfo(info);
  return rc;
}

int
tcp_set_buffer(tcp_con_t *con) {
  if (!con)
    return -EINVAL;

  return sock_set_buffer(con->soc, _TCP_BUF_MIN_SIZE, _TCP_BUF_SIZE);
}
gpointer
ipmsg_tcp_recv_thread(gpointer data){
  ssize_t recv_len;
  size_t addr_len;
  int rc;
  tcp_con_t *con;
  char *recv_buf=NULL;
  int count=200;
  struct addrinfo *info;

  con=(tcp_con_t *)data;
  if (!con) {
    err_out("No connection recived\n");
    g_assert(con);
    return NULL;
  }
  if (!(con->peer_info)) {
    err_out("Invalid connection recived\n");
    g_assert(con->peer_info);
    return NULL;
  }
  
  rc=tcp_enable_keepalive(con);
  if (rc<0)
    return NULL;

  recv_buf=g_malloc(_MSG_BUF_SIZE);
  if (!recv_buf)
    return NULL;

  memset(recv_buf,0,_MSG_BUF_SIZE);

  while(1) {
    if (wait_socket(con->soc,WAIT_FOR_READ,TCP_SELECT_SEC)<0) {
      err_out("Can not send socket\n");
      destroy_tcp_connection(con);
      goto error_out;
    }else{
    read_retry:
      errno=0;
      memset(recv_buf, 0, sizeof(recv_buf));
      info=con->peer_info;
      recv_len=recv(con->soc,
		    recv_buf,
		    _MSG_BUF_SIZE,
		    MSG_PEEK);

      if (recv_len<=0) {
	if (errno==EINTR)
	  goto read_retry;
	if (errno)
	  err_out("Can not peek message %s(errno:%d)\n",strerror(errno),errno);
	destroy_tcp_connection(con);
	goto error_out;
      }

      dbg_out("tcp peek read:%d %s\n",recv_len,recv_buf);

      recv_len=recv(con->soc,
		    recv_buf,
		    recv_len,
		    MSG_PEEK);

      dbg_out("tcp read:%d %s\n",recv_len,recv_buf);
      if (recv_len>0) {
	msg_data_t msg;
	request_msg_t req;
	char *path;
	off_t size;
	unsigned long ipmsg_fattr;

	recv_buf[recv_len-1]='\0';
	memset(&req,0,sizeof(request_msg_t));

	init_message_data(&msg);
	parse_message(NULL,&msg,recv_buf,recv_len);
	parse_request(&req,msg.message);
	rc=refer_attach_file(req.pkt_no,req.fileid,&ipmsg_fattr,(const char **)&path,&size);
	if (rc<0) {
	  err_out("Can not find message:pktno %ld id : %d\n",
		  req.pkt_no,
		  req.fileid);
	  close(con->soc); /* エラーとしてクローズする  */
	}else{
	  dbg_out("transfer:%s (%d)\n",path,size);
	  switch(ipmsg_fattr) 
	    {
	    case IPMSG_FILE_REGULAR:
	      if (!tcp_transfer_file(con,path,size,0)) {
		tcp_ipmsg_finalize_connection(con);
		download_monitor_release_file(req.pkt_no,req.fileid);
	      }
	      break;
	    case IPMSG_FILE_DIR:
	      if (!tcp_transfer_dir(con,path)){
		download_monitor_release_file(req.pkt_no,req.fileid);
		tcp_ipmsg_finalize_connection(con);
	      }
	      break;
	    default:
	      break;
	    }
	  release_message_data(&msg);
	  g_free(path);
	  break;
	}
	release_message_data(&msg);
	break;
      }
    }
    --count;
    if (!count)
      break;
  }
  if (con->peer_info) /* closeは相手側で行うので, destroy_tcp_connectionは呼び出せない
		  * (Win版ipmsgの仕様).  
		  */
    freeaddrinfo(con->peer_info); 
 error_out:
  g_free(con);
  if (recv_buf)
    g_free(recv_buf);

  return NULL;
}
gpointer 
ipmsg_tcp_server_thread(gpointer data){
  int rc;
  size_t len;
  int sock;
  int tcp_socket;
  int port;
  int reuse;
  int family;
  tcp_con_t *client_con=NULL;
  struct addrinfo *info=NULL;

  family=(int)data;
  rc=setup_addr_info(&info, 
		     NULL,
		     hostinfo_refer_ipmsg_port(),
		     SOCK_STREAM,family);
  if (rc<0)
    return NULL;

  tcp_socket = socket(info->ai_family,
		      info->ai_socktype,
		      info->ai_protocol);

  if (tcp_socket < 0) {
    err_out("Can not create socket:%s (%d)\n",strerror(errno),errno);
    goto error_out;
  }

  reuse=1;
  if (setsockopt(tcp_socket,SOL_SOCKET, SO_REUSEADDR, (const char *)&reuse, sizeof(reuse))<0) {
    err_out("Can not set sock opt:%s (%d)\n",strerror(errno),errno);
    goto error_out;
  }

#ifdef IPV6_V6ONLY
  if (info->ai_family == AF_INET6) {
    int v6only=1;

    rc=setsockopt(tcp_socket,IPPROTO_IPV6,IPV6_V6ONLY,(void *)&v6only,sizeof(v6only));
    if (rc) {
      goto error_out;
    }
  }
#endif /*  IPV6_V6ONLY  */

  if (bind(tcp_socket, info->ai_addr, info->ai_addrlen) != 0) {
    err_out("Can not bind socket:%s (%d)\n",strerror(errno),errno);
    goto error_out;
  }


  if (sock_set_buffer(tcp_socket, _TCP_BUF_MIN_SIZE, _TCP_BUF_SIZE)<0) {
    err_out("Can not set socket buffer:%s (%d)\n",strerror(errno),errno);
    goto error_out;
  }

  if (listen(tcp_socket, TCP_LISTEN_LEN) != 0) {
    err_out("Can not listen socket:%s (%d)\n",strerror(errno),errno);
    goto error_out;
  }

  while (1) {
    GThread *handler;
    struct addrinfo *client_info=NULL;

    client_con=g_malloc(sizeof(tcp_con_t));
    g_assert(client_con);
    dbg_out("accept\n");

    
    rc=setup_addr_info(&client_info, 
		     NULL,
		     hostinfo_refer_ipmsg_port(),
		     SOCK_STREAM,family);
    
    if (rc<0)
      goto free_client_out;
    
    sock = accept(tcp_socket, client_info->ai_addr,&(client_info->ai_addrlen));
    if (sock < 0) {
      err_out("Can not accept:%s (%d)\n",strerror(errno),errno);
      goto free_client_out;
    }
    if (sock_set_buffer(sock, _TCP_BUF_MIN_SIZE, _TCP_BUF_SIZE)<0) {
      err_out("Can not set socket buffer:%s (%d)\n",strerror(errno),errno);
      goto free_client_out;
    }

    client_con->soc=sock;
    client_con->peer_info=client_info;
    handler=g_thread_create(ipmsg_tcp_recv_thread,
                            client_con,
			    FALSE,
			    NULL);
    if (!handler) {
      err_out("Can not create handler thread.\n");
      goto free_client_out;
    }
  }
  /* ここには, 来ないはずだが, 念のため  */
  if (info)
    freeaddrinfo(info);

  return NULL;

 free_client_out:  
  destroy_tcp_connection(client_con);
 error_out:
  if (info)
    freeaddrinfo(info);
  exit(0); /* 多重起動のため 終了  */
  return NULL;
}

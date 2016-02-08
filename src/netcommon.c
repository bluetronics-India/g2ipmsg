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
internal_set_buffer(const int soc,int type,int max,int min,int *actual){
  int   rc = 0;
  int size = 0;

  if ( (soc < 0) || (actual == NULL) )
    return -EINVAL;
  if ( (type != SO_RCVBUF) && (type != SO_SNDBUF) )
    return -EINVAL;

  size = max;
  rc = setsockopt(soc, SOL_SOCKET, type, (void*)&size, sizeof(int));
  if (rc < 0) {
    size = min;
    rc = setsockopt(soc, SOL_SOCKET, type, (void*)&size, sizeof(int));
    if (rc < 0) {
      ipmsg_err_dialog("%s:(min, max) = (%d, %d) %s(%d)\n",
		       _("Can not set buffer size"), min, max, strerror(errno), errno);
      *actual = size;
      return -errno;
    }
  }
  *actual = size;
  return 0;
}
int
sock_set_buffer(int soc, unsigned long min_buf, unsigned long max_buf){
  int rc = 0;
  int size=0;

  if (soc<0)
    return -EINVAL;

  rc = internal_set_buffer(soc,SO_RCVBUF,max_buf,min_buf,&size);
  if (size < _MSG_BUF_MIN_SIZE) {
    dbg_out("Can not set recv buf size:%s (%d)\n",
	    strerror(-rc),
	    -rc);
    return rc;
  }
  size = 0;
  rc = internal_set_buffer(soc,SO_SNDBUF, max_buf, min_buf,&size);
  if (size < _MSG_BUF_MIN_SIZE) {
    dbg_out("Can not set send buf size:%s (%d)\n",
	    strerror(-rc),
	    -rc);
    return rc;
  }
  dbg_out("Set buffer size:%lu\n",size);
  return 0;
}
static int
internal_set_timeout(const int soc,int type,unsigned long msec){
  int rc;
  int size;
  struct timeval tmout;
  int sec;
  int usec;

  if (soc<0)
    return -EINVAL;

  if ( (type != SO_RCVTIMEO) && (type != SO_SNDTIMEO) )
    return -EINVAL;

  sec=msec/1000;
  usec=( (msec % 1000) * 1000 );
  tmout.tv_sec=sec;
  tmout.tv_usec=usec;

  dbg_out("Try to set %d socket for %s timeout %d sec %d msec\n",
	  soc,
	  (type == SO_RCVTIMEO) ? ("Receive") : ("Send"),
	  sec,
	  (usec/1000) );

  rc=setsockopt(soc, SOL_SOCKET, type, (void*)&tmout, sizeof(struct timeval));
  if (rc<0) {
    ipmsg_err_dialog("%s:%s(%d)\n", _("Can not set broad cast"), strerror(errno), errno);
    return -errno;
  }
  dbg_out("Set socket %d for %s timeout %d sec %d msec\n",
	  soc,
	  (type == SO_RCVTIMEO) ? ("Receive") : ("Send"),
	  sec,
	  (usec/1000) );
  return 0;
}
int
sock_recv_time_out(int soc,long msec){
  int rc;

  if ( (soc<0) || (msec<0) )
    return -EINVAL;

  rc=internal_set_timeout(soc,SO_RCVTIMEO,(unsigned long)msec);
  if (rc<0) 
    return rc;

  return 0;
}
int
sock_send_time_out(int soc,long msec){
  int rc;

  if ( (soc<0) || (msec<0) )
    return -EINVAL;

  rc=internal_set_timeout(soc,SO_SNDTIMEO,(unsigned long)msec);
  if (rc<0) 
    return rc;

  return 0;
}

int
setup_addr_info(struct addrinfo **infop, const char *ipaddr,int port,int stype,int family){
  int rc = 0;
  int soc = 0;
  struct addrinfo hints;
  struct addrinfo *res = NULL;
  struct addrinfo *node = NULL;
  char port_str[11];
  char host[NI_MAXHOST];
  char serv[NI_MAXSERV];

  if (infop == NULL)
    return -EINVAL;

  memset(&hints, 0, sizeof(struct addrinfo));
  snprintf(port_str, 10, "%d", port);

  if (family != 0)
    hints.ai_family = family;
  else
    hints.ai_family = PF_UNSPEC;
  
  hints.ai_socktype = stype;
  
  if (ipaddr == NULL) {
    hints.ai_flags = AI_PASSIVE;
  }

  rc = getaddrinfo(ipaddr, port_str, &hints,&res);
  if (rc != 0) {
    ipmsg_err_dialog("%s:%s(%d)\n", 
		     _("Can not get addr info"), gai_strerror(rc), rc);
    rc = (rc < 0) ? (rc) : (-rc);
    goto error_out;
  }
    
  for(node = res;node;node = node->ai_next) {
    rc = getnameinfo(res->ai_addr, res->ai_addrlen,
		host, sizeof(host),
		serv, sizeof(serv), 
		NI_NUMERICHOST|NI_NUMERICSERV);

    g_assert (rc == 0);

    dbg_out("host:%s service:%s (family,type,proto)=(%d,%d,%d)\n",
	    host,
	    serv,
	    res->ai_family,
	    res->ai_socktype,
	    res->ai_protocol);
  }

  *infop = res;

  return 0;
 error_out:
  return rc;
}

int
wait_socket(int soc,int wait_for,int sec) {
#if defined(G2IPMSG_ENABLE_WAIT_SOCK)
  fd_set rd_set;
  fd_set wr_set;
  int rc;
  int max_count;
  struct timeval tmout_val;
  int i;


  for(i=0;i<NET_MAX_RETRY;++i) {
    max_count=(sec*1000UL)/WAIT_UNIT_MS;
    while(max_count>0) {
      FD_ZERO(&rd_set);
      FD_ZERO(&wr_set);

      if (wait_for & WAIT_FOR_READ)
	FD_SET(soc,&rd_set);
      if (wait_for & WAIT_FOR_WRITE)
	FD_SET(soc,&wr_set);

      ipmsg_update_ui();
      dbg_out("select READ[%s] WRITE[%s]\n",
	      (wait_for & WAIT_FOR_READ)?("WAIT"):("NO"),
	      (wait_for & WAIT_FOR_WRITE)?("WAIT"):("NO") );
      tmout_val.tv_sec=0;
      tmout_val.tv_usec=(WAIT_UNIT_MS*1000);

      rc=select(soc+1,&rd_set,&wr_set,NULL,&tmout_val);
      if (rc>0) {
	dbg_out("selectOK\n");
	break;
      }
      if (rc<0) {
	if (errno == EINTR)
	  continue;
	dbg_out("select Fail:%s (%d)\n",strerror(errno),errno);
	return -errno;
      }
      --max_count;
    }
    if ( (wait_for & WAIT_FOR_READ ) && (FD_ISSET(soc,&rd_set)) )
      return 0;
    if ( (wait_for & WAIT_FOR_WRITE ) && (FD_ISSET(soc,&wr_set)) )
      return 0;
  }
  return -EINVAL; 
#else
  return 0;
#endif
}

int 
sock_set_recv_timeout(int soc,int sec){
  int rc;
  struct timeval tmout_val;  

  tmout_val.tv_sec=sec;
  tmout_val.tv_usec=0;
  dbg_out("Set timeout:%d\n",sec);

  rc=setsockopt(soc, SOL_SOCKET, SO_RCVTIMEO, (void*)&tmout_val, sizeof(struct timeval));
  if (rc<0) {
    ipmsg_err_dialog("Can not set timeout for socket :%s(%d)\n",strerror(errno),errno);
    return -errno;
  }

  return 0;
}

int
netcommon_get_peeraddr(struct addrinfo *info, char *peer_addr) {
  int rc = 0;

  if ( (info == NULL) || (peer_addr == NULL) )
    return -EINVAL;

  rc=getnameinfo(info->ai_addr, info->ai_addrlen,
		 peer_addr, NI_MAXHOST,
		NULL, 0, 
		NI_NUMERICHOST);

  g_assert (rc == 0);

  dbg_out("get peer host:%s (family,type,proto)=(%d,%d,%d)\n",
	  peer_addr,
	  info->ai_family,
	  info->ai_socktype,
	  info->ai_protocol);

  return 0;
}

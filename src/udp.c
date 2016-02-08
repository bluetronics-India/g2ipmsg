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

GList *con_list = NULL;
GStaticMutex udp_list_mutex = G_STATIC_MUTEX_INIT;

const char *
udp_get_peeraddr(const udp_con_t *con){
	int                rc = 0;
	struct addrinfo *info = NULL;

	g_assert(con != NULL);

	info = con->server_info;
	g_assert(info != NULL);

	rc = netcommon_get_peeraddr(info, (char *)con->peer); 
  
	return (const char *)con->peer;
}

int 
udp_setup_server(udp_con_t *con, int port, int family) {
	int                rc = 0;
	int               soc = 0;
	char            reuse = 1;
	struct addrinfo *info = NULL;

	if (con == NULL)
		return -EINVAL;

	memset(con, 0, sizeof(udp_con_t));

	rc = setup_addr_info(&info, NULL, port, SOCK_DGRAM, family);
	if (rc < 0)
		return rc;

	rc = socket(info->ai_family, info->ai_socktype, info->ai_protocol);
	if (rc < 0) 
		goto error_out;

	soc = rc;

	rc = bind(soc, info->ai_addr, info->ai_addrlen);
	if (rc < 0)
		goto error_out;

	con->soc    = soc;
	con->family = family;
	con->port   = port;

	/*
	 * allocate server side
	 */
	rc = setup_addr_info(&info, NULL, port, SOCK_DGRAM, family);
	if (rc != 0)
		goto error_out;

#ifdef IPV6_V6ONLY
	if (info->ai_family == AF_INET6) {
		int v6only = 1;

		rc = setsockopt(soc,IPPROTO_IPV6, IPV6_V6ONLY, 
		    (void *)&v6only, sizeof(v6only));
		if (rc != 0) {
			goto error_out;
		}
	}
#endif /*  IPV6_V6ONLY  */

	con->server_info = info;

	rc = 0;
  
 error_out:
	return rc;
}

int
udp_release_connection(udp_con_t *con) {
	if (con == NULL)
		return -EINVAL;

	dbg_out("Close udp sock:%d\n", con->soc);
	if (con->server_info != NULL)
		freeaddrinfo(con->server_info);

	con->server_info = NULL;

	close(con->soc);

	return 0;
}

int
udp_send_message(const udp_con_t *con, const char *ipaddr, int port, const char *msg, size_t len) {
	int                rc = 0;
	struct addrinfo *info = NULL;

	if ( (con == NULL) || (ipaddr == NULL) || (msg == NULL) )
		return -EINVAL;

	dbg_out("send [addr:%s port:%d]:\n %s\n", ipaddr, port, msg);

	rc = setup_addr_info(&info, ipaddr, port, SOCK_DGRAM, con->family);
	if (rc < 0)
		goto error_out;

	rc = sendto(con->soc, msg, len, 0, info->ai_addr, info->ai_addrlen);

error_out:
	if (info != NULL)
		freeaddrinfo(info);

	return rc;
}

int
udp_send_peer(const udp_con_t *con, const char *msg, size_t len) {
	int                rc = 0;
	struct addrinfo *info = NULL;

	if ( (con == NULL) || (msg == NULL) )
		return -EINVAL;

	dbg_out("send peer:%s\n", msg);

	info = con->server_info;
	g_assert(info != NULL);

	rc = sendto(con->soc, msg, len, 0, info->ai_addr, info->ai_addrlen);

	return rc;
}
static void
show_udp_con_node(gpointer data, gpointer user_data) {
	int         rc = 0;
	udp_con_t *con = NULL;
	char peer_addr[NI_MAXHOST];

	if (data == NULL)
		return;

	con = (udp_con_t *)data;
	rc = getnameinfo(con->server_info->ai_addr, 
	    con->server_info->ai_addrlen, peer_addr, NI_MAXHOST, 
	    NULL, 0, NI_NUMERICHOST);
	g_assert(rc == 0);

	dbg_out("ipaddr = %s len = %d family = %d port = %d\n",
	    peer_addr, con->server_info->ai_addrlen, con->family, con->port);


	return;
}

static int
show_udp_connections(void) {
	int         rc = 0;
	GList    *node = NULL;

        g_static_mutex_lock(&udp_list_mutex);
	g_list_foreach(con_list, show_udp_con_node, NULL);
	g_list_free(con_list);
        g_static_mutex_unlock(&udp_list_mutex);

	rc = 0;

error_out:
	return rc;	
}

static gint 
udp_con_find_by_family(gconstpointer a,gconstpointer b) {
  int rc=-EINVAL;
  udp_con_t *con_a, *con_b;

  if ( (!a) || (!b) ) 
    goto out;

  con_a=(udp_con_t *)a;
  con_b=(udp_con_t *)b;

  if (con_a->family == con_b->family) 
    return 0;

  rc=-ESRCH;
 out:

  return rc; /* Not found */
}

int
udp_send_broadcast_with_addr(const udp_con_t *con, const char *bcast, const char *msg, size_t len) {
	int                rc = 0;
	struct addrinfo *info = NULL;
	GList  *current_entry = NULL;

	if ( (con == NULL) || (msg == NULL) )
		return -EINVAL;

	dbg_out("%s\n", msg);

	if (bcast == NULL) {
		switch(con->family) {
		case PF_INET6:
			rc = setup_addr_info(&info, "ff02::1", 
			    hostinfo_refer_ipmsg_port(), SOCK_DGRAM, PF_UNSPEC);
			break;
		default:
			rc = setup_addr_info(&info, "255.255.255.255", 
			    hostinfo_refer_ipmsg_port(), SOCK_DGRAM, PF_UNSPEC);
			break;
		}
	} else {
		
		rc = setup_addr_info(&info, bcast, hostinfo_refer_ipmsg_port(), 
		    SOCK_DGRAM, PF_UNSPEC);
	}

	if (rc < 0)
		goto error_out;

	rc = sendto(con->soc, msg, len, 0, info->ai_addr, info->ai_addrlen);

error_out:
	if (info != NULL)
		freeaddrinfo(info);

	return rc;
}

int
udp_send_broadcast(const udp_con_t *con,const char *msg,size_t len){

  return udp_send_broadcast_with_addr(con,NULL,msg,len);
}


int 
alloc_udp_connection(udp_con_t **con_p) {
	int         rc = 0;
	udp_con_t *con = NULL;
	
	if ( (con_p == NULL) || (*con_p != NULL) )
		return -EINVAL;

	con = g_malloc(sizeof(udp_con_t));
	if (con == NULL) {
		rc = -ENOMEM;
		goto error_out;
	}

	memset(con,0,sizeof(udp_con_t));

	rc = 0;
	*con_p = con;

error_out:
	return rc;
}

int
free_udp_con_data(udp_con_t **con_p) {
	int         rc = 0;
	udp_con_t *con = NULL;

	if ( (con_p == NULL) || (*con_p == NULL) )
		return -EINVAL;

	con = *con_p;

	close(con->soc);

	if (con->server_info != NULL)
		freeaddrinfo(con->server_info);

	memset(con, 0, sizeof(udp_con_t));

	g_free(con);
	
	*con_p = NULL;
	rc = 0;

error_out:
	return rc;
}

int 
release_udp_connection(udp_con_t **con_p) {
	int         rc = 0;
	GList    *node = NULL;
	udp_con_t *con = NULL;

	if ( (con_p == NULL) || (*con_p == NULL) )
		return -EINVAL;

	con = *con_p;

        g_static_mutex_lock(&udp_list_mutex);

	node = g_list_find(con_list, con);

	if (node == NULL) {
		g_static_mutex_unlock(&udp_list_mutex);
		rc = -ENOENT;
		goto error_out;
	}

	con_list = g_list_remove_link(con_list, node);

        g_static_mutex_unlock(&udp_list_mutex);

	free_udp_con_data((udp_con_t **)&(node->data));
	g_list_free_1(node);

	*con_p = NULL;
	rc = 0;

error_out:
	return rc;
}

static void
free_udp_con_node(gpointer data, gpointer user_data) {
	int         rc = 0;
	udp_con_t *con = NULL;

	if (data == NULL)
		return;

	con = (udp_con_t *)data;
	rc = free_udp_con_data((udp_con_t **)&data);
	g_assert(rc == 0);

	return;
}

int 
free_udp_conlist(void) {
	int         rc = 0;
	GList    *node = NULL;

        g_static_mutex_lock(&udp_list_mutex);
	g_list_foreach(con_list, free_udp_con_node, NULL);
	g_list_free(con_list);
        g_static_mutex_unlock(&udp_list_mutex);

	rc = 0;

error_out:
	return rc;
}

int 
udp_server_setup(int port, int family) {
	int                       rc = 0;
	int                      soc = 0;
	struct addrinfo         *res = NULL;
	struct addrinfo        *node = NULL;
	udp_con_t               *con = NULL;
	char                port_str[11];
	char                    host[NI_MAXHOST];
	char                    serv[NI_MAXSERV];
	char                   reuse = 1;
	struct addrinfo        hints;


	memset(&hints, 0, sizeof(struct addrinfo));
	snprintf(port_str, 10, "%d", port);

	if (family != 0)
		hints.ai_family = family;
	else
		hints.ai_family = PF_UNSPEC;
	
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_PASSIVE;

	rc = getaddrinfo(NULL, port_str, &hints,&res);
	if (rc != 0) {
		ipmsg_err_dialog("%s:%s(%d)\n", 
		    _("Can not get addr info"), gai_strerror(rc), rc);
		rc = (rc < 0) ? (rc) : (-rc);
		goto error_out;
	}
    
	for(node = res; node != NULL; node = node->ai_next) {
		rc = getnameinfo(node->ai_addr, node->ai_addrlen,
		    host, sizeof(host),
		    serv, sizeof(serv), 
		    NI_NUMERICHOST|NI_NUMERICSERV);
		g_assert (rc == 0);

		dbg_out("host:%s service:%s (family,type,proto)=(%d,%d,%d)\n",
		    host,
		    serv,
		    node->ai_family,
		    node->ai_socktype,
		    node->ai_protocol);

		rc = socket(node->ai_family, node->ai_socktype, node->ai_protocol);
		if (rc < 0) 
			goto free_connections;
		
		soc = rc;

#ifdef IPV6_V6ONLY
		if (node->ai_family == AF_INET6) {
			int v6only=1;
			
			rc = setsockopt(soc, IPPROTO_IPV6, IPV6_V6ONLY,
			    (void *)&v6only, sizeof(v6only));
			if (rc != 0) {
				goto free_connections;
			}
		}
#endif /*  IPV6_V6ONLY  */
		rc = bind(soc, node->ai_addr, node->ai_addrlen);
		if (rc < 0)
			goto free_addr_out;
		
		rc = alloc_udp_connection(&con);
		if (rc != 0)
			goto free_connections;

		con->server_info = g_malloc(sizeof(struct addrinfo));
		if (con->server_info == NULL)
			goto free_connections;

		memmove(con->server_info, node, sizeof(struct addrinfo));
		con->server_info->ai_next = NULL;

		con->soc    = soc;
		con->family = node->ai_family;
		con->port   = port;

		rc = udp_enable_broadcast(con);
		if (rc < 0)
			goto free_connections;
		
		con_list = g_list_append(con_list, con);
		con = NULL;
	}

	rc = 0;

	show_udp_connections();

	goto free_addr_out;

free_connections:
	free_udp_conlist();

free_addr_out:
	if (res != NULL)
		freeaddrinfo(res);

error_out:
	return rc;
}

static int
udp_set_buffer(const udp_con_t *con){
  int rc;
  int size=0;

  if (!con)
    return -EINVAL;

  return sock_set_buffer(con->soc, _MSG_BUF_MIN_SIZE, _MSG_BUF_SIZE);
}

int
udp_enable_broadcast(const udp_con_t *con){
  int rc;
  int flag;
  int confirm,confirm_len;

  if (!con) 
    return -EINVAL;

  dbg_out("Here\n");
  flag=1;
  rc=setsockopt(con->soc, SOL_SOCKET, SO_BROADCAST, (void*)&flag, sizeof(int));
  if (rc<0) {
    err_out("Can not set broad cast:%s(%d)\n",strerror(errno),errno);
    return -errno;
  }


  confirm=0;
  confirm_len=sizeof(confirm);
  rc=getsockopt(con->soc, SOL_SOCKET, SO_BROADCAST, (void*)&confirm, &confirm_len);
  if (rc<0) {
    err_out("Can not get broad cast:%s(%d)\n",strerror(errno),errno);
    return -errno;
  }
  if (confirm)
    dbg_out("Broadcat is set\n");
  else
    err_out("Can not set broad cast:%s(%d)\n",strerror(errno),errno);

  return 0;
}
int
udp_disable_broadcast(const udp_con_t *con){
  int rc;
  int flag;

  if (!con) 
    return -EINVAL;

  dbg_out("Here\n");
  flag=0;
  rc=setsockopt(con->soc, SOL_SOCKET, SO_BROADCAST, (void*)&flag, sizeof(int));
  if (rc<0) {
    err_out("Can not unset broad cast:%s(%d)\n",strerror(errno),errno);
    return -errno;
  }

  return 0;
}

int
udp_recv_message(const udp_con_t *con,char **msg,size_t *len){
  ssize_t recv_len;
  char *recv_buf=NULL;
  int old_fd_flags;
  int rc;
  int ret_code=0;
  struct addrinfo *info;

  if ( (!con) || (!msg) || (!len) )
    return -EINVAL;

  g_assert(*msg==NULL);  

  recv_buf=g_malloc(_MSG_BUF_SIZE);
  if (!recv_buf)
    return -ENOMEM;

  rc=udp_set_buffer(con);
  if (rc<0)
    goto error_out;

  rc=udp_enable_broadcast(con);
  if (rc<0)
    goto error_out;

  info=con->server_info;
  /*
   *  空読みする
   */

  recv_len=recvfrom(con->soc,recv_buf,_MSG_BUF_SIZE,(MSG_PEEK|MSG_DONTWAIT),info->ai_addr,&(info->ai_addrlen));
  if (recv_len<=0) {
    err_out("%s(errno:%d)\n",strerror(errno),errno);
    ret_code=-errno;
    goto error_out;
  }

  dbg_out("read:%d %s\n",recv_len,recv_buf);
  /*
   *  バッファに読み込む
   */
  recv_len=recvfrom(con->soc,recv_buf,recv_len,MSG_DONTWAIT,info->ai_addr,&(info->ai_addrlen));
  if (recv_len<0) {
    err_out("%s(errno:%d)\n",strerror(errno),errno);
    ret_code=-errno;
    goto error_out;
  }

  /*
   * バッファ獲得
   */
  
  g_assert(*msg==NULL);
  *msg=g_malloc(recv_len);
  if (!(*msg))
    goto error_out;
  dbg_out("copy:%d %s\n",recv_len,recv_buf);
  *len=recv_len;
  memmove(*msg,recv_buf,recv_len);  /*  内容をコピー  */

error_out:
  if (recv_buf)
    g_free(recv_buf);

  return ret_code;
}

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

#if !defined(UDP_H)
#define UDP_H
#include <sys/socket.h>
#include <netdb.h>

typedef struct _udp_con{
  int soc;
  int family;
  int port;
  int target_tag;
  char peer[NI_MAXHOST];
  struct addrinfo *server_info;
}udp_con_t;


int alloc_udp_connection(udp_con_t **con_p);
int free_udp_con_data(udp_con_t **con_p);
int release_udp_connection(udp_con_t **con_p);
int free_udp_conlist(void);
int udp_server_setup(int port, int family);

int udp_setup_server(udp_con_t *con, int port, int family);
int udp_release_connection(udp_con_t *con);

int udp_send_message(const udp_con_t *con,const char *ipaddr,int port, const char *msg,size_t len);
int udp_send_peer(const udp_con_t *con,const char *msg,size_t len);
int udp_send_broadcast_with_addr(const udp_con_t *con,const char *bcast,const char *msg,size_t len);
int udp_send_broadcast(const udp_con_t *con,const char *msg,size_t len);
int udp_enable_broadcast(const udp_con_t *con);
int udp_disable_broadcast(const udp_con_t *con);
int udp_recv_message(const udp_con_t *con,char **msg,size_t *len);
const char *udp_get_peeraddr(const udp_con_t *con);
int udp_release_connection(udp_con_t *con);
#endif

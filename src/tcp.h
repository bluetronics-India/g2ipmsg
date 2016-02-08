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

#if !defined(IPMSG_TCP_H)
#define IPMSG_TCP_H
#include <signal.h>

#include "g2ipmsg.h"
#include "ipmsg_types.h"

#define TCP_LISTEN_LEN       (5)
#define TCP_DOWNLOAD_RETRY   (200)
#define TCP_FLUSH_RETRY      (10)
#define TCP_SELECT_SEC       (1)
#define TCP_CLIENT_TMOUT_MS  (500)
#define TCP_CLOSE_WAIT_SEC   (5)


typedef struct _tcp_con{
  int soc;
  int family;
  struct addrinfo *peer_info;
  char peer[NI_MAXHOST];
}tcp_con_t;

typedef struct _request_msg{
  pktno_t pkt_no;
  int fileid;
  int offset;
}request_msg_t;

typedef struct _tcp_dsend_pkt{
  tcp_con_t *con;
  char *topdir;
}tcp_dsend_pkt_t;

gpointer ipmsg_tcp_server_thread(gpointer data);
int tcp_enable_keepalive(const tcp_con_t *con);
int tcp_setup_client(int family,const char *ipaddr,int port,tcp_con_t *con);
int tcp_flush_buffer(tcp_con_t *con);
int tcp_set_ipmsg_bufsiz(int soc);
int disable_pipe_signal(struct sigaction *saved_act);
int enable_pipe_signal(struct sigaction *saved_act);
#endif /* IPMSG_TCP_H */

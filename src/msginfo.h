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

#if !defined(MSG_INFO_H)
#define MSG_INFO_H

#include "ipmsg_types.h"
#include "udp.h"
#include <glib.h>


#define MSG_INFO_MAX_RETRY  5
#define MSG_INFO_RETRY_INTERVAL (5*1000) /* 5秒 */
typedef struct _message_info{
  udp_con_t *con;
  pktno_t  seq_no;         /* パケット番号  */
  gchar *ipaddr;        /*  送信先アドレスのコピー  */
  gchar *ed_msg_string; /*  外部形式で表した送信伝文(のコピー)  */
  size_t len;
  int  retry_remains;  /*  残回数  */
  gboolean first;
}message_info_t;
typedef struct _send_info{
  char *msg;
  int flags;
  GtkWidget *attachment_editor;
}send_info_t;
int register_sent_message(const udp_con_t *con,const char *ipaddr,pktno_t pktno,const char *message,size_t len);
int unregister_sent_message(int pktno);
int retry_messages_once(void);
int init_message_info_manager(void);
#endif /*  MSG_INFO_H */

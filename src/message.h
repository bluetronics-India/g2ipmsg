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

#if !defined(MESSAGE_H)
#define MESSAGE_H

#include "ipmsg_types.h"

#define IPMSG_MSG_MAGIC 0x20061123
#define NAME_LEN 32
typedef struct _msg_data{
  int magic;
  int version;
  pktno_t pkt_seq_no;
  int command;
  int command_opts;
  char *username;
  char *hostname;
  char *extstring;
  struct timeval tv;
  char *message;
}msg_data_t;

int get_command_from_msg(const msg_data_t *msg, unsigned long *command, unsigned long *command_opts);
const char *refer_user_name_from_msg(const msg_data_t *msg);
const char *refer_host_name_from_msg(const msg_data_t *msg);
const char *refer_group_name_from_msg(const msg_data_t *msg);
const char *refer_nick_name_from_msg(const msg_data_t *msg);
pktno_t refer_pkt_no_name_from_msg(const msg_data_t *msg);
int init_message_data(msg_data_t *msg);
int release_message_data(msg_data_t *msg);
int parse_message(const char *ipaddr,msg_data_t *msg,const char *message_buff,size_t len);
#endif  /*  MESSAGE_H  */

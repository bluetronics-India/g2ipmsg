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

int
get_command_from_msg(const msg_data_t *msg, unsigned long *command, unsigned long *command_opts) {
	int rc = 0;

	if ( (msg == NULL) || (command == NULL) ) {
		rc = -EINVAL;
	}
	if (command_opts != NULL)
		*command_opts = msg->command_opts;

	*command = msg->command;

error_out:
	return rc; 
}

const char *
refer_user_name_from_msg(const msg_data_t *msg){
  if (!msg)
    return G2IPMSG_UNKNOWN_NAME;
  return (msg->username)?(msg->username):(G2IPMSG_UNKNOWN_NAME);
}

const char *
refer_host_name_from_msg(const msg_data_t *msg){
  if (!msg)
    return G2IPMSG_UNKNOWN_NAME;
  return (msg->hostname)?(msg->hostname):(G2IPMSG_UNKNOWN_NAME);
}
const char *
refer_nick_name_from_msg(const msg_data_t *msg){
  if (!msg)
    return G2IPMSG_UNKNOWN_NAME;
  return (msg->message)?(msg->message):(G2IPMSG_UNKNOWN_NAME);
}
const char *
refer_group_name_from_msg(const msg_data_t *msg){
  if (!msg)
    return G2IPMSG_UNKNOWN_NAME;
  return (msg->extstring)?(msg->extstring):(G2IPMSG_UNKNOWN_NAME);
}
pktno_t
refer_pkt_no_name_from_msg(const msg_data_t *msg){
  return msg->pkt_seq_no;
}

int
init_message_data(msg_data_t *msg){
  if (!msg)
    return -EINVAL;

  memset(msg,0,sizeof(msg_data_t));
  msg->magic=IPMSG_MSG_MAGIC;

  return 0;
}
int
release_message_data(msg_data_t *msg){

  if ( (!msg) || (msg->magic!= IPMSG_MSG_MAGIC) )
    return -EINVAL;
  
  if (msg->username)
    g_free(msg->username);

  if (msg->hostname)
    g_free(msg->hostname);

  if (msg->extstring)
    g_free(msg->extstring);

  if (msg->message)
    g_free(msg->message);

  msg->magic=0;

  return 0;
}
int
parse_message(const char *ipaddr,msg_data_t *msg,const char *message_buff,size_t len){
  long int_val;
  pktno_t pkt_val;
  char *sp=NULL;
  char *ep=NULL;
  char *buffer;
  ssize_t remains;
  char *temp_buffer=NULL;
  size_t tmp_len;
  int rc=0;

  /*
   * TCP経由でよばれた場合は, ipaddrがNULLになりうる  
   */
  if  ( (!message_buff) || (!msg)  || (msg->magic!= IPMSG_MSG_MAGIC) )
    return -EINVAL;
  _assert(len>0);
  buffer=g_malloc(len);
  if (!buffer)
    return -ENOMEM;

  gettimeofday(&msg->tv, NULL);

  memcpy(buffer,message_buff,len);
  remains=len;
  /*
   * バージョン番号   
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
  int_val=strtol(sp, (char **)NULL, 10);
  msg->version=int_val;
  sp=ep;

  /*
   * シーケンス番号   
   */
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
  pkt_val=strtoll(sp, (char **)NULL, 10);
  msg->pkt_seq_no=pkt_val;
  sp=ep;

  /*
   * 名前
   */
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
  msg->username=g_strdup(sp);
  sp=ep;

  /*
   * ホスト名
   */
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
  msg->hostname=g_strdup(sp);
  sp=ep;

  /*
   * コマンド番号   
   */
  ep=memchr(sp, ':', remains);
  if (!ep) {
    rc=-EINVAL;
    goto error_out;
  }
  *ep='\0';
  ++ep;
  int_val=strtol(sp, (char **)NULL, 10);
  msg->command = ipmsg_protocol_flags_get_command(int_val);
  msg->command_opts = ipmsg_protocol_flags_get_opt(int_val);
  sp=ep;
 
  /*
   *メッセージ本文
   */
  ep=memchr(sp, '\0', remains);
  if (!ep) {
    rc=-EINVAL;
    goto error_out;
  }
  if ( (msg->command == IPMSG_SENDMSG) && (msg->command_opts & (IPMSG_ENCRYPTOPT)) )  {
#if defined(USE_OPENSSL)
    unsigned char *enc_buff=NULL;
    char *extend=NULL;
    char *sep=NULL;
    size_t enc_len;

    /* 暗号化がある場合は, NULLを許さない(署名の検証があるので) */
    if (ipaddr == NULL)
      goto error_out; /*  復号不能のためメッセージは捨てる(攻撃とみなす) */

    dbg_out("This is encrypted message:%s.\n",sp);
    rc = ipmsg_decrypt_message(ipaddr,sp,&enc_buff,&enc_len);
    if (rc) {
      goto error_out;
    }
    msg->message=g_strdup(enc_buff);
    if (!(msg->message))
      goto decode_end;
    dbg_out("body:%s\n",msg->message);
    dbg_out("Decrypt message %s(%d) total=%d.\n",enc_buff,strlen(enc_buff),enc_len);
  decode_end:
    if (enc_buff)
      g_free(enc_buff);
#else
    dbg_out("I can not decode encrypted message.Ignore the message.");
    goto error_out; /*  暗号化されたメッセージは捨てる
		   *  (暗号化できないクライアントに送ってきた方が悪い)  
		   */    
#endif  /*  USE_OPENSSL  */
  }else{
    msg->message=g_strdup(sp);
    if (!(msg->message))
      goto error_out;
    dbg_out("body:%s\n",msg->message);
  }
  /*
   *拡張部
   */
  if ( ((unsigned long)ep - (unsigned long)buffer) < len) {
    ++ep;
    sp=ep;
    msg->extstring=g_strdup(sp);
    dbg_out("extention:%s\n",msg->extstring);
  }
error_out:
  g_free(buffer);
  return rc;
}

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

static GList *message_list=NULL;
GStaticMutex msglst_mutex = G_STATIC_MUTEX_INIT;
static guint timer_id;

static gint 
message_info_find_element(gconstpointer a,gconstpointer b) {
  message_info_t  *msg_a,*msg_b;

  if ( (!a) || (!b) )
    return -EINVAL;

  msg_a=(message_info_t *)a;
  msg_b=(message_info_t *)b;

  if (msg_a->seq_no == msg_b->seq_no)
    return 0; /* found */

  return 1; /* Not found */
}
static int
free_message_entry(GList *entry){
  message_info_t *this_msg;

  if (!entry)
    return -EINVAL;
  this_msg=entry->data;

  _assert(this_msg);
  dbg_out("Release seqno %d msg\n",this_msg->seq_no);

  g_free(this_msg->ipaddr);
  g_free(this_msg->ed_msg_string);
  dbg_out("Free: %x\n",(unsigned int)this_msg);
  g_slice_free(message_info_t,this_msg);

  message_list=g_list_remove_link(message_list,entry);
  g_list_free(entry);

  return 0;
}
int
unregister_sent_message(int pktno){
  GList *found;
  message_info_t srch_msg;

  dbg_out("Try to unregister %d\n",pktno);

  srch_msg.seq_no=pktno;

  g_static_mutex_lock(&msglst_mutex);
  found=g_list_find_custom (message_list,&srch_msg,message_info_find_element);  
  if (found) {
    free_message_entry(found);
  }
  g_static_mutex_unlock(&msglst_mutex);

  return 0;
}

/** 送信済みメッセージを記憶し, 後で再送するように予約する.
 * @param[in] con           UDPコネクション情報
 * @param[in] ipaddr        送信先IPアドレス
 * @param[in] pktno         送信時パケット番号
 * @param[in] packet       送信パケット(外部形式)
 * @param[in] len           メッセージ長
 *  @retval     0            正常終了
 *  @retval    -EINVAL       引数異常
 *  @retval    -ENOMEM       メモリ不足
 */
int
register_sent_message(const udp_con_t *con, const char *ipaddr, int pktno, const char *packet, size_t len){
	message_info_t    *new_msg = NULL;
	message_info_t *update_msg = NULL;
	GList               *found = NULL;
	message_info_t    srch_msg;
	int                     rc = 0;

	if ( (con == NULL) ||  (ipaddr == NULL) || (packet == NULL) )
		return -EINVAL;

	dbg_out("register ipaddr=%s\n", ipaddr);
	srch_msg.seq_no = pktno;

	g_static_mutex_lock(&msglst_mutex);
	found = g_list_find_custom (message_list, &srch_msg, message_info_find_element);
	if (found != NULL) { 
		/*  既に存在する場合は, リトライ回数を減算(ここにはこないはず)  */
		err_out("pktno duplicated: %d\n", pktno);
		g_assert_not_reached();

		_assert(found->data);
		update_msg = found->data;

		--(update_msg->retry_remains);
		dbg_out("pktno: %d remain:%d\n", pktno, update_msg->retry_remains);

		if (update_msg->retry_remains == 0) 
			free_message_entry(found);
		

		g_static_mutex_unlock(&msglst_mutex);

		goto success_out;
	}
	g_static_mutex_unlock(&msglst_mutex);


	new_msg = g_slice_new(message_info_t);
	if (new_msg == NULL) {
		rc = -ENOMEM;
		goto error_out;
	}
	new_msg->ed_msg_string = NULL;
	new_msg->ed_msg_string = g_strdup(packet);

	new_msg->ipaddr = g_strdup(ipaddr);
	if (new_msg->ipaddr == NULL) {
		rc = -ENOMEM;
		goto free_msg_string;
	}

	new_msg->con           = (udp_con_t *)con;
	new_msg->len           = len;
	new_msg->seq_no        = pktno;
	new_msg->retry_remains = MSG_INFO_MAX_RETRY;
	new_msg->first         = TRUE;

	g_static_mutex_lock(&msglst_mutex);

	message_list = g_list_append(message_list, new_msg);
	g_static_mutex_unlock(&msglst_mutex);

	dbg_out("Add new message:(pktno, packet)=(%d %s)\n", pktno, packet);

success_out:
	rc = 0;

	return rc; /* 正常終了 */

	/*
	 * 異常系 
	 */
	if (new_msg->ipaddr != NULL)
		g_free(new_msg->ipaddr);

free_msg_string:
	if (new_msg->ed_msg_string != NULL)
		g_free(new_msg->ed_msg_string);

free_message_info:
	dbg_out("Free: %x\n",(unsigned int)new_msg);
	g_slice_free(message_info_t,new_msg);

error_out:
	return rc;
}

static gboolean 
retry_message_handler(gpointer data) {
  retry_messages_once();
  return TRUE; 
}
int
retry_messages_once(void){
  int rc=0;
  GList *last;
  GList *entry;
  GList *retry_messages=NULL;
  message_info_t *this_msg;

  g_static_mutex_lock(&msglst_mutex);
  last=g_list_last(message_list);
  g_static_mutex_unlock(&msglst_mutex);
  if (!last) {
    return 0;
  }
  dbg_out("Try to re-send unsent messages.\n");
  g_source_remove (timer_id);
  g_static_mutex_lock(&msglst_mutex);
  while((entry=g_list_first(message_list))) {
    if (entry->data) {
      this_msg=entry->data;
      if (this_msg->first) {
	this_msg->first=FALSE;
      } else {
	dbg_out("retry:%d seq:%d addr:%s port: %d\nmsg:\n%s\n", 
		this_msg->retry_remains,
		this_msg->seq_no,
		this_msg->ipaddr,
		hostinfo_refer_ipmsg_port(),
		this_msg->ed_msg_string);
	/*
	 * メッセージの再登録を避けるために, udp_send_messageを直接呼び出す.
	 */
	rc = ipmsg_send_message(udp_con, this_msg->ipaddr, 
				this_msg->ed_msg_string, this_msg->len);
	if (rc<0) {
	  rc*=-1;
	  ipmsg_err_dialog("%s: %s %d\n", _("Can not send"), strerror(rc), rc);
	}else{
	  --this_msg->retry_remains;
	}
      }
      if (!(this_msg->retry_remains)) {
	GtkWidget *dialog;
	GtkWidget *nameLabel;
	userdb_t *user_info=NULL;
	char buffer[64];

	dialog=GTK_WIDGET(create_sendFailDialog ());
	if (!userdb_search_user_by_addr(this_msg->ipaddr,(const userdb_t **)&user_info)) {
	  nameLabel=lookup_widget(dialog,"SendFailDialogUserLabel");
	  g_assert(nameLabel);
	  snprintf(buffer,63,"%s@%s (%s)",user_info->nickname,user_info->group,user_info->host);
	  buffer[63]='\0';
	  gtk_label_set_text(GTK_LABEL(nameLabel),buffer);
	  g_assert(!destroy_user_info(user_info));
	}
	if (gtk_dialog_run (GTK_DIALOG (dialog))==GTK_RESPONSE_OK) {
	  this_msg->retry_remains=MSG_INFO_MAX_RETRY;
	  dbg_out("reset retry count:seq=%d (remains:%d)\n", this_msg->seq_no,this_msg->retry_remains);
	  this_msg->first=TRUE; /* 再登録 */
	}else{
	  dbg_out("Free:seq=%d\n", this_msg->seq_no);
	  free_message_entry(entry);
	  gtk_widget_destroy (dialog);
	  continue;
	}
	gtk_widget_destroy (dialog);
      }
      message_list=g_list_remove_link(message_list,entry);
      retry_messages=g_list_concat(retry_messages,entry);
    }
  }
  message_list=g_list_concat(message_list,retry_messages);
  g_static_mutex_unlock(&msglst_mutex);
  timer_id=g_timeout_add(MSG_INFO_RETRY_INTERVAL,retry_message_handler,NULL);
  return 0;

}
int
init_message_info_manager(void){
  dbg_out("Start retry handler\n");
  timer_id=g_timeout_add(MSG_INFO_RETRY_INTERVAL,retry_message_handler,NULL);
  return 0;
}

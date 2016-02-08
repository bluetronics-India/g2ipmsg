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

/** @file 
 * @brief  IPMSGプロトコル関連関数群
 * @author Takeharu KATO
 */ 


/** パケット番号
 * @attention 内部リンケージ
 */
static pktno_t pkt_no = 0;

/** スレッド間でのパケット番号アクセス排他用ロック
 * @attention 内部リンケージ
 */
GStaticMutex pktno_mutex = G_STATIC_MUTEX_INIT;


/** IPMSGのメッセージパケットを生成し, 送信先のコードセットに変換する
 *  @param[in]  ipaddr      送信先IPアドレス(送信先コードセット判定に使用)
 *                          (NULLの場合は, ブロードキャスト用として
 *                           扱い, デフォルトコードセットを使用する).
 *  @param[in]  pkt_arg      パケット番号(0の場合は, 自動生成する)
 *  @param[in]  flags        送信フラグ
 *  @param[in]  message      メッセージ部に格納する文字列を設定する.
 *  @param[in]  extension    拡張部に格納する文字列を設定する.
 *  @param[out]  external    外部形式のパケット文字列を指すためのポインタのアドレス
 *  @param[out]  len_p       送信パケット長(ヌルターミネート含む)返却先アドレス
 *  @param[out]  pkt_ret     割り当てたパケット番号の返却先アドレス
 *                           IPMSG_PROTOCOL_MSG_NONEED_PKTNOが指定された場合は,
 *                           パケット番号の返却を実施しない
 *  @retval  0       正常終了
 *  @retval -EINVAL 引数異常
 *                  
 *  @attention 内部リンケージ
 */
static int
build_ipmsg_packet(const char *ipaddr, const pktno_t pkt_arg, 
		   const ipmsg_send_flags_t flags, const char *message, 
		   const char *extension, char **external, size_t *len_p, 
		   pktno_t *pkt_ret) {
	int                      rc = 0;
	pktno_t              pkt_no = 0;
	size_t                  len = 0;
	gchar     *converted_common = NULL;
	gchar    *converted_message = NULL;
	gchar  *converted_extension = NULL;
	gchar *converted_all_packet = NULL;
	gchar               *common = NULL;

	if ( (ipaddr == NULL)  || (external == NULL) || 
	    (*external != NULL) || (len_p == NULL) ) {
		rc = -EINVAL;
	}

	/*
	 * パケット番号獲得
	 */
	if (pkt_arg == IPMSG_PROTOCOL_PKTNUM_AUTO)
		pkt_no = ipmsg_get_pkt_no();
	else
		pkt_no = pkt_arg;

	common = g_malloc(IPMSG_BUFSIZ);
	if (common == NULL)
	  goto error_out;
	
	  
	/* 
	 * 共通部分生成
	 */
	snprintf(common, IPMSG_BUFSIZ, IPMSG_PROTOCOL_COMMON_MSG_FMT, 
	    pkt_no, hostinfo_refer_user_name(), 
	    hostinfo_refer_host_name(), flags);

	/*
	 * 外部形式に変換
	 */

	len = 0; /* 出力長を初期化  */


	/* 共通部分 */
	rc = ipmsg_convert_string_external(ipaddr, common, 
	    (const char **)&converted_common);
	if (rc != 0) {
		ipmsg_err_dialog("%s:%s\n", 
		    _("Can not convert common into external representation"), 
		    common);
		goto free_common_out;
	} else {
		/*
		 * 末尾のヌルターミネートを含めて送信するため1加える
		 */
		len += (strlen(converted_common) + 1);
	}

	/* メッセージ本体  */
	if (message != NULL) {
		rc = ipmsg_convert_string_external(ipaddr, message, 
		    (const char **)&converted_message);
		if (rc != 0) {
			ipmsg_err_dialog("%s\n", 
			    _("Can not convert message into external representation"));
			goto free_converted_common;
		} else {
			/*
			 * 末尾のヌルターミネートを含めて送信するため1加える
			 */
			len += (strlen(converted_message) + 1);
		}
	}

	/* 拡張部  */
	if (extension != NULL) {
		rc = ipmsg_convert_string_external(ipaddr, extension, 
		    (const char **)&converted_extension);
		if (rc != 0) {
			ipmsg_err_dialog("%s:%s\n", 
			    _("Can not convert extension into external representation"), 
			    extension);
			goto free_converted_message;
		} else {
			/*
			 * 末尾のヌルターミネートを含めて送信するため1加える
			 */
			len += (strlen(converted_extension) + 1);
		}
	}

	/*
	 * パケットを組み立てる
	 */
	converted_all_packet = g_malloc(len);
	if (converted_all_packet == NULL) {
		rc = -ENOMEM;
		goto free_converted_extension;
	}

	memset(converted_all_packet, 0, len);
	snprintf(converted_all_packet, len, IPMSG_PROTOCOL_ALLMSG_PACKET_FMT,
	    converted_common, 
	    (converted_message != NULL) ? (converted_message) : (""),
	    IPMSG_PROTOCOL_MSG_EXT_DELIM,
	    (converted_extension != NULL) ? (converted_extension) : (""));

	dbg_out("New packet:" IPMSG_PROTOCOL_ALLMSG_PACKET_FMT,
	    common, message, IPMSG_PROTOCOL_MSG_EXT_DELIM_SYM, extension);

	/*
	 * 返却
	 */
	*external = converted_all_packet;
	*len_p = len;
	if (pkt_ret != IPMSG_PROTOCOL_MSG_NONEED_PKTNO) 
		*pkt_ret = pkt_no;

	rc = 0; /* 正常終了 */

free_converted_extension:
	if (converted_extension != NULL)
		g_free(converted_extension);

free_converted_message:
	if (converted_message != NULL)
		g_free(converted_message);

free_converted_common:
	if (converted_common != NULL)
		g_free(converted_common);
  
free_common_out:
	if (common != NULL)
		g_free(common);

error_out:
	return rc;
}

/** IPMSGのパケットを送出する.
 *  @param[in]  con          UDPコネクション情報
 *  @param[in]  ipaddr       送信先IPアドレス
 *  @param[in]  pktno        パケット番号
 *                           - IPMSG_PROTOCOL_PKTNUM_AUTO 番号を自動的に割り当てる
 *                           - 上記以外                   pktnoで指定した番号を使用
 *  @param[in]  flags        送信フラグ
 *  @param[in]  message      メッセージ部(内部形式)
 *                           - IPMSG_PROTOCOL_MSG_NO_MESSAGE 空文を送信する
 *                           - 上記以外                      messageで指定した文を送信する
 *  @param[in]  extension    拡張部(内部形式)
 *                           - IPMSG_PROTOCOL_MSG_NO_EXTENSION 空文を送信する
 *                           - 上記以外                    extensionで指定した文を送信する
 *  @retval     0            正常終了
 *  @retval    -EINVAL       引数異常
 *  @attention 内部リンケージ
 */
static int
ipmsg_send_packet(const udp_con_t *con, const char *ipaddr, const pktno_t pktno, const int flags, const char *message, const char *extension) {
	char              *packet = NULL;
	int                    rc = 0;
	size_t                len = 0;
	pktno_t       sent_pkt_no = 0;

	if ( (ipaddr == IPMSG_PROTOCOL_ENTRY_PKT_ADDR) && (con == NULL) ){
		rc = -EINVAL;
		goto error_out;
	}

	/*
	 * パケット構築
	 */
	rc = build_ipmsg_packet(ipaddr, pktno, flags, message, extension, 
	    &packet, &len, &sent_pkt_no);
	
	if (rc != 0) {
		g_assert(packet == NULL);  /* エラー時は獲得しない前提 */
		goto error_out;
	}

	dbg_out("send message to %s:(message, ext-part) = (%s, %s)\n", 
	    ( ipaddr ), 
	    ( (message   != NULL) ? message : ""), 
	    ( (extension != NULL) ? extension : "") );

	/*
	 * 送信済みパケット記録
	 */
	if  ( ( flags & IPMSG_SENDCHECKOPT ) &&
	      ( !( flags & (IPMSG_NO_REPLY_OPTS|IPMSG_AUTORETOPT) ) ) ){
			rc = register_sent_message(con, ipaddr, sent_pkt_no, 
						   packet, len);

			if (rc < 0) {
				rc = -rc;
				ipmsg_err_dialog(
					_("Can not register message for:"
					    "%s reason: %s error code = %d"), 
					ipaddr, strerror(rc), rc);
				goto free_packet_out;
			}
	}
	

	/*
	 * 送信 
	 */ 
	rc = ipmsg_send_message(con, ipaddr, packet, len);
	if (rc != 0) {
		goto free_packet_out;
	}

	rc = 0; /* 正常終了 */

free_packet_out:
	g_free(packet);

error_out:
	return rc;
}
#if 0
/** IPMSGのブロードキャストパケットを送出する.
 *  @param[in]  pktno        パケット番号
 *                           - IPMSG_PROTOCOL_PKTNUM_AUTO 番号を自動的に割り当てる
 *                           - 上記以外                   pktnoで指定した番号を使用
 *  @param[in]  flags        送信フラグ
 *  @param[in]  message      メッセージ部(内部形式)
 *                           - IPMSG_PROTOCOL_MSG_NO_MESSAGE 空文を送信する
 *                           - 上記以外                      messageで指定した文を送信する
 *  @param[in]  extension    拡張部(内部形式)
 *                           - IPMSG_PROTOCOL_MSG_NO_EXTENSION 空文を送信する
 *                           - 上記以外                    extensionで指定した文を送信する
 *  @retval     0            正常終了
 *  @retval    -EINVAL       引数異常
 *  @attention 内部リンケージ
 */
static int
ipmsg_send_broadcast(const pktno_t pktno, const int flags, const char *message, const char *extension) {
	char              *packet = NULL;
	int                    rc = 0;
	size_t                len = 0;
	pktno_t       sent_pkt_no = 0;

	if ( ipaddr != IPMSG_PROTOCOL_ENTRY_PKT_ADDR) {
		rc = -EINVAL;
		goto error_out;
	}

	/*
	 * パケット構築
	 */
	rc = build_ipmsg_packet(ipaddr, pktno, flags, message, extension, 
	    &packet, &len, &sent_pkt_no);
	
	if (rc != 0) {
		g_assert(packet == NULL);  /* エラー時は獲得しない前提 */
		goto error_out;
	}

	dbg_out("send message to %s:(message, ext-part) = (%s, %s)\n", 
	    ("Broad cast addr"), 
	    ( (message   != NULL) ? message : ""), 
	    ( (extension != NULL) ? extension : "") );


	/*
	 * 送信 
	 */ 
	rc = udp_send_broadcast(packet, len);
	if (rc != 0) {
		goto free_packet_out;
	}

	rc = 0; /* 正常終了 */

free_packet_out:
	g_free(packet);

error_out:
	return rc;
}
#endif
/** IPMSGのパケット番号通知/インデクス番号通知系メッセージを送出する.
 *  @param[in]  con          UDPコネクション情報
 *  @param[in]  ipaddr       送信先IPアドレス
 *                           - IPMSG_PROTOCOL_ENTRY_PKT_ADDR ブロードキャストとして扱う
 *                           - 上記以外                      ユニキャストとして扱う
 *  @param[in]  flags        送信フラグ
 *  @param[in]  pktno        通知するパケット番号
 *                           - IPMSG_PROTOCOL_PKTNUM_AUTO 番号を自動的に割り当てる
 *                           - 上記以外                   pktnoで指定した番号を使用
 *  @retval     0            正常終了
 *  @retval    -EINVAL       引数異常
 *  @note  各番号通知系メッセージの形式は, 以下のとおり:
 *
 *  IPMSG_RECVMSG パケット形式
 *    - メッセージ部: 受信パケット番号を表す文字列
 *    - 拡張部      : なし
 *
 *  IPMSG_RELEASEFILES
 *    - メッセージ部: 開放ファイルパケット番号を表す文字列
 *    - 拡張部      : なし
 *
 *  IPMSG_GETLIST:
 *    - メッセージ部: ホストリストエントリ取得開始番号
 *    - 拡張部      : なし
 *
 *  @attention 内部リンケージ
 */
static int
ipmsg_notify_number(const udp_con_t *con, const char *ipaddr , 
		    ipmsg_send_flags_t flags, pktno_t num) {
	int                    rc = 0;
	char              pkt_buf[IPMSG_PROTOCOL_PKTNUM_LEN];

	g_assert(con != NULL);  /* 呼び出し側責任で引数チェックを実施 */
	
	/*
	 * パケット番号を文字列化して送信.
	 */
	snprintf(pkt_buf, IPMSG_PROTOCOL_PKTNUM_LEN, IPMSG_PROTOCOL_PKTNUM_FMT, num);

	dbg_out("Notify number : (ipaddr, pktno) = (%s, %d)\n", ipaddr, num);

	/*
	 * パケット送信
	 */
	rc = ipmsg_send_packet(con, ipaddr,IPMSG_PROTOCOL_PKTNUM_AUTO, flags, 
	    pkt_buf, IPMSG_PROTOCOL_MSG_NO_EXTENSION);
	if (rc != 0) {
		goto error_out;
	}

	rc = 0; /* 正常終了 */

error_out:
	return rc;
}

/** IPMSGのIPMSG_BR_ISGETLIST2を送出する.
 *  @param[in]  flags        送信フラグ
 *  @retval     0            正常終了
 *  @retval    -EINVAL       引数異常
 *  @note  IPMSG_BR_ISGETLIST2 パケット形式
 *
 *    - メッセージ部: 空文
 *    - 拡張部      : なし
 *
 *  @attention 内部リンケージ
 */
static int
ipmsg_send_br_isgetlist2(const udp_con_t *con, const ipmsg_send_flags_t flags){
	int                         rc = 0;
	ipmsg_send_flags_t local_flags = 0;

	if (con == NULL) {
		rc = -EINVAL;
		goto error_out;
	}

	/*
	 * フラグを設定する
	 */
	local_flags  = flags;
	local_flags |= hostinfo_get_normal_entry_flags();
	local_flags |= IPMSG_BR_ISGETLIST2;

	/*
	 * パケット送信
	 */
	rc = ipmsg_send_packet(con, IPMSG_PROTOCOL_ENTRY_PKT_ADDR, 
	    IPMSG_PROTOCOL_PKTNUM_AUTO, local_flags, 
	    IPMSG_PROTOCOL_MSG_NO_MESSAGE, IPMSG_PROTOCOL_MSG_NO_EXTENSION);

	if (rc != 0) {
		goto error_out;
	}

	rc = 0; /* 正常終了 */

error_out:
	return rc;
}

/** IPMSGのIPMSG_RECVMSGを送出する.
 *  @param[in]  con          UDPコネクション情報
 *  @param[in]  orig_msg     受信したメッセージのメッセージ情報
 *  @retval     0            正常終了
 *  @retval    -EINVAL       引数異常
 *  @attention 内部リンケージ
 */
static int
ipmsg_send_recv_msg(const udp_con_t *con, const msg_data_t *orig_msg){
	const char             *ipaddr = NULL;
	int                         rc = 0;
	size_t                     len = 0;
	pktno_t             packet_num = 0;
	ipmsg_send_flags_t local_flags = 0;

	if ( (con == NULL) || (orig_msg == NULL) ) {
		rc = -EINVAL;
		goto error_out;
	}

	/*
	 * フラグを設定する
	 */
	local_flags = (IPMSG_RECVMSG|IPMSG_AUTORETOPT);
	
	/*
	 * ピアのIPアドレスを参照する
	 */
	ipaddr = udp_get_peeraddr(con);
	if (ipaddr == NULL) {
		rc = -ENOENT;
		goto error_out;
	}

	/*
	 * 送信されたパケット番号を取得
	 */
	packet_num = refer_pkt_no_name_from_msg(orig_msg);
	dbg_out("Send recv packet : (ipaddr, pktno) = (%s, %d)\n", 
	    ipaddr, packet_num);

	/*
	 * パケット送信
	 */
	rc = ipmsg_notify_number(con, ipaddr , local_flags, packet_num);
	if (rc != 0) {
		goto error_out;
	}

	rc = 0; /* 正常終了 */

error_out:
	return rc;
}

/** IPMSGのIPMSG_READMSGパケットを処理する
 *  @param[in]  con          UDPコネクション情報
 *  @param[in]  msg          受信したメッセージのメッセージ情報
 *  @retval  0       正常終了
 *  @retval -EINVAL 引数異常
 *                  
 *  @attention 内部リンケージ
 */
static int
ipmsg_proc_read_msg(const udp_con_t *con,const msg_data_t *msg) {
	int            rc = 0;
	pktno_t     pktno = 0;
	gchar       *user = NULL;
	struct timeval tv;

	if (con == NULL) {
		rc = -EINVAL;
		goto error_out;
	}

	gettimeofday(&tv, NULL); /* 受信確認時刻取得  */

	if (msg != NULL) {
		errno = 0;
		pktno = strtol(msg->message, (char **)NULL, 10);
		if (errno == 0) {
			dbg_out("read mssage:seq %ld\n", pkt_no);
		} else {
			war_out("Gan not optain packet number:%s\n", msg->message);
			rc = -errno;
			goto error_out;
		}

		dbg_out("read mssage:seq %s\n", msg->message);
		
		user = g_strdup(refer_user_name_from_msg(msg)); /* ユーザ名獲得 */
	}

	/* 
	 *  メモリ不足の場合やmsgがNULLの場合, ユーザ名にNULLを送信。
	 *  user領域の開放は受け手責任で実施する.
	 */
	read_message_dialog(user, udp_get_peeraddr(con), tv.tv_sec);

	rc = 0; /* 正常終了 */
	
error_out:
	return rc;
}

/** IPMSGのIPMSG_ANSPUBKEYパケットを処理する
 *  @param[in]  con       UDPコネクション情報
 *  @param[in]  msg       受信したメッセージのメッセージ情報
 *  @retval       0       正常終了
 *  @retval      -EINVAL  引数異常
 *  @retval      -ENOSYS  暗号化未サポート
 *  @retval      -ENOENT  ピアのIPアドレスが不明
 *  @attention  内部リンケージ
 */
static int
ipmsg_proc_anspubkey(const udp_con_t *con,const msg_data_t *msg){
	int                 rc = 0;
	char         *pubkey_e = NULL;
	char         *pubkey_n = NULL;
	const char     *ipaddr = NULL;
	ipmsg_cap_t peer_cap = 0;

	if ( (con == NULL) || (msg == NULL) ) {
		rc = -EINVAL;
		goto error_out;
	}

	/*
	 * ピアのIPアドレスを取得する.
	 */
	ipaddr = udp_get_peeraddr(con);
	if (ipaddr == NULL) {
		rc = -ENOENT;
		goto error_out;
	}

#if defined(USE_OPENSSL)
	/*
	 * ANSPUBKEYパケットから公開鍵と秘密鍵を獲得する.
	 */
	rc = pcrypt_crypt_parse_anspubkey(msg->message, &peer_cap,
	    (unsigned char **)&pubkey_e, (unsigned char **)&pubkey_n);
	if (rc != 0) {
		dbg_out("Can not parse anspub key message:rc=%d\n", rc);
		goto error_out;
	}

	/*
	 * ANSPUBKEYパケットに登録された内容で, 公開鍵と秘密鍵を更新する.
	 */
	rc = userdb_replace_public_key_by_addr(ipaddr, peer_cap, pubkey_e, pubkey_n);
	if (rc != 0) {
		dbg_out("Can register parse anspub key message:rc=%d\n", rc);
		goto free_pubkey_out;
	}

	rc = 0; /* 正常終了  */

free_pubkey_out:
	if (pubkey_e != NULL)
		g_free(pubkey_e);
	if (pubkey_n != NULL)
		g_free(pubkey_n);
#else
	rc = -ENOSYS;  /* 暗号化機能未サポート  */
#endif  /*  USE_OPENSSL  */

error_out:
	return rc;  
}

/** IPMSGのIPMSG_GETPUBKEYパケットを処理する
 *  @param[in]  con       UDPコネクション情報
 *  @param[in]  msg       受信したメッセージのメッセージ情報
 *  @retval       0       正常終了
 *  @retval      -EINVAL  引数異常
 *  @attention  内部リンケージ
 */
static int
ipmsg_proc_get_public_key(const udp_con_t *con,const msg_data_t *msg) {
	int                         rc = 0;
	size_t                     len = 0;
	int                      index = 0;
	ipmsg_cap_t           peer_cap = 0;
	ipmsg_send_flags_t local_flags = 0;
	char                  *ans_msg = NULL;
	const char             *ipaddr = NULL;

	dbg_out("here\n");

	if ( (con == NULL) || (msg == NULL) ) {
		rc = -EINVAL;
		goto error_out;
	}

	/*
	 * フラグ設定
	 */
	local_flags = IPMSG_ANSPUBKEY;

	/*
	 * ピアのIPアドレスを取得する.
	 */
	ipaddr = udp_get_peeraddr(con);
	if (ipaddr == NULL) {
		rc = -ENOENT;
		goto error_out;
	}

#if defined(USE_OPENSSL)

	peer_cap = (ipmsg_cap_t)strtoul(msg->message, (char **)NULL, 16);

	dbg_out("Get public key:peer capability %x(%s)\n", peer_cap, msg->message);

	rc = pcrypt_crypt_generate_anspubkey_string(peer_cap, (const char **)&ans_msg);
	if (rc != 0) {
		g_assert(ans_msg == NULL);
		goto error_out;
	}
	/*
	 * パケット送信
	 */
	rc = ipmsg_send_packet(con, ipaddr,IPMSG_PROTOCOL_PKTNUM_AUTO, local_flags, 
	    ans_msg, IPMSG_PROTOCOL_MSG_NO_EXTENSION);
	if (rc != 0) {
		goto free_ans_msg_out;
	}

	rc = 0; /* 正常終了 */

free_ans_msg_out:
	if (ans_msg != NULL)
		g_free(ans_msg);

#else
	rc = -ENOSYS;  /*  暗号化未サポート  */
#endif  /* USE_OPENSSL */

error_out:
	return rc;
}

/** IPMSGのIPMSG_RELEASEFILESパケットを処理する
 *  @param[in]  con       UDPコネクション情報
 *  @param[in]  msg       受信したメッセージのメッセージ情報
 *  @retval       0       正常終了
 *  @retval      -EINVAL  引数異常
 *  @attention  内部リンケージ
 */
static int
ipmsg_proc_release_files_msg(const udp_con_t *con, const msg_data_t *msg) {
	pktno_t pktno = 0;
	int        rc = 0;

	if ( (con == NULL) || (msg == NULL) ) {
		rc = -EINVAL;
		goto error_out;
	}

	/*
	 * 開放する添付ファイルを示すパケット番号を獲得する
	 */
	errno = 0;
	pktno = strtol(msg->message, (char **)NULL, 10);
	if (errno == 0) {
		dbg_out("read mssage:seq %ld\n", pkt_no);
	} else {
		err_out("Can not parse packet no.\n");
		rc = -errno;
		goto error_out;
	}

	dbg_out("release files mssage:seq %d(%s)\n", pktno, msg->message);

	/*
	 * 指定された添付ファイルを開放する
	 */
	rc = release_attach_file_block(pktno, FALSE);
	if (rc < 0) {
		dbg_out("Can not release attach file information:pktno=%d %s (%d)", 
		    pktno, strerror(-rc), -rc);
		goto error_out;
	}

	rc = 0; /* 正常終了 */

error_out:
	return rc;
}

/** IPMSGのIPMSG_RECVMSGパケットを処理する
 *  @param[in]  con          UDPコネクション情報
 *  @param[in]  msg          受信したメッセージのメッセージ情報
 *  @retval  0       正常終了
 *  @retval -EINVAL 引数異常
 *                  
 *  @attention 内部リンケージ
 */
static int
ipmsg_proc_recv_msg(const udp_con_t *con, const msg_data_t *msg) {
	int        rc = 0;
	pktno_t pktno = 0;

	if ( (con == NULL) ||  (msg == NULL) || (msg->message == NULL) ) {
		rc = -EINVAL;
		goto error_out;
	}
	
	/*
	 * パケット番号取得
	 */
	errno = 0;
	pktno = strtol(msg->message, (char **)NULL, 10);
	if (errno == 0) {
		dbg_out("recv mssage:seq %ld\n", pkt_no);
	} else {
		err_out("Can not parse packet no.\n");
		rc = -errno;
		goto error_out;
	}

	dbg_out("recv mssage:seq %ld\n", pkt_no);

	unregister_sent_message(pktno); /*  送信完了パケットを再送登録から除去する  */

	rc = 0; /* 正常終了 */

error_out:
	return rc;
}


/** IPMSGのIPMSG_BR_ISGETLIST2パケットを処理する.
 *  @param[in]  con          UDPコネクション情報
 *  @param[in]  msg          受信メッセージ(未使用)
 *  @retval     0            正常終了
 *  @retval    -EINVAL       引数異常
 *
 *  @attention 内部リンケージ
 */
static int
ipmsg_proc_br_isgetlist2(const udp_con_t *con, const msg_data_t *msg){
	int                         rc = 0;
	ipmsg_send_flags_t local_flags = 0;
	const char             *ipaddr = NULL;

	if ( (con == NULL) || (msg == NULL) ) {
		rc = -EINVAL;
		goto error_out;
	}

	/*
	 * フラグ設定
	 */
	local_flags = IPMSG_OKGETLIST;
	
	/*
	 * ピアのIPアドレスを参照する
	 */
	ipaddr = udp_get_peeraddr(con);
	if (ipaddr == NULL) {
		rc = -ENOENT;
		goto error_out;
	}

	/*
	 * パケット送信
	 */
	rc = ipmsg_send_packet(con, ipaddr, IPMSG_PROTOCOL_PKTNUM_AUTO, local_flags, 
	    IPMSG_PROTOCOL_MSG_NO_MESSAGE, IPMSG_PROTOCOL_MSG_NO_EXTENSION);

	if (rc != 0) {
		goto error_out;
	}

	rc = 0; /* 正常終了 */

error_out:
	return rc;
}

/** IPMSGのIPMSG_GETLISTパケットを処理する.
 *  @param[in]  con          UDPコネクション情報
 *  @param[in]  msg          受信メッセージ(未使用)
 *  @retval     0            正常終了
 *  @retval    -EINVAL       引数異常
 *  @note  IPMSG_ANSLISTを返却する.
 *
 *  IPMSG_ANSLISTのパケット形式:
 *
 *    - メッセージ部: ホストリストメッセージ
 *    - 拡張部      : なし
 *
 *  ホストリストメッセージの形式:
 *  
 *  - ホストリストの内容を1エントリ毎に:で区切って送信する.
 *  - 1エントリの情報は, 以下の順で, デリミタ('\a'(0x13))で区切って送信する.
 *    -# ユーザ名
 *    -# ホスト名
 *    -# IPMSG_ANSLIST(十進で記載:19)
 *    -# IPアドレス
 *    -# ポート番号
 *    -# ニックネーム
 *    -# グループ名
 *
 *  @attention 内部リンケージ
 */
static int
ipmsg_proc_getlist(const udp_con_t *con,const msg_data_t *msg){
	char                *host_list = NULL;
	const char             *ipaddr = NULL;
	ipmsg_send_flags_t local_flags = 0;
	int                     length = 0;
	int                      start = 0;
	int                         rc = 0;

	dbg_out("here\n");

	if ( (con == NULL) || (msg == NULL) ) {
	  rc = -EINVAL;
	  goto error_out;
	}

	/*
	 * フラグ設定
	 */
	local_flags = IPMSG_ANSLIST;
	
	/*
	 * ピアのIPアドレスを参照する
	 */
	ipaddr = udp_get_peeraddr(con);
	if (ipaddr == NULL) {
		rc = -ENOENT;
		goto error_out;
	}

	/*
	 * 返却するホストリスト作成
	 */

	length = G2IPMSG_DEFAULT_HOST_LIST_LEN; /* ホストリスト長設定  */

	/* 要求されたホストリストの返却開始位置を取得する. */
	errno = 0;
	start = strtol(msg->message, (char **)NULL, 10);
	if (errno != 0) {
		err_out("Can not parse IPMSG_GETLIST packet no.\n");
		rc = -errno;
		goto error_out;
	}  

	dbg_out("require:%d(%s)\n", start, msg->message); 

	/* 返却するホストリストを作成する */
	rc = userdb_get_hostlist_string(start, &length, (const char **)&host_list);
	if (rc != 0) {
		g_assert(host_list == NULL);
		goto error_out;
	}

	/*
	 * 送信
	 */
	rc = ipmsg_send_packet(con, ipaddr, IPMSG_PROTOCOL_PKTNUM_AUTO, local_flags, 
	    host_list, IPMSG_PROTOCOL_MSG_NO_EXTENSION);
	if (rc != 0) {
		goto free_hostlist_out;
	}

	rc = 0; /* 正常終了 */

free_hostlist_out:
	if (host_list != NULL)
		g_free(host_list);
error_out:
	return rc;
}

/** ユーザ情報更新要求を処理する
 *  @param[in]  con          UDPコネクション情報
 *  @param[in]  msg          受信メッセージ情報
 *  @param[in]  op_type      操作種別
 *  @retval     0            正常終了
 *  @retval    -EINVAL       引数異常
 *  @retval    -ENOMEM       メモリ不足
 */
static int
ipmsg_protocol_user_opration(const udp_con_t *con, const msg_data_t *msg, 
    const int op_type) {
	int rc = 0;

	g_assert( (con != NULL) && (msg != NULL) ); /* 呼び出し元責任で確認 */

	
	switch(op_type) {
	case IPMSG_PROTOCOL_USR_ADD: /* ユーザ追加 */
		rc = userdb_add_user(con, msg);
		/* 既にユーザが存在する場合は, 
		 * ユーザ情報更新処理に移行し, それ以外の場合は, 復帰する.
		 */
		if (rc != -EEXIST) 
			break;
		/* fall through */
	case IPMSG_PROTOCOL_USR_UPDATE: /* ユーザ情報更新  */
		rc = userdb_update_user(con, msg);
		break;
	case IPMSG_PROTOCOL_USR_DEL:  /* ユーザ情報削除  */
		rc = userdb_del_user(con, msg);
		break;
	case IPMSG_PROTOCOL_ADD_WITH_HOSTLIST: /* ホストリストによる追加 */
		rc = userdb_hostlist_answer_add(con, msg);
		break;		
	default:
		break;
	}

	userdb_print_user_list();

	return rc;
}

/** IPMSGのIPMSG_ANSLISTパケットを処理する.
 *  @param[in]  con          UDPコネクション情報
 *  @param[in]  msg          受信メッセージ(未使用)
 *  @retval     0            正常終了
 *  @retval    -EINVAL       引数異常
 *
 *  @attention 内部リンケージ
 */
static int
ipmsg_proc_anslist(const udp_con_t *con, const msg_data_t *msg){
	int rc= 0;

	dbg_out("here\n");

	if ( (con == NULL) || (msg == NULL) ) {
		rc = -EINVAL;
		goto error_out;
	}

	/*
	 * ANSLISTを元にユーザリストにエントリを登録する
	 */
	rc = ipmsg_protocol_user_opration(con, msg, 
	    IPMSG_PROTOCOL_ADD_WITH_HOSTLIST);
	if (rc != 0) {
		goto error_out;
	}

	rc = 0; /* 正常終了 */

error_out:
	return rc;
}

/** IPMSGのIPMSG_OKGETLISTパケットを処理する.
 *  @param[in]  con          UDPコネクション情報
 *  @param[in]  msg          受信メッセージ(未使用)
 *  @retval     0            正常終了
 *  @retval    -EINVAL       引数異常
 *
 *  @attention 内部リンケージ
 */
static int
ipmsg_proc_okgetlist(const udp_con_t *con, const msg_data_t *msg){
	int             rc = 0;
	const char *ipaddr = NULL;

	dbg_out("here\n");

	if ( (con == NULL) || (msg == NULL) ) {
		rc = -EINVAL;
		goto error_out;
	}

	dbg_out("OK_GETLIST from %s.\n",udp_get_peeraddr(con));
	
	/*
	 * ピアのIPアドレスを参照する
	 */
	ipaddr = udp_get_peeraddr(con);
	if (ipaddr == NULL) {
		rc = -ENOENT;
		goto error_out;
	}

	/*
	 * 最初のエントリからホストリスト要求を発行する
	 */
	rc = ipmsg_send_get_list(udp_con, ipaddr, 0);
	if (rc != 0) {
		goto error_out;
	}

	rc = 0; /* 正常終了 */

error_out:
	return rc;
}

/** IPMSGのエントリ系パケットを送出する.
 *  @param[in]  con          UDPコネクション情報
 *  @param[in]  ipaddr       送出先IPアドレス(NULLの場合は, ブロードキャスト)
 *  @param[in]  flags        パケット送信フラグ
 *  @retval     0            正常終了
 *  @retval    -EINVAL       引数異常
 *  @retval    -ENOMEM       メモリ不足
 *  @note  エントリ系ブロードキャストパケットの形式は, 以下のとおり
 *
 *  IPMSG_BR_ENTRY パケット形式
 *    - メッセージ部: ニックネーム (通常時)
 *                  ニックネーム<空白>'['不在種別']' (不在モード時)
 *    - 拡張部      : グループ名
 *
 *  IPMSG_ANSENTRY パケット形式
 *    - メッセージ部: ニックネーム (通常時)
 *                  ニックネーム<空白>'['不在種別']' (不在モード時)
 *    - 拡張部      : グループ名
 *
 *  IPMSG_BR_EXIT パケット形式
 *    - メッセージ部: ニックネーム (通常時)
 *    - 拡張部      : グループ名
 *
 *  IPMSG_BR_ABSENCE パケット形式
 *    - メッセージ部: ニックネーム (通常時)
 *                  ニックネーム<空白>'['不在種別']' (不在モード時)
 *    - 拡張部      : グループ名
 */
static int
ipmsg_send_entry_packets(const udp_con_t *con, const char *ipaddr, 
    const int flags) {
	int                         rc = 0;
	size_t                     len = 0;
	size_t            msg_part_len = 0;
	ipmsg_send_flags_t local_flags = 0;
	int                  abs_index = 0;
	char             *absent_title = NULL;
	char                  *message = NULL;
	char                *extension = NULL;

	if (con == NULL) {
		rc = -EINVAL;
		goto error_out;
	}

	/*
	 * 送信フラグ作成
	 */
	local_flags  = flags;

	/*
	 * 自発的IPMSG_ANSENTRYパケット以外は, デフォルトのフラグを設定
	 */
	if ( !( (local_flags & IPMSG_ANSENTRY) && 
		(local_flags & hostinfo_get_normal_entry_flags()) ) )
		local_flags |= hostinfo_get_normal_entry_flags();

	dbg_out("Entry Type Flag :%x\n", local_flags);

	/*
	 * IPMSG_BR_EXITは, 不在通知不要.
	 */
	if ( ipmsg_protocol_flags_get_command(flags) == IPMSG_BR_EXIT )
		goto  create_message;

	/*
	 * 不在処理拡張
	 */
	if  (hostinfo_is_ipmsg_absent()) {
		rc = hostinfo_get_absent_id(&abs_index);
		if (rc == 0) {
			hostinfo_get_absent_title(abs_index, 
			    (const char **)&absent_title);
		}
	}

create_message:	
	/*
	 * メッセージ部
	 */
	msg_part_len = 
		(strlen(hostinfo_refer_nick_name())) + /* ニックネーム */
		((absent_title != NULL) ? 
		    (strlen(absent_title) + 
			IPMSG_PROTOCOL_BR_ABSENT_FMT_LEN)  : 
		    (0) ) + 1; /* +1 は, ヌルターミネート */

	message = g_malloc(msg_part_len);
	if (message == NULL) {
		rc = -ENOMEM;
		goto error_out;
	}

	if (absent_title != NULL) 
		snprintf(message, msg_part_len, 
		    IPMSG_PROTOCOL_BR_ABSENT_FMT,
		    hostinfo_refer_nick_name(), (absent_title));
	else
		snprintf(message, msg_part_len, 
		    IPMSG_PROTOCOL_BR_ENTRY_FMT, 
		    hostinfo_refer_nick_name());

	message[msg_part_len - 1] = '\0';


	/*
	 * 拡張部
	 */
	extension = g_strdup(hostinfo_refer_group_name());
	if (extension == NULL) {
		rc = -ENOMEM;
		goto free_message_out;
	}

	/*
	 * パケット送信
	 */
	rc = ipmsg_send_packet(con, IPMSG_PROTOCOL_ENTRY_PKT_ADDR, 
	    IPMSG_PROTOCOL_PKTNUM_AUTO, local_flags, message, extension);
	if (rc != 0) {
		goto free_extension_out;
	}

	rc = 0; /* 正常終了 */

free_extension_out:
	if (extension != NULL)
		g_free(extension);

free_message_out:
	if (message != NULL)
		g_free(message);

error_out:
	return rc;
}


/** IPMSGのIPMSG_ANSENTRYパケットを送出する
 *  @param[in]  con          UDPコネクション情報
 *  @param[in]  flags        パケット送信フラグ
 *  @retval     0            正常終了
 *  @retval    -EINVAL       引数異常
 *  @retval    -ENOMEM       メモリ不足
 *  @note  IPMSG_ANSENTRYパケットの形式は, 以下のとおり
 *
 *  IPMSG_ANSENTRY パケット形式
 *    - メッセージ部: ニックネーム (通常時)
 *                  ニックネーム<空白>'['不在種別']' (不在モード時)
 *    - 拡張部      : グループ名
 */
static int
ipmsg_send_ans_entry_common(const udp_con_t *con, const char *ipaddr, 
    const ipmsg_send_flags_t flags){
	int                         rc = 0;
	ipmsg_send_flags_t local_flags = 0;

	local_flags  = flags;
	local_flags |= IPMSG_ANSENTRY;

	rc = ipmsg_send_entry_packets(con, ipaddr, local_flags);
	if (rc != 0) {
		goto error_out;
	}

	rc = 0; /* 正常終了 */

error_out:	
	return rc;
}

/** IPMSG_BR_ENTRYへの応答として, IPMSG_ANSENTRYパケットを送出する
 *  @param[in]  con          UDPコネクション情報
 *  @param[in]  flags        パケット送信フラグ
 *  @retval     0            正常終了
 *  @retval    -EINVAL       引数異常
 *  @retval    -ENOMEM       メモリ不足
 *  @note  IPMSG_ANSENTRYパケットの形式は, 以下のとおり
 *
 *  IPMSG_ANSENTRY パケット形式
 *    - メッセージ部: ニックネーム (通常時)
 *                  ニックネーム<空白>'['不在種別']' (不在モード時)
 *    - 拡張部      : グループ名
 */
static int
ipmsg_send_ans_entry(const udp_con_t *con,const int flags){
	int                         rc = 0;
	ipmsg_send_flags_t local_flags = 0;

	local_flags  = flags;
	local_flags |= hostinfo_get_normal_entry_flags();

	dbg_out("ANS-Entry Flag :%x\n", local_flags);

	/*
	 * 応答用ANS_ENTRYを返す.
	 */
	rc = ipmsg_send_ans_entry_common(con, udp_get_peeraddr(con), 
	    local_flags);
	if (rc != 0) {
		goto error_out;
	}

	rc = 0; /* 正常終了 */

error_out:
	return rc;
}

/** IPMSGのIPMSG_BR_ABSENCEパケットを処理する
 *  @param[in]  con          UDPコネクション情報
 *  @param[in]  msg          受信メッセージ情報
 *  @retval     0            正常終了
 *  @retval    -EINVAL       引数異常
 *  @retval    -ENOMEM       メモリ不足
 */
static int
ipmsg_proc_br_absence(const udp_con_t *con, const msg_data_t *msg){
	int rc = 0;

	dbg_out("here\n");

	if ( (con == NULL) || (msg == NULL) ) {
		rc = -EINVAL;
		goto error_out;
	}

	dbg_out("\nNew user: \n\tUser:%s Host: %s\n\t"
	        "NickName: %s GroupName:%s\n",
	    refer_user_name_from_msg(msg),
	    refer_host_name_from_msg(msg),
	    refer_nick_name_from_msg(msg),
	    refer_group_name_from_msg(msg));
	
	/* ユーザーリストを更新する */
	rc = ipmsg_protocol_user_opration(con, msg, IPMSG_PROTOCOL_USR_UPDATE);
	if (rc != 0) {
		goto error_out;
	}

	rc = 0; /* 正常終了 */

error_out:
	return rc;
}

/** IPMSGのIPMSG_BR_ENTRYパケットを処理する
 *  @param[in]  con          UDPコネクション情報
 *  @param[in]  msg          受信メッセージ情報
 *  @retval     0            正常終了
 *  @retval    -EINVAL       引数異常
 *  @retval    -ENOMEM       メモリ不足
 */
static int
ipmsg_proc_br_entry(const udp_con_t *con,const msg_data_t *msg) {
	int                   rc = 0;
	const char       *ipaddr = NULL;
	ipmsg_command_t  command = 0;
	ipmsg_cap_t     peer_cap = 0;

	dbg_out("here\n");

	if ( (con == NULL) || (msg == NULL) ) {
		rc = -EINVAL;
		goto error_out;
	}

	ipmsg_send_ans_entry(con, 0);

	/* ユーザーリストを更新する */
	rc = ipmsg_protocol_user_opration(con, msg, IPMSG_PROTOCOL_USR_ADD);
	if (rc != 0) {
		goto error_out;
	}
	

	/*
	 * ピアのIPアドレスを取得する
	 */
	ipaddr = udp_get_peeraddr(con);
	if (ipaddr == NULL) {
		rc = -ENOENT;
		goto error_out;
	}

	/* コマンドオプションを取得  */
	rc = get_command_from_msg(msg, &command, &peer_cap);
	g_assert(rc == 0);

        /*
	 * 暗号化可能なピアの場合, 鍵獲得を試みる  
	 */
	if (peer_cap & IPMSG_ENCRYPTOPT) {
	  ipmsg_send_getpubkey(con, ipaddr);
	}

	rc = 0; /* 正常終了 */

error_out:	
	return rc;
}

/** IPMSGのIPMSG_BR_EXITパケットを処理する
 *  @param[in]  con          UDPコネクション情報
 *  @param[in]  msg          受信メッセージ情報
 *  @retval     0            正常終了
 *  @retval    -EINVAL       引数異常
 *  @retval    -ENOMEM       メモリ不足
 */
static int
ipmsg_proc_br_exit(const udp_con_t *con,const msg_data_t *msg) {
	int rc = 0;

	dbg_out("here\n");

	if ( (con == NULL) || (msg == NULL) ) {
		rc = -EINVAL;
		goto error_out;
	}

	dbg_out("\nLeave user: \n\tUser:%s Host: %s\n\t"
	    "NickName: %s GroupName:%s\n",
	    refer_user_name_from_msg(msg),
	    refer_host_name_from_msg(msg),
	    refer_nick_name_from_msg(msg),
	    refer_group_name_from_msg(msg));

	/* ユーザーリストを更新する */
	rc = ipmsg_protocol_user_opration(con, msg, IPMSG_PROTOCOL_USR_DEL);
	if (rc != 0) {
		goto error_out;
	}

	rc = 0; /* 正常終了 */

error_out:
	return rc;
}

/** IPMSGのIPMSG_ANSENTRYパケットを処理する
 *  @param[in]  con          UDPコネクション情報
 *  @param[in]  msg          受信メッセージ情報
 *  @retval     0            正常終了
 *  @retval    -EINVAL       引数異常
 *  @retval    -ENOMEM       メモリ不足
 */
static int
ipmsg_proc_ans_entry(const udp_con_t *con,const msg_data_t *msg) {
	int                   rc = 0;
	const char       *ipaddr = NULL;
	ipmsg_command_t  command = 0;
	ipmsg_cap_t     peer_cap = 0;

	dbg_out("here\n");

	if ( (con == NULL) || (msg == NULL) ) {
		rc = -EINVAL;
		goto error_out;
	}

	dbg_out("\nNew user: \n\tUser:%s Host: %s\n\t"
	    "NickName: %s GroupName:%s\n",
	    refer_user_name_from_msg(msg),
	    refer_host_name_from_msg(msg),
	    refer_nick_name_from_msg(msg),
	    refer_group_name_from_msg(msg));

	/* ユーザーリストを更新する */
	rc = ipmsg_protocol_user_opration(con, msg, IPMSG_PROTOCOL_USR_ADD);
	if (rc != 0) {
		goto error_out;
	}

	/*
	 * ピアのIPアドレスを取得する
	 */
	ipaddr = udp_get_peeraddr(con);
	if (ipaddr == NULL) {
		rc = -ENOENT;
		goto error_out;
	}

	/* コマンドオプションを取得  */
	rc = get_command_from_msg(msg, &command, &peer_cap);
	g_assert(rc == 0);

        /*
	 * 暗号化可能なピアの場合, 鍵獲得を試みる  
	 */
	if (peer_cap & IPMSG_ENCRYPTOPT) {
	  ipmsg_send_getpubkey(con, ipaddr); 
	}

	rc = 0; /* 正常終了 */

error_out:
	return rc;
}
/** IPMSGの情報系/メッセージ送信系パケットを送出する
 *  @param[in]  con          UDPコネクション情報
 *  @param[in]  ipaddr       送信先IPアドレス
 *  @param[in]  flags        送信フラグ
 *  @param[in]  pktno        パケット番号
 *  @param[in]  message      メッセージ部
 *  @param[in]  extension    拡張部
 *  @retval     0            正常終了
 *  @retval    -EINVAL       引数異常
 *  @retval    -ENOMEM       メモリ不足
 *  @note 情報系/メッセージ送信系パケットの形式は, 以下のとおり.
 *
 *  IPMSG_SENDABSENCEINFO パケット形式
 *    - メッセージ部: 不在通知文
 *    - 拡張部      : なし
 *
 *  IPMSG_SENDINFO パケット形式
 *    - メッセージ部: 版数情報(自由形式)
 *    - 拡張部      : なし
 *
 *  IPMSG_SENDMSG パケット形式
 *    - メッセージ部: メッセージ本文
 *    - 拡張部      : 添付ファイル文字列
 */
static int
ipmsg_send_sendmsg_packets(const udp_con_t *con, const char *ipaddr, 
    int flags, int pkt_no, const char *message, const char *extension) {
	int                              rc = 0;
	size_t                          len = 0;
	char                  *sent_message = NULL;
	char             *converted_message = NULL;
	size_t                 sent_msg_len = 0;
	gchar             *internal_message = NULL;
	ipmsg_cap_t                peer_cap = 0;
	ipmsg_cap_t          peer_crypt_cap = 0;
	ipmsg_send_flags_t      local_flags = 0;

	if ( (con == NULL) || (ipaddr == NULL) || (message == NULL) ) {
		rc = -EINVAL;
		goto errot_out;
	}
	
	/*
	 * フラグ設定
	 */
	local_flags  = flags;

	/*
	 * フラグ値加工
	 */
	if ( flags & IPMSG_PROTOCOL_INFOMSG_TYPE ) {
		/* 情報系パケットにはSENDMSGおよび暗号化フラグをつけない */
		local_flags &= ~(IPMSG_SENDMSG| IPMSG_ENCRYPTOPT); 
		local_flags |= (IPMSG_NO_REPLY_OPTS|IPMSG_AUTORETOPT); /* 自動送信フラグ */
	} else {
		local_flags |= IPMSG_SENDMSG;
	}

	/* ファイル添付を指定されたが, 拡張部が無い場合, ファイル添付を中止 */
	if ( (local_flags & IPMSG_FILEATTACHOPT) && (extension == NULL) ) {
		local_flags &= ~IPMSG_FILEATTACHOPT;
		war_out("%s\n%s",
		    _("No extension  in file attached message."
			"Ignore attachments."));
	}

	sent_message = (char *)message; /* 平文を送ることを仮定 */

	/*
	 * 暗号化通信パケットの処理
	 */
#if defined(USE_OPENSSL)
	if (local_flags & IPMSG_ENCRYPTOPT) { /* 暗号化メッセージを送る場合 */

		/*
		 * ピアの暗号化能力を参照し, 暗号化不可能の場合は, 暗号化を
		 * 取りやめる.
		 */
		rc = userdb_get_cap_by_addr(ipaddr, &peer_cap, &peer_crypt_cap);

		if (rc != 0) {		/* 暗号化能力不明のため暗号化しないで抜ける. */
			goto cancel_encryption;
		}
		if ( ( !(peer_cap & IPMSG_ENCRYPTOPT) ) || (peer_crypt_cap == 0) ) {
			/* 未だ暗号化に必要な情報が得られていない 
			 * 次回に備えて取得要求を発行し, 今回は暗号化せずに送る.
			 */
			ipmsg_send_getpubkey(con, ipaddr); 
			goto cancel_encryption;
		}

		if (peer_cap & IPMSG_ENCRYPTOPT) {
			/*
			 * 暗号化を実施する場合は, 先にエンコード変換が必要
			 */
			rc = ipmsg_convert_string_external(ipaddr, message, 
			    (const gchar **)&converted_message);
			if (rc != 0) {
				ipmsg_err_dialog("%s:%s\n", 
				    _("Can not convert common into external representation"), 
				    message);
				goto cancel_encryption; /* 平文送信  */
			}

			/*
			 * 暗号化可能なピアについては, 暗号化を実施
			 */
			dbg_out("Peer's cap=%x\n", peer_cap);
			rc = ipmsg_encrypt_message(ipaddr, converted_message, 
			    (unsigned char **)&sent_message, &sent_msg_len);
			if (rc != 0) { /*  暗号化失敗 */
				goto cancel_encryption;
			}

			dbg_out("Encrypted message.%s\n", sent_message);
			/* 暗号化に成功した場合は, この時点で送信ログを
			 * 記録する 
			*/
			logfile_send_log(ipaddr, local_flags, message);
			goto end_encryption;
		}
	}

cancel_encryption:
	/*
	 * 暗号化未対応なピアについては, 
	 * 暗号化処理をせずに抜ける
	 */
	dbg_out("Peer can not handle crypted message.rc=%d cap=%x\n", 
	    rc, peer_cap);
	local_flags &= ~IPMSG_ENCRYPTOPT;
	sent_message = (char *)message; /* 平文を送る */
end_encryption:
#else
	/*
	 * 暗号化をせずに送信
	 */
	flags &= ~IPMSG_ENCRYPTOPT;
#endif  /*  USE_OPENSSL  */


	/*
	 * パケット送信
	 */
	rc = ipmsg_send_packet(con, ipaddr, pkt_no, local_flags, 
	    sent_message, extension);

	if (rc != 0) {
		goto free_internal_message_out;
	}

	rc = 0; /* 正常終了 */

free_internal_message_out:
	if (internal_message == NULL)
		g_free(internal_message);

free_sent_message_out:
	if ( (message != sent_message) && (sent_message != NULL) )
		g_free(sent_message);

	if (converted_message != NULL)
		g_free(converted_message);
errot_out:
	return rc;
}

/** IPMSGのIPMSG_SENDABSENCEINFOパケットを送出する
 *  @param[in]  con          UDPコネクション情報
 *  @param[in]  flags        パケット送信フラグ
 *  @retval     0            正常終了
 *  @retval    -EINVAL       引数異常
 *  @retval    -ENOMEM       メモリ不足
 */
static int
ipmsg_send_absence_msg(const udp_con_t *con, const int flags) {
	char        *absence_message = NULL;
	const char          *ipaddr = NULL;
	int             local_flags = 0;
	int                      rc = 0;

	if (con == NULL) {
	  rc = -EINVAL;
	  goto no_need_free_out;
	}

	/*
	 * フラグを設定する
	 */
	local_flags  = flags;
	local_flags |= (IPMSG_NO_REPLY_OPTS|IPMSG_AUTORETOPT);

	/*
	 * ピアのIPアドレスを取得する
	 */
	ipaddr = udp_get_peeraddr(con);
	if (ipaddr == NULL) {
		rc = -ENOENT;
		goto no_need_free_out;
	}
		
	/*
	 * 不在通知電文を獲得する
	 */
	rc = hostinfo_get_current_absence_message(&absence_message);
	if (rc != 0) {
		g_assert(absence_message == NULL);
		goto no_need_free_out;
	}

	rc = ipmsg_send_sendmsg_packets(con, ipaddr, local_flags, 
	    IPMSG_PROTOCOL_PKTNUM_AUTO, 
	    absence_message, IPMSG_PROTOCOL_MSG_NO_EXTENSION);
	if (rc != 0) {
		goto free_absence_message;
	}

	dbg_out("Absence message:%s\n", absence_message);

	rc = 0; /* 正常終了 */

free_absence_message:
	if (absence_message != NULL)
		g_free(absence_message);

no_need_free_out:
	return rc;
}

/** IPMSGのIPMSG_SENDABSENCEINFO/IPMSG_SENDINFOパケットを処理する
 *  @param[in]  con          UDPコネクション情報
 *  @param[in]  msg          受信したメッセージ情報
 *  @retval     0            正常終了
 *  @retval    -EINVAL       引数異常
 *  @retval    -ENOMEM       メモリ不足
 */
static int
ipmsg_proc_send_info_packets(const udp_con_t *con, const msg_data_t *msg) {
	int                           rc = 0;
	ipmsg_command_t received_command = 0;
	gchar                 *peer_name = NULL;

	dbg_out("here\n");

	g_assert( (con != NULL) && (msg != NULL) ); /* 呼び出し元責任で確認 */

	dbg_out("send absence info:version %s\n", msg->message);

	/*
	 * 受信したコマンドを取得
	 */
	received_command = IPMSG_NOOPERATION;
	rc = get_command_from_msg(msg, &received_command, NULL);
	if (rc != 0)
		goto error_out;

	/* ユーザ名をメッセージから抽出する  */
	peer_name = g_strdup(refer_user_name_from_msg(msg));
	if (peer_name == NULL) {
		rc = -ENOMEM;
		goto error_out;
	}

	/*  
	 * 受信したメッセージを処理
	 * peer_nameの開放は, 受け手で実施  
	 */
	switch (received_command) {
	case IPMSG_SENDABSENCEINFO:
		info_message_window(peer_name, udp_get_peeraddr(con), 
		    IPMSG_SENDABSENCEINFO, msg->message);
		break;
	case IPMSG_SENDINFO:
		info_message_window(peer_name, udp_get_peeraddr(con), 
		    IPMSG_SENDINFO, msg->message);		
		break;
	default:
		break;
	}

	rc = 0; /* 正常終了 */

error_out:
	return rc;
}

/** IPMSGのIPMSG_SEND_INFOパケットを処理する
 *  @param[in]  con          UDPコネクション情報
 *  @param[in]  msg          受信したメッセージ情報
 *  @retval     0            正常終了
 *  @retval    -EINVAL       引数異常
 *  @retval    -ENOMEM       メモリ不足
 */
static int
ipmsg_proc_send_info(const udp_con_t *con, const msg_data_t *msg) {
	int rc = 0;

	dbg_out("here\n");

	if ( (con == NULL) || (msg == NULL) ) {
		rc = -EINVAL;
		goto error_out;
	}

	dbg_out("send info:version %s\n", msg->message);

	rc = ipmsg_proc_send_info_packets(con, msg);
	if (rc != 0) {
		goto error_out;
	}

	rc = 0; /* 正常終了 */

error_out:
	return rc;
}

/** IPMSGのIPMSG_SENDABSENCEINFOパケットを処理する
 *  @param[in]  con          UDPコネクション情報
 *  @param[in]  msg          受信したメッセージ情報
 *  @retval     0            正常終了
 *  @retval    -EINVAL       引数異常
 *  @retval    -ENOMEM       メモリ不足
 */
static int
ipmsg_proc_send_absence_info(const udp_con_t *con, const msg_data_t *msg) {
	int rc = 0;
	
	dbg_out("here\n");

	if ( (con == NULL) || (msg == NULL) ) {
		rc = -EINVAL;
		goto error_out;
	}

	dbg_out("send absence info %s\n", msg->message);

	rc = ipmsg_proc_send_info_packets(con, msg);
	if (rc != 0) {
		goto error_out;
	}

	rc = 0; /* 正常終了 */

error_out:
	return rc;
}

/** IPMSGのIPMSG_GETABSENCEINFOパケットを処理する
 *  @param[in]  con          UDPコネクション情報
 *  @param[in]  msg          受信したメッセージのメッセージ情報
 *  @retval       0          正常終了
 *  @retval      -EINVAL     引数異常
 *  @attention 内部リンケージ
 */
static int
ipmsg_proc_get_absence_info(const udp_con_t *con, const msg_data_t *orig_msg) {
	ipmsg_send_flags_t local_flags = 0;
	int                         rc = 0;

	dbg_out("here\n");

	if (con == NULL) {
		rc = -EINVAL;
		goto error_out;
	}

	/*
	 * フラグ作成
	 */
	local_flags  = hostinfo_get_normal_send_flags();
	local_flags |= IPMSG_SENDABSENCEINFO;

	/*
	 * 不在通知文送信
	 */
	rc = ipmsg_send_absence_msg(con, local_flags);
	if (rc != 0)
		goto error_out;

	rc = 0; /* 正常終了 */

error_out:
	return rc;
}

/** IPMSGのIPMSG_GETINFOパケットを処理する
 *  @param[in]  con          UDPコネクション情報
 *  @param[in]  orig_msg     受信メッセージ(未使用)
 *  @retval     0            正常終了
 *  @retval    -EINVAL       引数異常
 *  @retval    -ENOENT       ピアのIPアドレスを獲得できなかった.
 *  @retval    -ENOMEM       メモリ不足
 *  @note  IPMSG_SENDINFOを送出する.
 *  @attention 内部リンケージ
 */
static int
ipmsg_proc_get_info(const udp_con_t *con, const msg_data_t *orig_msg){
	char           *version_string = NULL;
	const char             *ipaddr = NULL;
	ipmsg_send_flags_t local_flags = 0;
	size_t                     len = 0;
	int                         rc = 0;

	dbg_out("here\n");

	if (con == NULL) {
		rc = -EINVAL;
		goto error_out;
	}
	
	/*
	 * フラグ設定
	 */
	local_flags  = IPMSG_SENDINFO;

	/*
	 * ピアのIPアドレスを取得する
	 */
	ipaddr = udp_get_peeraddr(con);
	if (ipaddr == NULL) {
		rc = -ENOENT;
		goto error_out;
	}
	
	/*
	 * 返却文字列長を算出する
	 * 書式指定文字列の長さをそのまま足しているので, 実際に必要な分より少し多めに
	 * 取っている.
	 */
	
	len = 0;
	len += strlen(IPMSG_PROTOCOL_SENDINFO_FMT);
	len += strlen(_("Edition"));
	len += strlen(PACKAGE);
	len += strlen(_("Version"));
	len += strlen(VERSION);
	len += 1; /* ヌルターミネート */

	/*
	 * 領域割り付け
	 */
	version_string = g_malloc(len);
	if (version_string == NULL) {
		rc = -ENOMEM;
		goto error_out;
	}

	/*
	 * 版数文字列作成
	 */
	snprintf(version_string, len, IPMSG_PROTOCOL_SENDINFO_FMT,
	    _("Edition"), PACKAGE, _("Version"), VERSION);


	dbg_out("Version string:%s\n", version_string);

	/*
	 * 送信
	 */
	rc = ipmsg_send_sendmsg_packets(con, ipaddr, 
	    local_flags, IPMSG_PROTOCOL_PKTNUM_AUTO, 
	    version_string, IPMSG_PROTOCOL_MSG_NO_EXTENSION);
	if (rc != 0) {
		goto free_version_string_out;
	}

	rc = 0;

free_version_string_out:
	if (version_string != NULL)
		g_free(version_string);

error_out:
	return rc;
}

/** IPMSG_SENDMSGパケットを処理する.
 *  @param[in]  con          UDPコネクション情報
 *  @param[in]  flags        パケット送信フラグ
 *  @retval     0            正常終了
 *  @retval    -EINVAL       引数異常
 *  @retval    -ENOMEM       メモリ不足
 *  @note  
 */
static int
ipmsg_proc_send_msg(const udp_con_t *con, const msg_data_t *msg) {
	gchar *internal_message = NULL;
	const gchar     *ipaddr = NULL;
	int         local_flags = 0;
	int                  rc = 0;

	/*
	 * 文字列変換のためにピアのIPアドレスを取得する.
	 */
	ipaddr = udp_get_peeraddr(con);
	if (ipaddr == NULL) {
		rc = -EINVAL;
		goto error_out;
	}

	/*
	 * 文字列を内部形式に変換する.
	 */
	rc = ipmsg_convert_string_internal(ipaddr, msg->message, 
	    (const gchar **)&internal_message);
	if (rc != 0) {
		ipmsg_err_dialog(_("Can not convert message from %s into ineternal representation"), ipaddr);
		goto error_out;
	}

	dbg_out("message:\n%s\n", internal_message);

	/*
	 * ログを取らないように要求している場合を除いてログを設定する.
	 * 鍵付き封書のログ処理抑制は, ログ処理機構で実施
	 * (IPMSGのプロトコル上の仕様ではないため).
	 */
	if ( !(msg->command_opts & IPMSG_NOLOGOPT) )
		logfile_recv_log(ipaddr, msg->command_opts, internal_message);


	/*
	 * 返信抑制フラグが付いている場合は, 返信に関わる処理を実施しない.
	 */
	if ( msg->command_opts & IPMSG_NO_REPLY_OPTS )
		goto show_receive_win;

	/*
	 * 送受信確認チェックが付いているメッセージの場合, 
	 * 受信確認パケットを送信する.
	 */
	if ( msg->command_opts & IPMSG_SENDCHECKOPT )
		ipmsg_send_recv_msg(con, msg);

	/*
	 * 不在の場合は, 不在通知を実施する.
	 * 
	 */
	if ( hostinfo_is_ipmsg_absent() ) {

		local_flags = hostinfo_get_normal_send_flags();

		if (hostinfo_refer_ipmsg_default_secret())
			local_flags |= IPMSG_SECRETOPT;

		ipmsg_send_absence_msg(con, local_flags);
	}

show_receive_win:
	/*
	 * 受信ウィンドウを生成する.
	 */
	if (hostinfo_refer_ipmsg_default_popup()) 
		store_message_window(msg, ipaddr);
	else
		recv_message_window(msg, ipaddr);

	rc = 0; /* 正常終了 */

free_internal_message_out:
	if (internal_message != NULL)
		g_free(internal_message);

error_out:
	return rc;
}

/** 送信パケットのパケット番号を取得する.
 *  @retval  パケット番号
 */
pktno_t
ipmsg_get_pkt_no(void) {
	pktno_t this_pkt_no = 0;
	
	/*
	 * 自動パケット番号生成とぶつからない番号を生成する  
	 */

	g_static_mutex_lock(&pktno_mutex);

	do{
		++pkt_no;
		this_pkt_no = (time(NULL) % 100000 + pkt_no);
	} while (this_pkt_no == IPMSG_PROTOCOL_PKTNUM_AUTO); 

	g_static_mutex_unlock(&pktno_mutex);

	return this_pkt_no;
}

/** 自発的なIPMSG_ANSENTRYパケットを送出する
 *  不正暗号化電文受信時に, 暗号化能力をキャンセルするのに使用する.
 *  @param[in]  con          UDPコネクション情報
 *  @param[in]  flags        パケット送信フラグ
 *  @retval     0            正常終了
 *  @retval    -EINVAL       引数異常
 *  @retval    -ENOMEM       メモリ不足
 *  @note  IPMSG_ANSENTRYパケットの形式は, 以下のとおり
 *
 *  IPMSG_ANSENTRY パケット形式
 *    - メッセージ部: ニックネーム (通常時)
 *                  ニックネーム<空白>'['不在種別']' (不在モード時)
 *    - 拡張部      : グループ名
 */
int
ipmsg_send_gratuitous_ans_entry(const udp_con_t *con, const char *ipaddr, 
    const int flags){
	int rc = 0;

	if ( (con == NULL) || (ipaddr == NULL) )
		return -EINVAL;

	/*  無条件に送りつけるANS_ENTRYなので, flagsは呼び出し側責任で設定する.
	 *  これは, 暗号化通信のキャンセルなどで使用する.
	 */

	dbg_out("Send gratutous ANS_ENTRY to %s with 0x%08x\n", ipaddr, flags);

	rc = ipmsg_send_ans_entry_common(con, ipaddr, flags);
	if (rc != 0) {
		goto error_out;
	}

	rc = 0; /* 正常終了 */

error_out:
	return rc;
}

/** IPMSGのIPMSG_BR_ENTRYパケットを送出する.
 *  @param[in]  con          UDPコネクション情報
 *  @param[in]  flags        パケット送信フラグ
 *  @retval     0            正常終了
 *  @retval    -EINVAL       引数異常
 *  @retval    -ENOMEM       メモリ不足
 *  @note  
 *  IPMSG_BR_ENTRY パケット形式
 *    - メッセージ部: ニックネーム (通常時)
 *                  ニックネーム<空白>'['不在種別']' (不在モード時)
 *    - 拡張部      : グループ名
 */
int
ipmsg_send_br_entry(const udp_con_t *con, const int flags){
	int                         rc = 0;
	ipmsg_send_flags_t local_flags = 0;

	if (con == NULL) {
		rc = -EINVAL;
		goto error_out;
	}

	/*
	 * 設定されている場合は, ホストリストを要求
	 */
	if (hostinfo_refer_ipmsg_is_get_hlist())
		ipmsg_send_br_isgetlist2(con, flags);

	local_flags  = flags;
	local_flags |= IPMSG_BR_ENTRY;

	dbg_out("BR-Entry Flag :%x\n", local_flags);

	rc = ipmsg_send_entry_packets(con, IPMSG_PROTOCOL_ENTRY_PKT_ADDR, 
	    local_flags);
	if (rc != 0) {
		goto error_out;
	}

	rc = 0; /* 正常終了 */

error_out:
	return rc;
}

/** IPMSGのIPMSG_BR_EXITパケットを送出する.
 *  @param[in]  con          UDPコネクション情報
 *  @param[in]  flags        パケット送信フラグ
 *  @retval     0            正常終了
 *  @retval    -EINVAL       引数異常
 *  @retval    -ENOMEM       メモリ不足
 *  @note  
 *  IPMSG_BR_EXIT パケット形式
 *    - メッセージ部: ニックネーム (通常時)
 *    - 拡張部      : グループ名
 */
int
ipmsg_send_br_exit(const udp_con_t *con, const int flags){
	int                         rc = 0;
	ipmsg_send_flags_t local_flags = 0;

	local_flags  = flags;
	local_flags |= IPMSG_BR_EXIT;

	dbg_out("BR-Exit Flag :%x\n", local_flags);

	rc = ipmsg_send_entry_packets(con, IPMSG_PROTOCOL_ENTRY_PKT_ADDR, 
	    local_flags);
	if (rc != 0) {
		goto error_out;
	}

	rc = 0; /* 正常終了 */

error_out:
	return rc;
}

/** IPMSGのIPMSG_BR_ABSENCEパケットを送出する.
 *  @param[in]  con          UDPコネクション情報
 *  @param[in]  flags        パケット送信フラグ
 *  @retval     0            正常終了
 *  @retval    -EINVAL       引数異常
 *  @retval    -ENOMEM       メモリ不足
 *  @note  
 *  IPMSG_BR_ABSENCE パケット形式
 *    - メッセージ部: 
 *                  - ニックネーム (通常時)
 *                  - ニックネーム<空白>'['不在種別']' (不在モード時)
 *    - 拡張部      : グループ名
 */
int
ipmsg_send_br_absence(const udp_con_t *con, const int flags) {
	int                         rc = 0;
	ipmsg_send_flags_t local_flags = 0;

	local_flags  = flags;
	local_flags |= IPMSG_BR_ABSENCE;

	dbg_out("BR-Absence Flag :%x\n", local_flags);

	rc = ipmsg_send_entry_packets(con, IPMSG_PROTOCOL_ENTRY_PKT_ADDR,
	    local_flags);
	if (rc != 0) {
		goto error_out;
	}

	rc = 0; /* 正常終了 */

error_out:
	return rc;
}

/** IPMSGのファイル取得メッセージ(外部形式)を作成する
 *  @param[in]  con                 UDPコネクション情報
 *  @param[in]  ipaddr              送信先IPアドレス
 *  @param[in]  ftype               ファイル種別
 *  @param[in]  msg_string          ファイル取得メッセージ部
 *  @param[in]  flags               パケット送信フラグ
 *  @param[out] returned_len        ファイル取得メッセージ長の返却領域
 *  @param[out] returned_message    ファイル取得メッセージの返却領域
 *  @retval     0                   正常終了
 *  @retval    -EINVAL              引数異常
 *  @retval    -ENOMEM              メモリ不足
 */
int
ipmsg_construct_getfile_message(const tcp_con_t *con, const char *ipaddr, ipmsg_ftype_t ftype, const char *msg_string, ipmsg_send_flags_t flags, size_t *returned_len, char **returned_message){
	int                         rc = 0;
	ipmsg_send_flags_t local_flags = 0;
	char                   *packet = NULL;
	size_t              packet_len = 0;
	pktno_t            sent_pkt_no = 0;

	if ( (con == NULL) || (ipaddr == NULL) || (msg_string == NULL) || 
	    (returned_len == NULL) || (returned_message == NULL) ) {
		rc = -EINVAL;
		goto error_out;
	}
	
	/*
	 * フラグ設定
	 */
	local_flags  = flags;
	local_flags |= hostinfo_get_normal_send_flags();
	local_flags |= ( (ftype == IPMSG_FILE_DIR) ? 
	    (IPMSG_GETDIRFILES) :  (IPMSG_GETFILEDATA) );

	/*
	 * パケット生成(外部形式)
	 */
	rc = build_ipmsg_packet(ipaddr, IPMSG_PROTOCOL_PKTNUM_AUTO, local_flags, 
	    msg_string, IPMSG_PROTOCOL_MSG_NO_EXTENSION, 
	    &packet, &packet_len, IPMSG_PROTOCOL_MSG_NONEED_PKTNO);	
	if ( rc != 0 ) {
		goto error_out;
	}

	/*
	 * 返却
	 */

	*returned_message = packet;
	*returned_len = packet_len; 

	rc = 0; /* 正常終了 */

error_out:
	return rc;
}

/** IPMSGのIPMSG_SENDMSGパケットを送出する
 *  @param[in]  con          UDPコネクション情報
 *  @param[in]  ipaddr       送信先IPアドレス
 *  @param[in]  flags        送信フラグ
 *  @param[in]  pktno        パケット番号
 *  @param[in]  message      メッセージ本文
 *  @param[in]  extension    拡張部
 *  @retval     0            正常終了
 *  @retval    -EINVAL       引数異常
 *  @retval    -ENOMEM       メモリ不足
 */
int
ipmsg_send_send_msg(const udp_con_t *con, const char *ipaddr, 
    int flags, int pkt_no, const char *message, const char *extension){
	int                         rc = 0;

	if ( (con == NULL) || (ipaddr == NULL) || (message == NULL) ) {
		rc = -EINVAL;
		goto error_out;
	}
	
	/* メッセージ送信  */
	rc = ipmsg_send_sendmsg_packets(con, ipaddr, flags, pkt_no, message, extension);
	if (rc != 0) {
		goto error_out;
	}

	/*
	 * 送信ログを記録(自動送信パケットは記録しない).
	 */
	if ( (ipmsg_protocol_flags_get_command(flags) == IPMSG_SENDMSG) && 
	     ( !(flags & (IPMSG_AUTORETOPT|IPMSG_ENCRYPTOPT)) ) ) {
		/* ロギングはオプションであるため, 失敗しても通信は, 正常終了させる */
		logfile_send_log(ipaddr, flags, message);
	}	

	rc = 0; /* 正常終了  */

error_out:
	return rc;
}

/** IPMSGのIPMSG_RELEASEFILESパケットを送出する
 *  @param[in]  con          UDPコネクション情報
 *  @param[in]  ipaddr       送信先IPアドレス
 *  @param[in]  pktno        開放するファイルのパケット番号
 *  @retval     0            正常終了
 *  @retval    -EINVAL       引数異常
 *  @retval    -ENOMEM       メモリ不足
 */
int
ipmsg_send_release_files(const udp_con_t *con, const char *ipaddr, int pktno){
	int                         rc = 0;
	ipmsg_send_flags_t local_flags = 0;

	dbg_out("here");

	if ( (con == NULL) || (ipaddr == NULL) ) {
		rc = -EINVAL;
		goto error_out;
	}

	/*
	 * フラグ設定
	 */
	local_flags = IPMSG_RELEASEFILES;

	dbg_out("Send release file pktno=%d\n", pktno);

	/*
	 * パケット送信
	 */
	rc = ipmsg_notify_number(con, ipaddr , local_flags, pktno);
	if (rc != 0) {
		goto error_out;
	}

	rc = 0; /* 正常終了  */

error_out:
	return rc;
}

/** IPMSGのIPMSG_GETLISTパケットを送出する.
‘ *  @param[in]  con          UDPコネクション情報
 *  @param[in]  ipaddr       送信先IPアドレス
 *  @param[in]  index        取得開始ホストリストエントリ番号
 *  @retval     0            正常終了
 *  @retval    -EINVAL       引数異常
 *
 */
int
ipmsg_send_get_list(const udp_con_t *con, const char *ipaddr, const int index) {
	int                         rc = 0;
	ipmsg_send_flags_t local_flags = 0;

	dbg_out("here");

	if ( (con == NULL) || (ipaddr == NULL) ) {
		rc = -EINVAL;
		goto error_out;
	}

	/*
	 * フラグ設定
	 */
	local_flags = IPMSG_GETLIST;

	dbg_out("Send getlist index = %d\n", index);

	/*
	 * パケット送信
	 */
	rc = ipmsg_notify_number(con, ipaddr , local_flags, index);
	if (rc != 0) {
		goto error_out;
	}

	rc = 0; /* 正常終了  */

error_out:
	return rc;
}

/** IPMSGのIPMSG_READMSGを送出する.
 *  @param[in]  con          UDPコネクション情報
 *  @param[in]  ipaddr       送信先IPアドレス
 *  @param[in]  pkt_num      開封済みメッセージのパケット番号
 *  @retval     0            正常終了
 *  @retval    -EINVAL       引数異常
 *  @attention 内部リンケージ
 */
int
ipmsg_send_read_msg(const udp_con_t *con, const char *ipaddr, pktno_t pkt_num){
	int                         rc = 0;
	size_t                     len = 0;
	ipmsg_send_flags_t local_flags = 0;

	if  (con == NULL)  {
		rc = -EINVAL;
		goto error_out;
	}

	/*
	 * フラグを設定する
	 */
	local_flags = (IPMSG_READMSG|IPMSG_AUTORETOPT);
	
	dbg_out("Send readmsg packet : (ipaddr, pkt_num) = (%s, %d)\n",  ipaddr, pkt_num);

	/*
	 * パケット送信
	 */
	rc = ipmsg_notify_number(con, ipaddr , local_flags, pkt_num);
	if (rc != 0) {
		goto error_out;
	}

	rc = 0; /* 正常終了 */

error_out:
	return rc;
}

/** IPMSGの情報系パケット取得要求を送出する.
 *  @param[in]  con          UDPコネクション情報
 *  @param[in]  ipaddr       送信先IPアドレス
 *  @param[in]  command      情報系コマンド
 *  @retval     0            正常終了
 *  @retval    -EINVAL       引数異常
 *
 *  IPMSG_GETABSENCEINFO パケット形式
 *    - メッセージ部: なし
 *    - 拡張部      : なし
 *
 *  IPMSG_GETINFO パケット形式
 *    - メッセージ部: なし
 *    - 拡張部      : なし
 */
int
ipmsg_send_get_info_msg(const udp_con_t *con,const char *ipaddr,ipmsg_command_t command){
	int                         rc = 0;
	size_t                     len = 0;
	ipmsg_send_flags_t local_flags = 0;

	if ( (con == NULL) || (ipaddr == NULL) || 
	    ( !(command & IPMSG_PROTOCOL_GETINFO_TYPE) ) ) {
		rc = -EINVAL;
		goto error_out;
	}

	g_assert( ( (command & IPMSG_GETINFO) == IPMSG_GETINFO) || 
	    ( (command & IPMSG_GETABSENCEINFO) == IPMSG_GETABSENCEINFO) ) ;

	/*
	 * フラグ設定
	 */
	local_flags = command;
	
	/*
	 * パケット送信
	 */
	rc = ipmsg_send_packet(con, ipaddr, IPMSG_PROTOCOL_PKTNUM_AUTO, local_flags, 
	    IPMSG_PROTOCOL_MSG_NO_MESSAGE, IPMSG_PROTOCOL_MSG_NO_EXTENSION);

	if (rc != 0) {
		goto error_out;
	}

	rc = 0; /* 正常終了 */

error_out:
  return rc;
}

/** IPMSGのIPMSG_GETPUBKEYパケットを送出する.
 *  @param[in]  con          UDPコネクション情報
 *  @param[in]  ipaddr       送信先IPアドレス
 *  @retval     0            正常終了
 *  @retval    -EINVAL       引数異常
 *
 *  IPMSG_GETPUBKEY パケット形式
 *    - メッセージ部: 暗号化ケイパビリティ(16進)を文字列で表した数値
 *    - 拡張部      : なし
 */
int
ipmsg_send_getpubkey(const udp_con_t *con, const char *ipaddr){
	char               *cap_string = NULL;
	char              *send_string = NULL;
	int                         rc = 0;
	size_t                     len = 0;
	ipmsg_send_flags_t local_flags = 0;
	ipmsg_cap_t         capability = 0;

	dbg_out("here\n");

	if ( (con == NULL) || (ipaddr == NULL) ) {
		rc = -EINVAL;
		goto error_out;
	}

#if !defined(USE_OPENSSL)
	rc = -ENOSYS;  /* 未サポート */
	goto error_out;
#else

	/*
	 * フラグ設定
	 */
	local_flags = IPMSG_GETPUBKEY;

	/*
	 * 暗号化能力を文字列化
	 */
	capability = hostinfo_get_ipmsg_crypt_capability();
	rc = pcrypt_crypt_generate_getpubkey_string(capability, 
	    (const char **)&cap_string);
	if (rc != 0) {
		g_assert(cap_string == NULL);
		goto error_out;
	}

	dbg_out("Send GETPUBKEY Capability:%s\n", cap_string);

	/*
	 * パケット送信
	 */
	rc = ipmsg_send_packet(con, ipaddr, IPMSG_PROTOCOL_PKTNUM_AUTO, local_flags, 
	    cap_string, IPMSG_PROTOCOL_MSG_NO_EXTENSION);

	if (rc != 0) {
		goto free_cap_string_out;
	}

	rc = 0; /* 正常終了 */

free_cap_string_out:
	if (cap_string != NULL)
		g_free(cap_string);

#endif  /* USE_OPENSSL */

error_out:
	return rc;
}

/** 受信したパケットを処理する
 *  @param[in]  con          UDPコネクション情報
 *  @param[in]  msg          受信したメッセージ情報
 *  @retval     0            正常終了
 *  @retval    -EINVAL       引数異常
 *  @retval    -ENOENT       指定されたコマンドが見つからなかった.
 */
int
ipmsg_dispatch_message(const udp_con_t *con, const msg_data_t *msg){
	int rc = 0;

	if ( (con == NULL) || (msg == NULL) ) {
		rc = -EINVAL;
		goto error_out;
	}

	switch(msg->command) {
	case IPMSG_NOOPERATION:
		break;
	case IPMSG_BR_ENTRY:
		dbg_out("Dispatch br_entry\n");
		ipmsg_proc_br_entry(con, msg);
		break;
	case IPMSG_BR_EXIT:
		dbg_out("Dispatch br_exit\n");
		ipmsg_proc_br_exit(con, msg);
		break;
	case IPMSG_ANSENTRY:
		dbg_out("Dispatch ans_entry\n");
		ipmsg_proc_ans_entry(con, msg);
		break;
	case IPMSG_BR_ABSENCE:
		dbg_out("Dispatch br_absense\n");
		ipmsg_proc_br_absence(con, msg);
		break;
	case IPMSG_BR_ISGETLIST:
		dbg_out("Dispatch isget_list2\n");
		if (hostinfo_refer_ipmsg_is_allow_hlist())
			ipmsg_proc_br_isgetlist2(con, msg);
		break;
	case IPMSG_OKGETLIST:
		dbg_out("Dispatch okget_list\n");
		if (hostinfo_refer_ipmsg_is_get_hlist())    
			ipmsg_proc_okgetlist(con, msg);
		break;
	case IPMSG_GETLIST:
		dbg_out("Dispatch get_list\n");
		if (hostinfo_refer_ipmsg_is_allow_hlist())
			ipmsg_proc_getlist(con, msg);
		else
			dbg_out("Discard getlist request\n");
		break;
	case IPMSG_ANSLIST:
		dbg_out("Dispatch ans_list\n");
		ipmsg_proc_anslist(con, msg);
		break;
	case IPMSG_BR_ISGETLIST2:
		dbg_out("Dispatch br_isget_list2\n");
		if (hostinfo_refer_ipmsg_is_allow_hlist())
			ipmsg_proc_br_isgetlist2(con, msg);
		break;
	case IPMSG_SENDMSG:
		dbg_out("Dispatch send_message\n");
		ipmsg_proc_send_msg(con, msg);
		break;
	case IPMSG_RECVMSG:
		dbg_out("Dispatch recv_message\n");
		ipmsg_proc_recv_msg(con, msg);
		break;
	case IPMSG_READMSG:
		dbg_out("Dispatch read_message\n");
		ipmsg_proc_read_msg(con, msg);
		break;
	case IPMSG_DELMSG:
		dbg_out("Dispatch delete_message\n");
		break;
	case IPMSG_ANSREADMSG:
		dbg_out("Dispatch ans_read_message\n");
		break;
	case IPMSG_GETINFO:
		dbg_out("Dispatch get_info\n");
		ipmsg_proc_get_info(con, msg);
		break;
	case IPMSG_SENDINFO:
		dbg_out("Dispatch send_info\n");
		ipmsg_proc_send_info(con, msg);
		break;
	case IPMSG_GETABSENCEINFO:
		dbg_out("Dispatch get_absence_info\n");
		ipmsg_proc_get_absence_info(con, msg);
		break;
	case IPMSG_SENDABSENCEINFO:
		dbg_out("Dispatch send_absence_info\n");
		ipmsg_proc_send_absence_info(con, msg);
		break;
	case IPMSG_RELEASEFILES:
		dbg_out("Dispatch release files\n");
		ipmsg_proc_release_files_msg(con, msg);
		break;
	case IPMSG_GETPUBKEY:
		dbg_out("Dispatch get public key\n");
		ipmsg_proc_get_public_key(con, msg);
		break;
	case IPMSG_ANSPUBKEY:
		dbg_out("Dispatch ans public key\n");
		ipmsg_proc_anspubkey(con, msg);
		break;
	default:
		dbg_out("Can not dispatch unknown:%x\n", msg->command);
		/* Unknown command */
		rc = -ENOENT;
		break;
	}

error_out:
	return rc;
}

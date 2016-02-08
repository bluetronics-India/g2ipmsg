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
 * @brief  暗号化処理インターフェース部
 * @author Takeharu KATO
 */ 

/** 16進->10進変換用テーブル
 * @attention 内部リンケージ
 */
static char *hexstr = "0123456789abcdef";

/** 16進->10進変換を行う
 *  @param[in]   ch           16進数文字(0-9a-fA-F)
 *  @param[out]  index        chの10進変換結果返却領域
 *  @retval  0       正常終了
 *  @retval -EINVAL  引数異常(indexがNULL, chが16進数文字でない)
 *  @retval -ESRCH   変換できなかった(変換テーブル内に当該の文字がなかった)
 *  @attention 内部リンケージ
 */
static int 
get_hexstr_index(const char ch, char *index){
	int         rc = 0;
	char       ind = 0;
	int  search_ch = 0;
	char    *found = NULL;

	if ( (index == NULL) || ( !isxdigit(ch) ) ) {
		rc = -EINVAL;
		goto error_out;
	}
	  
	search_ch = tolower((int)ch);

	found = strchr(hexstr, (int)search_ch);
	if (found == NULL) {
		err_out("Character %c(%c in original message) can not be found.\n",
		    search_ch, ch);
		rc = -ESRCH;
		goto error_out;
	}

	ind = (char)(found - hexstr);  /* 16進->10進変換 */

	*index = ind;  /* 値返却 */

	rc = 0; /* 正常終了 */

error_out:
	return rc;
}

/** 与えられたバッファ内の各バイトの内容を16進で表示する.
 *  @param[in]  buff           16進数文字(0-9a-fA-F)
 *  @param[in]  len            バッファサイズ
 *  @retval  0       正常終了
 *  @retval -EINVAL  引数異常(buffがNULL)
 *                  
 *  @attention 内部リンケージ
 */
static int
print_hex(const char *buff, const size_t len){
  int  i = 0;
  int rc = 0;

  if (buff == NULL) {
	  rc = -EINVAL;
	  goto error_out;
  }

  dbg_out("decode-in-hex:");
  for (i=0;i<len;++i) {
    dbg_out("%x",buff[i]>>4);
    dbg_out("%x",buff[i]&0xf);
  }
  dbg_out("\n");

error_out:  
  return rc;
}

/** 暗号化されたメッセージを解析し, 以下の情報を取り出す.
 *  - 通信に使用した暗号種別を表す暗号ケイパビリティ
 *  - 本文の暗号化に使用した鍵(hex形式)
 *  - 暗号化された本文(hex形式)
 *  - 署名(hex形式)
 *  @param[in]   message        暗号化されたメッセージ
 *  @param[out]  this_cap_p     通信に使用した暗号種別を表す暗号ケイパビリティ返却領域
 *  @param[out]  hex_skey_p     本文の暗号化に使用した鍵を指すポインタ変数のアドレス
 *  @param[out]  enc_message_p  暗号化された本文を指すポインタ変数のアドレス
 *  @param[out]  hex_sign_p     署名を指すポインタ変数のアドレス
 *  @retval  0       正常終了
 *  @retval -EINVAL  引数異常(buffがNULL)
 *                  
 *  @attention 内部リンケージ
 */
static int
parse_encrypted_message(const unsigned char *message,
		      unsigned long *this_cap_p,
		      unsigned char **hex_skey_p,
		      unsigned char **enc_message_p,
		      unsigned char **hex_sign_p){
	int                     rc = 0;
	char                   *sp = 0;
	char                   *ep = 0;
	unsigned long     this_cap = 0;
	unsigned char    *hex_skey = NULL;
	unsigned char *enc_message = NULL;
	unsigned char    *hex_sign = NULL;
	char                 *buff = NULL;

	if ( (message == NULL) || (this_cap_p == NULL) || (hex_skey_p == NULL) || (enc_message_p == NULL) ) {
		rc = -EINVAL;
		goto error_out;
	}

	rc = -ENOMEM;
	buff = g_strdup(message);
	if (buff == NULL)
		goto error_out;
	/*
	 * 鍵情報
	 */
	sp = buff;
	ep = strchr(sp, ':');
	if (ep == NULL) {
		rc = -EINVAL;
		err_out("Error : No cap\n");
		goto free_buff_out;
	}
	*ep = '\0';
	++ep;
	errno = 0;
	this_cap = strtoul(sp, (char **)NULL, 16);
	dbg_out("Cap:0x%x\n", this_cap);

	/*
	 *暗号鍵
	 */
	sp = ep;
	ep = strchr(sp, ':');
	if (ep == NULL) {
		rc = -EINVAL;
		err_out("Error : crypt key\n");
		goto free_buff_out;
	}
	*ep = '\0';
	++ep;
	rc = -ENOMEM;
	hex_skey = g_strdup(sp);
	if (hex_skey == NULL)
		goto free_buff_out;
	dbg_out("hex secret key:%s\n", hex_skey);

	
	/*
	 * 暗号化本文
	 */
	sp = ep;
	ep = strchr(sp, ':');
	if (ep != NULL) { /* これで最後の可能性もある  */
		*ep = '\0';
		++ep;
	}
	rc = -ENOMEM;
	enc_message = g_strdup(sp);
	if (enc_message == NULL)
		goto free_hex_skey_out;

	dbg_out("hex secret body:%s\n", enc_message);

	/*
	 * 署名
	 */
	if (ep != NULL) {
		sp = ep;
		rc = -ENOMEM;
		hex_sign = g_strdup(sp);
		if (hex_sign == NULL)
			goto free_enc_message_out;
		dbg_out("hex sign:%s\n", hex_sign);
	}

	/*
	 * 返却
	 */

	*this_cap_p    = this_cap;
	*hex_skey_p    = hex_skey;
	*enc_message_p = enc_message;
	*hex_sign_p    = hex_sign;
	if (buff != NULL)
		g_free(buff);

	return 0;

free_enc_message_out:
	if (enc_message != NULL)
		g_free(enc_message);

free_hex_skey_out:
	if (hex_skey != NULL)
		g_free(hex_skey);

free_buff_out:
	if (buff != NULL)
		g_free(buff);

error_out:
	return rc;
}

/** バイナリのバイトストリームをhex形式に変換する
 *  @param[in]   bindata        バイナリのバイトストリーム
 *  @param[in]   len            バイトストリーム長
 *  @param[in]   ret_p          hex形式に変換した結果を指すポインタ変数のアドレス
 *  @retval  0       正常終了
 *  @retval -EINVAL  引数異常(bindataまたはret_pがNULL)
 *                  
 *  @attention 内部リンケージ
 */
static int
string_bin2hex(const u_int8_t *bindata, int len, unsigned char **ret_p)
{
	int                rc = 0;
	int                 i = 0;
	size_t        buf_len = 0;
	unsigned char   *buff = NULL;
	unsigned char *buff_p = NULL;

	if ( (bindata == NULL) || (ret_p == NULL) ) {
		rc = -EINVAL;
		goto error_out;
	}

	buf_len = len * 2 + 1;  /*  バッファ長算出  */

	buff = g_malloc(buf_len); /* バッファ獲得  */
	if (buff == NULL) {
		rc = -ENOMEM;
		goto error_out;
	}

	memset(buff, 0, buf_len); /* バッファを0クリアする */
	
	/*
	 * 変換
	 */
	for (i = 0, buff_p = buff; i < len; ++i) {
		    *buff_p++ = hexstr[bindata[i] >> 4];
		    *buff_p++ = hexstr[bindata[i] & 0x0f];
	}

	*buff_p = '\0'; /* 文字列として扱うようにヌルターミネートする */

	/*
	 * 返却
	 */
	rc = 0;
	*ret_p = buff;

 error_out:
	return rc;
}

/** hex形式の文字列をバイナリのバイトストリームに変換する
 *  @param[in]   hexdata        hex形式(16進文字列)のバイトストリーム
 *  @param[out]   len           バイナリバイトストリーム長返却領域
 *  @param[out]   ret_p         変換したバイナリバイトストリームを指すポインタ
 *                              変数のアドレス
 *  @retval  0       正常終了
 *  @retval -EINVAL  引数異常(bindataまたはret_pがNULL)
 *  @attention 内部リンケージ
 */
static int
string_hex2bin(const char *hexdata, int *len,unsigned char **ret_p)
{
	int                rc = 0;
	int                 i = 0;
	size_t       data_len = 0;
	size_t        buf_len = 0;
	unsigned char   *buff = NULL;
	unsigned char *buff_p = NULL;
	char              low = 0, high = 0;

	data_len = strlen(hexdata);
	buf_len = data_len / 2;
	

	buff = g_malloc(buf_len);
	if (buff == NULL) {
		rc = -ENOMEM;
		goto error_out;
	}

	memset(buff, 0, buf_len);

	for (i=0, buff_p = buff; 
	     ( ( (i + 1) < data_len) && (hexdata[i] != '\0') && (hexdata[i+1])); ) {
		rc = get_hexstr_index(hexdata[i++], &high);
		if (rc != 0)
			goto error_out;
		rc = get_hexstr_index(hexdata[i++], &low);
		if (rc != 0)
			goto error_out;
		*buff_p++ = ( ( (high << 4) & 0xf0) | (low & 0xf) );
	}
  
	*ret_p = buff;
	*len = buf_len;

	return 0;

 error_out:
	if (buff != NULL)
		g_free(buff);
	return rc;
}

/** メッセージを暗号化する
 *  @param[in]   peer_addr     送信先IPアドレス
 *  @param[in]   message       暗号化するメッセージ
 *  @param[out]  ret_str       暗号化したメッセージを指すポインタ変数のアドレス
 *  @param[out]  len           暗号化メッセージ長返却領域
 *  @retval  0       正常終了
 *  @retval -EINVAL  引数異常
 */
int
ipmsg_encrypt_message(const char *peer_addr, const char *message, 
    unsigned char **ret_str, size_t *len) {
	int                           rc = 0;
	int                        retry = 0;
	unsigned long           peer_cap = 0;
	unsigned long          skey_type = 0, akey_type = 0;
	char                *session_key = NULL;
	char                   *enc_skey = NULL;
	size_t                  skey_len = 0;
	char                      *key_e = NULL, *key_n = NULL;
	char               *raw_enc_body = NULL;
	unsigned char          *enc_body = NULL;
	size_t                   enc_len = 0;
	unsigned char *encrypted_message = NULL;
	size_t                 total_len = 0;
	unsigned char              *sign = NULL;
	unsigned long         sign_type = 0;
	int                           i = 0;

	if ( (peer_addr == NULL) || (message == NULL) || 
	     (ret_str == NULL) || (len == NULL) ) {
		rc = -EINVAL;
		goto error_out;
	}

	/*  相手の暗号化能力を取得  */
	retry = PUBKEY_MAX_RETRY;
	do{
		rc = userdb_wait_public_key(peer_addr, &peer_cap, &key_e, &key_n);
		if ( (rc < 0) && (rc != -EINTR) ) {
			goto error_out; /* 明示的なエラー */
		}
		if (rc == 0) { /* 見付けた */
			dbg_out("Found: \n\taddr = %s\n"
			    "\tcap = %x\n"
			    "\tpubkey-e = %s\n"
			    "\tpubkey-n = %s\n",
			    peer_addr,
			    peer_cap,
			    key_e,
			    key_n);

			break; 
		}
		--retry;
	} while(retry > 0);
  
	if ( (rc != 0) && (retry == 0) )
		goto free_peer_key_out; /* 取得失敗 */

	/*
	 *暗号化アルゴリズムを選択
	 */
	rc = select_symmetric_key(peer_cap, &skey_type, 
	    hostinfo_refer_ipmsg_crypt_policy_is_speed());
	if (rc != 0)
		goto free_peer_key_out;
	
	/*
	 * セッションキー作成と本文の暗号化
	 */
	rc = symcrypt_encrypt_message(skey_type, message, &session_key,
	    &skey_len, &raw_enc_body, &enc_len);
	if (rc != 0)
		goto free_peer_key_out;

	rc = string_bin2hex((const u_int8_t *)raw_enc_body, enc_len, &enc_body);
	if (rc != 0)
		goto free_session_key_out;    

	/*
	 * セッションキーを暗号化
	 */
	rc = pcrypt_encrypt_message(peer_cap, key_e, key_n, session_key, 
	    skey_len, &enc_skey, &akey_type);
	if (rc != 0)
		goto free_enc_body_out;
	/*
	 * 暗号化文字列長取得
	 * longの長さは, 64ビット環境の場合は, 8バイトになるので, 
	 * 1バイトあたりの表示領域長(2)をlongのバイト長文だけ掛け合わせることで,
	 * 領域長に対するアーキ依存性を排除する.
	 * +3はデリミタ(':')2つとヌルターミネートの分 
	 */
	total_len = sizeof(unsigned long) * 2 + strlen(enc_skey) + strlen(enc_body) + 3;

	encrypted_message = g_malloc(total_len);
	if (encrypted_message == NULL) {
		rc = -ENOMEM;
		goto free_encoded_session_key;
	}

	/* 
	 *署名を選択する
	 */
	rc = select_signature(peer_cap, &sign_type, 
	    hostinfo_refer_ipmsg_crypt_policy_is_speed());
	if (rc != 0) {
		sign_type = 0; /*  署名を使用しない  */
	}

	snprintf(encrypted_message, total_len, "%x:%s:%s", 
	    (skey_type|akey_type|sign_type), enc_skey, enc_body);

	dbg_out("Encrypted body:%s\n", encrypted_message);

	if (sign_type != 0) {
		/*
		 * 小文字に変換
		 */
		for(i = 0; i < total_len; ++i) {
			encrypted_message[i] = tolower((int)encrypted_message[i]);
		}
		rc = pcrypt_sign(akey_type, sign_type, encrypted_message, &sign);

		if (rc != 0)
			goto free_sign_out;

		g_free(encrypted_message);	
		/*  
		 * 署名を付して本文を再作成  
		 */
		total_len += (strlen(sign) + 1); /* +1は, デリミタ分 */
		encrypted_message = g_malloc(total_len);
		if (encrypted_message == NULL)
			goto free_sign_out;
		snprintf(encrypted_message, total_len, "%x:%s:%s:%s",
		    (skey_type|akey_type|sign_type), 
		    enc_skey,
		    enc_body,
		    sign);

		dbg_out("Signed body:%s\n", encrypted_message);
	}

	/*
	 * 小文字に変換
	 */
	total_len = strlen(encrypted_message);
	for(i = 0; i < total_len; ++i) {
		encrypted_message[i] = tolower((int)encrypted_message[i]);
	}
	
	/*
	 * 解析結果返却
	 */

	*ret_str = encrypted_message;
	*len     = total_len;

	rc = 0;

free_sign_out:
	if (sign != NULL)
		g_free(sign);

free_enc_body_out:
	if (enc_body != NULL)
		g_free(enc_body);

free_encoded_session_key:
	if (enc_skey != NULL)
		g_free(enc_skey);

free_session_key_out:
	if (session_key != NULL)
		g_free(session_key);

	if (raw_enc_body != NULL)
		g_free(raw_enc_body);

free_peer_key_out:
	if (key_e != NULL)
		g_free(key_e);

	if (key_n != NULL)
		g_free(key_n);

error_out:
	return rc;
}

/** 暗号化されたメッセージを複号する
 *  @param[in]   peer_addr     送信先IPアドレス
 *  @param[in]   message       暗号化するメッセージ
 *  @param[out]  ret_str       暗号化したメッセージを指すポインタ変数のアドレス
 *  @param[out]  len           暗号化メッセージ長返却領域
 *  @retval  0       正常終了
 *  @retval -EINVAL  引数異常
 *  @retval -EAGAIN  署名異常
 */
int
ipmsg_decrypt_message(const char *peer_addr, const char *message, 
    unsigned char **ret_str, size_t *len) {
	int                            rc = 0;
	unsigned long            this_cap = 0;
	unsigned char           *hex_skey = NULL;
	char                        *skey = NULL;
	unsigned char     *signed_message = NULL;
	unsigned char *end_message_body_p = NULL;
	unsigned char        *enc_message = NULL;
	unsigned char           *hex_sign = NULL;
	unsigned long           skey_type = 0, akey_type = 0;
	unsigned long           sign_type = 0;
	unsigned char       *enc_bin_body = NULL;
	unsigned char         *peer_key_e = NULL;
	unsigned char         *peer_key_n = NULL;
	unsigned long             tmp_cap = 0;
	unsigned long           new_flags = 0;
	char                       *plain = NULL;
	size_t                  plain_len = 0;
	size_t                   skey_len = 0;
	size_t                enc_bin_len = 0;
  
	if ( (message == NULL) || (ret_str == NULL) || (len == NULL) ) {
		rc = -EINVAL;
		goto error_out;
	}

	/*
	 * 暗号化されたメッセージから鍵情報を獲得する.
	 */
	rc = parse_encrypted_message(message, &this_cap, 
	    &hex_skey, &enc_message, &hex_sign);
	if (rc != 0) {
		err_out("Can not parse message\n");
		goto error_out;
	}

	/*
	 * 暗号化に使用した鍵種別をケイパビリティから算出する
	 */
	skey_type = get_symkey_part(this_cap);  /* 共通鍵 */
	akey_type = get_asymkey_part(this_cap); /* 公開鍵 */
	sign_type = get_sign_part(this_cap);    /* 署名 */

	dbg_out("Cap:%x Skey:%x AKey:%x Sign:%x\n", 
	    this_cap, skey_type, akey_type, sign_type);

	/*
	 * 署名がある場合は署名を検証
	 */
	g_assert(peer_addr); /* udpからの呼出しの場合はかならずいれる  */

	if ( (hostinfo_get_ipmsg_crypt_capability() & sign_type) && 
	    (hex_sign != NULL) ) {
		dbg_out("This message is signed by peer.\n");

		/*
		 * 相手の公開鍵を取得
		 */
		rc = userdb_get_public_key_by_addr(peer_addr, &tmp_cap, 
		    (char **)&peer_key_e, (char **)&peer_key_n);
		if (rc != 0) {
			goto free_parsed_datas;
		}

		/* 編集用にコピー  */
		signed_message = g_strdup(message);
		rc = -ENOMEM;
		if (signed_message == NULL)
			goto free_parsed_datas;
		end_message_body_p = strrchr(signed_message, ':');
		if (end_message_body_p == NULL) /*  異常データ  */
			goto free_parsed_datas;

		*end_message_body_p = '\0'; /* 本文だけを参照  */
		dbg_out("Verify:%s with %s\n", 
		    signed_message, hex_sign);
		rc = pcrypt_verify_sign(this_cap, sign_type, signed_message, 
		    hex_sign, peer_key_e, peer_key_n);

		/*  失敗した場合でも, 不要なデータを開放してからぬける  */
		if (rc != 0) {
			err_out("Verify failed: libcrypt rc=%d\n", rc);
			rc = -EAGAIN;
			goto free_parsed_datas;
		}

		dbg_out("Verify OK\n");
	}

	/*
	 * 共通鍵をデコード
	 */

	/* FIXME 鍵のバリデーション(RSAが2つ以上設定されていないか) */
	rc = pcrypt_decrypt_message(akey_type, hex_skey, &skey, &skey_len);
	if (rc != 0) {
		goto free_parsed_datas;
	}
	dbg_out("Decrypt key len:%d\n", skey_len);

	/*
	 * 暗号化された本文のバイナリ化
	 */
	rc = string_hex2bin(enc_message, &enc_bin_len, &enc_bin_body);
	if (rc != 0) {
		goto free_skey;
	}

#if 0
	print_hex(skey, skey_len);
#endif
	/*
	 * 本文を復号化する
	 */
	rc = symcrypt_decrypt_message(skey_type, enc_bin_body, enc_bin_len,
	    skey, &plain, &plain_len);
	if (rc != 0) {
		goto free_enc_bin_body;
	}
	dbg_out("Decoded:%s len=%d\n", plain, plain_len);

	*ret_str = plain;
	*len     = plain_len;

	rc = 0;

free_enc_bin_body:
	if (enc_bin_body != NULL)
		g_free(enc_bin_body);

free_skey:
	if (skey != NULL)
		g_free(skey);
	
free_parsed_datas:
	if (hex_skey != NULL)
		g_free(hex_skey);
	
	if (enc_message != NULL)
		g_free(enc_message);

	if (hex_sign != NULL)
		g_free(hex_sign);

	if (peer_key_e != NULL)
		g_free(peer_key_e);

	if (peer_key_n != NULL)
		g_free(peer_key_n);

	if (signed_message != NULL)
		g_free(signed_message);

 error_out:
	if ( (rc != 0) && (rc != -EAGAIN) ){ /* 署名検証に失敗した場合を除く */
		ipmsg_err_dialog("%s: %s = %s, rc = %d", 
		    _("Can not decode message"), _("peer"), peer_addr, rc);
	}

	return rc;
}

/** パスワード入力を促す
 *  @retval  0       正常終了
 *  @retval -ENOMEM  メモリ不足
 */
int
enter_password(void) {
	int            rc = 0;
	GtkWidget *window = NULL;
	gint       result = 0;

	window = create_passwdWindow();
	if (window == NULL) {
		rc = -ENOMEM;
		goto error_out;
	}

error_out:
	return rc;
}



/*
 * Following codes are derived from 
 * ``Secure Programming Cookbook for C and C++''.
 * URL:http://www.oreilly.com/catalog/secureprgckbk/ 
 *     http://www.secureprogramming.com/
 * 
 * Licensing Information can be obtain from following URL:
 *
 *  http://www.secureprogramming.com/?action=license
 *
 * Copyright  2003 by John Viega and Matt Messier.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met: 

 * 1. Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 *  documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the developer of this software nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include "common.h"

/** @file 
 * @brief  共通鍵暗号処理
 * @author Takeharu KATO
 */ 


/** キー選択順序テーブル
 * @attention 内部リンケージ
 */
static ipmsg_cap_t key_select_table[]={
	IPMSG_AES_256,
	IPMSG_BLOWFISH_256,  
	IPMSG_RC2_256,
	IPMSG_AES_192,
	IPMSG_AES_128,
	IPMSG_BLOWFISH_128,
	IPMSG_RC2_128,
	IPMSG_RC2_40,
  -1,
};

/** IPMSGの暗号化ケイパビリティから鍵長を取得する
 *  @param[in]   type           16進数文字(0-9a-fA-F)
 *  @param[out]  ret_len        鍵長(単位:バイト)
 *  @retval            0       正常終了
 *  @retval           -EINVAL  引数異常
 *  @retval           -ENOENT  指定されたケイパビリティに対応する鍵が見付からなかった
 */
int
symcrypt_get_skey_length(const ipmsg_cap_t type, size_t *ret_len){
	size_t key_len = 0;
	int         rc = 0;

	if (ret_len == NULL) {
		rc = -EINVAL;
		goto error_out;
	}

	switch(type){
	case IPMSG_RC2_40:
		key_len = 5;
		break;
	case IPMSG_RC2_128:
	case IPMSG_AES_128:
	case IPMSG_BLOWFISH_128:
		key_len = 16;
		break;
	case IPMSG_AES_192:
		key_len = 24;
		break;
	case IPMSG_RC2_256:
	case IPMSG_AES_256:
	case IPMSG_BLOWFISH_256:
		key_len = 32;
		break;
	default:
		rc = -ENOENT;
		goto error_out;
		break;
	}

	*ret_len = key_len; /* 鍵長返却  */

	rc = 0; /* 正常終了 */

error_out:
	return rc;
}

/** BlowFish暗号化のセットアップを行う
 *  @param[in]   key            暗号化鍵(バイナリ形式)
 *  @param[in]   key_len_byte   暗号化鍵長(単位:バイト)
 *  @param[in]   iv             情報ベクタ
 *  @param[out]  ret            設定済み暗号コンテキスト返却領域を指す
 *  @retval        0            正常終了
 *  @retval       -EINVAL       引数異常
 *  @retval       -ENOMEM       メモリ不足
 *  @retval       -ENOENT       指定されたケイパビリティに対応する鍵が見付からなかった
 */
int
blowfish_cbc_encrypt_setup(const char *key, const size_t key_len_byte, const char *iv, 
    EVP_CIPHER_CTX **ret) {
	int              rc = 0;
	EVP_CIPHER_CTX *ctx = NULL;

	if (ret == NULL) {
		rc = -EINVAL;
		goto error_out;
	}

	/*
	 * 暗号コンテキスト獲得
	 */
	ctx = (EVP_CIPHER_CTX *)g_malloc(sizeof(EVP_CIPHER_CTX));
	if (ctx == NULL) {
		rc = -ENOMEM;
		goto error_out;
	}

	EVP_CIPHER_CTX_init(ctx);  /* 暗号コンテキスト初期化 */
  
	/*
	 * 暗号コンテキスト設定
	 */
	EVP_EncryptInit(ctx, EVP_bf_cbc(), NULL, NULL);   /* BlowFishを使用 */
	EVP_CIPHER_CTX_set_key_length(ctx, key_len_byte); /* 鍵長設定 */
	EVP_EncryptInit(ctx, NULL, key, iv);              /* iv設定 */

	*ret = ctx;
	rc = 0;

error_out:
	return rc;
}

/** BlowFish複号化のセットアップを行う
 *  @param[in]   key            暗号化鍵(バイナリ形式)
 *  @param[in]   key_len_byte   暗号化鍵長(単位:バイト)
 *  @param[in]   iv             情報ベクタ
 *  @param[out]  ret            設定済み暗号コンテキスト返却領域を指す
 *  @retval      0              正常終了
 *  @retval     -EINVAL         引数異常
 *  @retval     -ENOMEM         メモリ不足
 */
int
blowfish_cbc_decrypt_setup(const char *key, const size_t key_len_byte, const char *iv, 
    EVP_CIPHER_CTX **ret) {
	int              rc = 0;
	EVP_CIPHER_CTX *ctx = NULL;
	
	if (ret == NULL) {
		rc = -EINVAL;
		goto error_out;
	}

	/*
	 * 暗号コンテキスト獲得
	 */
	ctx = (EVP_CIPHER_CTX *)g_malloc(sizeof(EVP_CIPHER_CTX));
	if (ctx == NULL) {
		rc = -ENOMEM;
		goto error_out;
	}

	EVP_CIPHER_CTX_init(ctx);  /* 暗号コンテキスト初期化 */

	/*
	 * 暗号コンテキスト設定
	 */
  
	EVP_DecryptInit(ctx, EVP_bf_cbc(), NULL, NULL);   /* BlowFish暗号を使用 */
	EVP_CIPHER_CTX_set_key_length(ctx, key_len_byte); /* 鍵長設定 */
	EVP_DecryptInit(ctx, NULL, key, iv);              /* iv設定 */

	/*
	 * 返却
	 */
	*ret = ctx; 
	rc   = 0;

error_out:
	return rc;
}

/** AES暗号化のセットアップを行う
 *  @param[in]   key            暗号化鍵(バイナリ形式)
 *  @param[in]   key_len_byte   暗号化鍵長(単位:バイト)
 *  @param[in]   iv             情報ベクタ
 *  @param[out]  ret            設定済み暗号コンテキスト返却領域を指す
 *  @retval        0            正常終了
 *  @retval       -EINVAL       引数異常
 *  @retval       -ENOMEM       メモリ不足
 *  @retval       -ENOENT       指定されたケイパビリティに対応する鍵が見付からなかった
 */
int
aes_cbc_encrypt_setup(const char *key, const size_t key_len_byte, 
    const char *iv, EVP_CIPHER_CTX **ret) {
	int              rc = 0;
	EVP_CIPHER_CTX *ctx = NULL;

	if (ret == NULL) {
		rc = -EINVAL;
		goto error_out;
	}

	/*
	 * 暗号コンテキスト獲得
	 */
	ctx = (EVP_CIPHER_CTX *)g_malloc(sizeof(EVP_CIPHER_CTX));
	if (ctx == NULL) {
		rc = -ENOMEM;
		goto error_out;
	}

	EVP_CIPHER_CTX_init(ctx);  /* 暗号コンテキスト初期化 */
  
	/*
	 * 暗号コンテキスト設定
	 */
	switch(key_len_byte) {
	case 16:	/* IPMSG_AES_128 */
		EVP_EncryptInit(ctx, EVP_aes_128_cbc(), NULL, NULL);   
		break;
	case 24:        /* IPMSG_AES_192 */
		EVP_EncryptInit(ctx, EVP_aes_192_cbc(), NULL, NULL);   
		break;
	case 32:        /* IPMSG_AES_256 */
		EVP_EncryptInit(ctx, EVP_aes_256_cbc(), NULL, NULL);   
		break;
	default:
		rc = -ENOENT;
		goto error_out;
		break;
	}

	EVP_CIPHER_CTX_set_key_length(ctx, key_len_byte);  /* 鍵長設定 */
	EVP_EncryptInit(ctx, NULL, key, iv);               /* iv設定 */

	*ret = ctx;
	rc = 0;

error_out:
	return rc;
}

/** AES複号化のセットアップを行う
 *  @param[in]   key            暗号化鍵(バイナリ形式)
 *  @param[in]   key_len_byte   暗号化鍵長(単位:バイト)
 *  @param[in]   iv             情報ベクタ
 *  @param[out]  ret            設定済み暗号コンテキスト返却領域を指す
 *  @retval      0              正常終了
 *  @retval     -EINVAL         引数異常
 *  @retval     -ENOMEM         メモリ不足
 */
int
aes_cbc_decrypt_setup(const char *key, const size_t key_len_byte, const char *iv, 
    EVP_CIPHER_CTX **ret) {
	int              rc = 0;
	EVP_CIPHER_CTX *ctx = NULL;
	
	if (ret == NULL) {
		rc = -EINVAL;
		goto error_out;
	}

	/*
	 * 暗号コンテキスト獲得
	 */
	ctx = (EVP_CIPHER_CTX *)g_malloc(sizeof(EVP_CIPHER_CTX));
	if (ctx == NULL) {
		rc = -ENOMEM;
		goto error_out;
	}

	EVP_CIPHER_CTX_init(ctx);  /* 暗号コンテキスト初期化 */

	/*
	 * 暗号コンテキスト設定
	 */
	/*
	 * 暗号コンテキスト設定
	 */
	switch(key_len_byte) {
	case 16:	/* IPMSG_AES_128 */
		EVP_DecryptInit(ctx, EVP_aes_128_cbc(), NULL, NULL); 
		break;
	case 24:        /* IPMSG_AES_192 */
		EVP_DecryptInit(ctx, EVP_aes_192_cbc(), NULL, NULL); 
		break;
	case 32:        /* IPMSG_AES_256 */
		EVP_DecryptInit(ctx, EVP_aes_256_cbc(), NULL, NULL); 
		break;
	default:
		rc = -ENOENT;
		goto error_out;
		break;
	}
	EVP_CIPHER_CTX_set_key_length(ctx, key_len_byte);  /* 鍵長設定 */
	EVP_DecryptInit(ctx, NULL, key, iv);               /* iv設定 */

	/*
	 * 返却
	 */
	*ret = ctx; 
	rc   = 0;

error_out:
	return rc;
}


/** RC2暗号化のセットアップを行う
 *  @param[in]   key            暗号化鍵(バイナリ形式)
 *  @param[in]   key_len_byte   暗号化鍵長(単位:バイト)
 *  @param[in]   iv             情報ベクタ
 *  @param[out]  ret            設定済み暗号コンテキスト返却領域を指す
 *  @retval      0              正常終了
 *  @retval     -EINVAL         引数異常
 *  @retval     -ENOMEM         メモリ不足
 */
int
rc2_cbc_encrypt_setup(const char *key, const size_t key_len_byte, const char *iv, 
    EVP_CIPHER_CTX **ret) {
	int              rc = 0;
	EVP_CIPHER_CTX *ctx = NULL;

	if (ret == NULL) {
		rc = -EINVAL;
		goto error_out;
	}
	
	/*
	 * 暗号コンテキスト取得
	 */
	ctx = (EVP_CIPHER_CTX *)g_malloc(sizeof(EVP_CIPHER_CTX));
	if (ctx == NULL) {
		rc = -ENOMEM;
		goto error_out;
	}

	EVP_CIPHER_CTX_init(ctx);  /* 暗号コンテキスト初期化 */

	/*
	 * 暗号コンテキスト設定
	 */
	EVP_EncryptInit(ctx, EVP_rc2_cbc(), NULL,NULL);  /* RC2を使う */
	EVP_CIPHER_CTX_set_key_length(ctx,key_len_byte); /* 鍵長を設定 */
	EVP_EncryptInit(ctx, NULL, key, iv);             /* ivを設定 */

	/*
	 * 設定したコンテキストを返却
	 */
	*ret = ctx;
	rc   = 0;

error_out:
	return rc;
}

/** RC2複号化のセットアップを行う
 *  @param[in]   key            暗号化鍵(バイナリ形式)
 *  @param[in]   key_len_byte   暗号化鍵長(単位:バイト)
 *  @param[in]   iv             情報ベクタ
 *  @param[out]  ret            設定済み暗号コンテキスト返却領域を指す
 *  @retval      0              正常終了
 *  @retval     -EINVAL         引数異常
 *  @retval     -ENOMEM         メモリ不足
 */
int
rc2_cbc_decrypt_setup(const char *key, const size_t key_len_byte, const char *iv, EVP_CIPHER_CTX **ret) {
	int             rc = 0;
	EVP_CIPHER_CTX *ctx = NULL;

	if (ret == NULL) {
		rc = -EINVAL;
		goto error_out;
	}

	/*
	 * 暗号化コンテキスト獲得
	 */
	ctx = (EVP_CIPHER_CTX *)g_malloc(sizeof(EVP_CIPHER_CTX));
	if (ctx == NULL) {
		rc = -ENOMEM;
		goto error_out;
	}

	EVP_CIPHER_CTX_init(ctx);  /*  暗号コンテキスト初期化 */
  
	/*
	 * 暗号コンテキスト設定
	 */
	EVP_DecryptInit(ctx, EVP_rc2_cbc(), NULL, NULL); /* RC2を使用 */
	EVP_CIPHER_CTX_set_key_length(ctx,key_len_byte); /* 鍵長設定 */
	EVP_DecryptInit(ctx, NULL, key,iv);              /* iv設定 */

	/*
	 * 返却
	 */
	*ret = ctx;
	rc   = 0;

error_out:
	return rc;
}

/** Cipher Block Chaining(CBC)モードでの暗号化を行う
 *  @param[in]   ctx            暗号コンテキスト
 *  @param[in]   data           平文
 *  @param[in]   inl            入力電文(平文)長(単位:バイト)
 *  @param[out]  ret_buff       暗号化した電文(バイナリ形式)を指すポインタ変数のアドレス
 *  @param[out]  rb_len         暗号化した電文(バイナリ形式)の長さ(単位:バイト)
 *  @retval      0              正常終了
 *  @retval     -EINVAL         引数異常
 *  @retval     -ENOMEM         メモリ不足
 *  @retval     -ENOENT         暗号化に失敗した
 */
int
common_cbc_encrypt(EVP_CIPHER_CTX *ctx, const char *data, const int inl, char **ret_buff, size_t *rb_len) {
	int               rc = 0;
	int         buf_size = 0;
	size_t       out_len = 0;
	size_t total_out_len = 0;
	char         *buffer = NULL;
	char          errbuf[G2IPMSG_CRYPT_EBUFSIZ];
  
	if ( (ctx == NULL) || (data == NULL) || (ret_buff == NULL) ) {
		rc = -EINVAL;
		goto error_out;
	}
	
	/*
	 * 暗号化電文格納領域を確保
	 */
	buf_size = inl + EVP_CIPHER_CTX_block_size(ctx) + 1;
	if (buf_size < inl) {
		rc = -EINVAL;
		goto error_out;
	}

	/*
	 * 暗号化データ格納領域を確保
	 */
	buffer = g_malloc(buf_size);
	if (buffer == NULL) {
		rc = -ENOMEM;
		goto error_out;
	}

	/*
	 * 暗号化を実施
	 */
	rc = EVP_EncryptUpdate(ctx, buffer, &out_len, data, inl);

	if (rc == 0) {
		rc = ERR_get_error();
		err_out("Can not Encrypt update message err=%s\n", 
		    ERR_error_string(rc, errbuf));
		rc = -ENOENT;
		goto free_buffer_out;
	}  

	total_out_len = out_len;

	/*
	 * パディング処理を実施
	 */
	rc = EVP_EncryptFinal (ctx, buffer + total_out_len, &out_len);
	if (rc == 0) {
		rc = ERR_get_error();
		err_out("Can not Encrypt update message err=%s\n", 
		    ERR_error_string(rc, errbuf));
		rc = -ENOENT;
		goto free_buffer_out;
	}  

	total_out_len += out_len; /* パディング領域長を追加 */

	/*
	 * 返却
	 */
	*ret_buff = buffer;
	*rb_len   = total_out_len;

	rc = 0;  /* 正常終了  */

	return rc;

 free_buffer_out:
	if (buffer != NULL)
		g_free(buffer);
 error_out:
	return rc;
}

/** Cipher Block Chaining(CBC)モードでの複号化を行う
 *  @param[in]   ctx            暗号コンテキスト
 *  @param[in]   ct             暗号化電文(バイナリ形式)
 *  @param[in]   inl            入力電文(暗号化電文)長(単位:バイト)
 *  @param[out]  ret_buff       複号化した電文(バイナリ形式)を指すポインタ変数のアドレス
 *  @param[out]  rb_len         複号化した電文(バイナリ形式)の長さ(単位:バイト)
 *  @retval      0              正常終了
 *  @retval     -EINVAL         引数異常
 *  @retval     -ENOMEM         メモリ不足
 *  @retval     -ENOENT         暗号化に失敗した
 */
int 
common_cbc_decrypt(EVP_CIPHER_CTX *ctx, const char *ct, const int inl, char **ret_buff, size_t *outl) {
	int      rc = 0;
	int      ol = 0, i = 0, tlen = 0;
	char    *pt = NULL;
	char errbuf[G2IPMSG_CRYPT_EBUFSIZ];

	if ( (ctx == NULL) || (ct == NULL) || (ret_buff == NULL) || (outl == NULL) ) {
		rc = -EINVAL;
		goto error_out;
	}

	/*
	 * 復号化した電文を格納する領域を確保
	 */
	i = inl + EVP_CIPHER_CTX_block_size(ctx) + 1;
	pt = g_malloc(i);
	if (pt == NULL) {
		rc = -ENOMEM;
		goto error_out;
	}

	if (i < inl) {
		rc = -EINVAL;
		goto error_out;
	}

	dbg_out("Decrypt try to :%d length\n",inl);

	/*
	 * 復号化を実施
	 */
	rc = EVP_DecryptUpdate(ctx, pt, &ol, ct, inl);
	if (rc == 0) {
		rc = ERR_get_error();
		err_out("Can not decrypt update message err=%s\n", 
		    ERR_error_string(rc, errbuf));
		rc = -ENOENT;
		goto free_buffer_out;
	}

	dbg_out("Decrypt update %d length\n", ol);

	/*
	 * 最終ブロックの復号化
	 */
	rc=EVP_DecryptFinal (ctx, pt + ol, &tlen);
	if (rc == 0) {
		rc = ERR_get_error();
		err_out("Can not decrypt finalize message err=%s\n", 
		    ERR_error_string(rc, errbuf));
		rc = -ENOENT;
		goto free_buffer_out;
	}

	ol     += tlen;

	pt[ol]  = '\0'; /* 文字列化するために念のためヌルターミネートする */
	
	/*
	 * 返却
	 */
	*ret_buff = pt;
	*outl     = ol;

	return 0;  /*  正常終了  */

free_buffer_out:
	if (pt != NULL)
		g_free(pt);
error_out:
	return rc;
}

/** 暗号コンテキストを開放する
 *  @param[in]   ctx_p          暗号コンテキストを指しているポインタ変数のアドレス
 *  @retval      0              正常終了
 *  @retval     -EINVAL         引数異常
 */
int
common_cbc_finalize(EVP_CIPHER_CTX **ctx_p) {
	int              rc = 0;
	EVP_CIPHER_CTX *ctx = NULL;

	if ( (ctx_p == NULL) ||  (*ctx_p == NULL)  ) {
		rc = -EINVAL;
		goto error_out;
	}

	ctx = *ctx_p;
	EVP_CIPHER_CTX_cleanup(ctx);  /* 暗号コンテキストの中身を開放 */
	g_free(ctx);  /* 暗号コンテキストを開放 */

	*ctx_p = NULL; /* NULLに再初期化 */
	rc = 0; /* 正常終了 */

error_out:
	return rc;
}

/** 共通鍵暗号による暗号化を行う
 *  @param[in]   type           暗号種別を表す暗号ケイパビリティ
 *  @param[in]   plain          平文
 *  @param[out]  key_p          暗号化に使用した鍵を指すポインタのアドレス
 *  @param[out]  key_len_p      暗号化に使用した鍵の鍵長返却領域
 *  @param[out]  enc_p          暗号化された電文を指すポインタのアドレス
 *  @param[out]  enc_len_p      暗号化された電文の電文長返却領域
 *  @retval      0              正常終了
 *  @retval     -EINVAL         引数異常
 *  @retval     -ENOMEM         メモリ不足
 */
int
symcrypt_encrypt_message(ipmsg_cap_t type, const unsigned char *plain,
			 char **key_p, size_t *key_len_p, char **enc_p,
			 size_t *enc_len_p){
	size_t      key_len = 0;
	size_t      enc_len = 0;
	size_t    plain_len = 0;
	EVP_CIPHER_CTX *ctx = NULL;
	char           *key = NULL;
	char           *enc = NULL;
	int              rc = 0;
	char             iv[EVP_MAX_IV_LENGTH];

	if ( (plain == NULL) || (key_p == NULL) || (key_len_p == NULL) || 
	    (enc_p == NULL) || (enc_len_p == NULL) ) {
		rc = -EINVAL;
		goto error_out;
	}

	/*
	 * 平文のバッファ長を獲得(ヌルターミネートを含む)
	 */
	plain_len = strlen(plain) + 1;

	/*
	 * 暗号ケイパビリティから鍵長を算出
	 */
	rc = symcrypt_get_skey_length(type, &key_len);
	if (rc != 0)
		goto error_out;

	/*
	 * 鍵格納バッファを獲得
	 */
	key = g_malloc(key_len);
	if (key == NULL) {
		rc = -ENOMEM;
		goto error_out;
	}

	/*
	 * 暗号化鍵を生成(ランダム値を使用)
	 */
	rc = generate_rand(key, key_len);
	if (rc != 0) 
		goto key_free_out;

	/*
	 * IPMSGで規定されたivを設定
	 */
	memset(iv, 0, EVP_MAX_IV_LENGTH);

	/*
	 * 暗号コンテキストの設定
	 */
	switch(type){
	case IPMSG_RC2_40:
	case IPMSG_RC2_128:
	case IPMSG_RC2_256:
		dbg_out("Use RC2 key %d\n", key_len);
		rc = rc2_cbc_encrypt_setup(key, key_len, NULL, &ctx);
		break;
	case IPMSG_BLOWFISH_128:
	case IPMSG_BLOWFISH_256:
		dbg_out("Use BlowFish key %d\n", key_len);
		rc = blowfish_cbc_encrypt_setup(key, key_len, NULL, &ctx);
		break;
	case IPMSG_AES_128:
	case IPMSG_AES_192:
	case IPMSG_AES_256:
		dbg_out("Use AES key %d\n", key_len);
		rc = aes_cbc_encrypt_setup(key, key_len, NULL, &ctx);
		break;
	default:
		rc = -EINVAL; /* 不正な鍵種別 */
		break;
	}
	if (rc != 0) {
		err_out("Can not set key\n");
		goto key_free_out;
	}

	/*
	 * 暗号化を実施
	 */
	rc = common_cbc_encrypt(ctx, plain, plain_len, &enc, &enc_len);
	if (rc != 0) {
		err_out("Can not encrypt message\n");
		goto key_free_out;
	}
	
	/*
	 * 暗号コンテキスト開放
	 */
	rc = common_cbc_finalize(&ctx);
	if (rc != 0) {
		err_out("Can not clean up encrypt\n");
		goto enc_free_out;
	}

	/*
	 * 返却
	 */
	*key_p     = key;     /* 暗号鍵       */
	*key_len_p = key_len; /* 鍵長         */
	*enc_p     = enc;     /* 暗号化電文   */
	*enc_len_p = enc_len; /* 暗号化電文長 */

	return 0;

 enc_free_out:
	if (enc != NULL)
		g_free(enc);
 ctx_free_out:
	if (ctx != NULL)
		common_cbc_finalize(&ctx);
 key_free_out:
	if (key != NULL)
		g_free(key);
error_out:
	return rc;
}

/** 共通鍵暗号による複号化を行う
 *  @param[in]   type           暗号種別を表す暗号ケイパビリティ
 *  @param[in]   encoded        暗号化電文
 *  @param[in]   enc_len        暗号化電文長(単位:バイト)
 *  @param[in]   key            暗号化に使用した鍵
 *  @param[out]  dec_p          複号化された電文を指すポインタのアドレス
 *  @param[out]  dec_len_p      複号化された電文の電文長返却領域
 *  @retval      0              正常終了
 *  @retval     -EINVAL         引数異常
 *  @retval     -ENOMEM         メモリ不足
 */
int
symcrypt_decrypt_message(ipmsg_cap_t type,const unsigned char *encoded,
			 size_t enc_len, const char *key,char **dec_p, 
			 size_t *dec_len_p){
	EVP_CIPHER_CTX *ctx = NULL;
	char           *dec = NULL;
	char       *new_dec = NULL;
	size_t      key_len = 0;
	size_t      dec_len = 0;
	int              rc = 0;

	if ( (encoded == NULL) || (key == NULL) || 
	    (dec_p == NULL) || (dec_len_p == NULL) ) {
		rc = -EINVAL;
		goto error_out;
	}
	
	/*
	 * 暗号化ケイパビリティから鍵長を獲得
	 */
	rc = symcrypt_get_skey_length(type, &key_len);
	if (rc != 0)
		goto error_out;

	/*
	 * 暗号コンテキストのセットアップを行う
	 */
	switch(type){
	case IPMSG_RC2_40:
	case IPMSG_RC2_128:
	case IPMSG_RC2_256:
		dbg_out("Use RC2 key %d\n", key_len);
		rc = rc2_cbc_decrypt_setup(key, key_len, NULL, &ctx);
		break;
	case IPMSG_BLOWFISH_128:
	case IPMSG_BLOWFISH_256:
		dbg_out("Use blowfish key %d\n", key_len);
		rc = blowfish_cbc_decrypt_setup(key, key_len, NULL, &ctx);
		break;
	case IPMSG_AES_128:
	case IPMSG_AES_192:
	case IPMSG_AES_256:
		dbg_out("Use AES key %d\n", key_len);
		rc = aes_cbc_decrypt_setup(key, key_len, NULL, &ctx);
		break;
	default:
		rc = -EINVAL;
		break;
	}
	if (rc != 0)
		goto error_out;

	dbg_out("Decrypt:%s (len=%d) into plain\n", encoded, enc_len);
	rc = common_cbc_decrypt(ctx, encoded, enc_len, &dec, &dec_len);
	if (rc != 0) {
		err_out("Can not decrypt\n");
		goto ctx_free_out;
	}

	dbg_out("Decrypt len:%d\n", dec_len);

	/*
	 * 暗号コンテキスト開放
	 */
	rc = common_cbc_finalize(&ctx);
	if (rc != 0) {
		err_out("Can not clean up decrypt\n");
		goto dec_free_out;
	}

	/*
	 * ヌルターミネートされていない電文を受け取った場合は,
	 * 自分でヌルターミネートを追加する.
	 */
	if (dec[dec_len-1] != '\0') {
		new_dec = g_malloc(dec_len+1);
		if (new_dec == NULL)
			goto dec_free_out;

		memcpy(new_dec, dec, dec_len);
		new_dec[dec_len] = '\0';
		g_free(dec);
		dec = new_dec;
	}

	/*
	 * 返却
	 */
	*dec_p     = dec;
	*dec_len_p = dec_len;

	rc = 0;  /*  正常終了  */

	dbg_out("decoded:%s (%d)\n", dec, dec_len);

	return 0;

 dec_free_out:
	if (dec != NULL)
		g_free(dec);
 ctx_free_out:
	if (ctx != NULL)
		common_cbc_finalize(&ctx);
error_out:
	return rc;
}

/** ピアの暗号化能力に基づいて暗号化に使用する共通鍵種別を決定する.
 *  @param[in]   peer_cap       ピアの暗号ケイパビリティ
 *  @param[out]  selected_key   選択した鍵種別返却領域
 *  @param[in]   speed          速度優先で鍵の選択を実施する指示
 *                              - 真 処理速度優先で選択する
 *                              - 偽 安全性優先で選択する(デフォルト)
 *  @retval      0              正常終了
 *  @retval     -EINVAL         引数異常
 *  @retval     -ENOENT         使用可能な暗号が見付からなかった.
 */
int 
select_symmetric_key(ipmsg_cap_t peer_cap, ipmsg_cap_t *selected_key, 
		     int speed){
	int                    i = 0;
	int                   rc = 0;
	int                first = 0;
	int             key_type = 0;
	int            added_num = 1;
	ipmsg_cap_t   candidates = 0;

	if (selected_key == NULL) {
		rc = -EINVAL;
		goto error_out;
	}

	/*
	 * 通信に使用可能な鍵種別を算出する
	 */
	candidates = (get_symkey_part(peer_cap) & hostinfo_get_ipmsg_crypt_capability());

	if (speed) {
		/*
		 * 速度優先の場合は, テーブルの末尾から先頭に向かって, 
		 * 鍵探査を行う.
		 */
		first = SYMCRYPT_MAX_KEY_TYPE - 1;
		added_num = -1;
	} else {
		/*
		 * 暗号強度優先の場合は, テーブルの先頭から末尾に向かって, 
		 * 鍵探査を行う.
		 */
		first = 0;
		added_num = 1;
	}
	
	/*
	 * キー選択順序テーブルを探査し, 通信に使用する鍵を決定する.
	 */
	for (i=first;
	     ( (i >= 0) && (i < SYMCRYPT_MAX_KEY_TYPE) && 
		 (!(key_select_table[i] & candidates) ) );
	     i += added_num);

	if ( !( (i >= 0) && (i < SYMCRYPT_MAX_KEY_TYPE) ) ) {
		rc = -ENOENT; /* 鍵が見付からなかった */
		goto error_out;
	}

	/*
	 * 返却
	 */
	*selected_key = key_select_table[i];

	dbg_out("Selected key name in ipmsg:0x%x\n", key_select_table[i]);

error_out:
	return rc;
}


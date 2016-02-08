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

#if !defined(G2IPMSG_PUBCRYPT_H)
#define G2IPMSG_PUBCRYPT_H

/** @file 
 * @brief  公開鍵(非対称鍵)暗号系暗号関連定義
 * @author Takeharu KATO
 */ 

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#if defined(USE_OPENSSL)
#include <openssl/bn.h>
#include <openssl/rsa.h>
#include <openssl/pem.h>

/** RSA鍵生成試行回数
 */
#define RSA_KEYGEN_RETRY    (10)
/** RSA 2048 bits 鍵 インデクス
 */
#define RSA_KEY_INDEX_2048   (0)
/** RSA 1024 bits 鍵 インデクス
 */
#define RSA_KEY_INDEX_1024   (1)
/** RSA 512 bits 鍵 インデクス
 */
#define RSA_KEY_INDEX_512    (2)
/** RSA鍵種別総数
 */
#define RSA_KEY_MAX          (3)
/** RSA暗号化能力ビットマップ
 */
#define RSA_CAPS             (IPMSG_RSA_512|IPMSG_RSA_1024|IPMSG_RSA_2048)
/** 署名能力ビットマップ
 */
#define SIGN_CAPS            (IPMSG_SIGN_MD5|IPMSG_SIGN_SHA1)
/** MD5署名長
 */
#define MD5_DIGEST_LEN       (16)
/** SHA1署名長
 */
#define SHA1_DIGEST_LEN      (20)
/** MD5/SHA1署名共通バッファ長
 */
#define MD5SHA1_DIGEST_LEN   (36)
/** IPMSG_GETPUBKEYパケットのメッセージ部の長さ(ヌルターミネート含む)
 */
#define IPMSG_GETPUBKEY_LEN   (9)

/** 安全でないディレクトリのパーミッション
 */
#define RSA_DIR_INVALID_FLAGS (S_IWOTH|S_IWGRP) 
/** RSA秘密鍵のファイル名プレフィックス
 */
#define RSA_PRIVKEY_FNAME_PREFIX "priv-key-"
/** RSA公開鍵のファイル名プレフィックス
 */
#define RSA_PUBKEY_FNAME_PREFIX  "pub-key-"

/*
 * プロトタイプ宣言
 */
int pcrypt_get_rsa_key_length(ipmsg_cap_t , size_t *);
int pcrypt_crypt_parse_anspubkey(const char *, ipmsg_cap_t *, unsigned char **, unsigned char **);
int pcrypt_crypt_generate_anspubkey_string(ipmsg_cap_t , const char **);
int pcrypt_crypt_generate_getpubkey_string(ipmsg_cap_t , const char **);
int pcrypt_crypt_init_keys(void);
int pcrypt_crypt_release_keys(void);
int pcrypt_crypt_refer_rsa_key_with_index(int , RSA **);
int pcrypt_crypt_refer_rsa_key(ipmsg_cap_t , RSA **);
int pcrypt_encrypt_message(const ipmsg_cap_t , const char *, const char *, const char *, size_t , char **, ipmsg_cap_t *);
int pcrypt_decrypt_message(ipmsg_cap_t , const char *, char **, size_t *);
int  pcrypt_sign(const ipmsg_cap_t , const ipmsg_cap_t , const unsigned char *, unsigned char **);
int pcrypt_verify_sign(const ipmsg_cap_t , const ipmsg_cap_t , const unsigned char *, const unsigned char *, const unsigned char *, const unsigned char *);
int pcrypt_store_rsa_key(const ipmsg_cap_t , const gchar *);
int pcrypt_load_rsa_key(const ipmsg_cap_t , const gchar *);
int select_asymmetric_key(const ipmsg_cap_t , const  RSA *, ipmsg_cap_t *);
int select_signature(ipmsg_cap_t peer_cap,ipmsg_cap_t *selected_algo,int speed);

#else
/** OpenSSL未使用時のRSA暗号化能力ビットマップ
 */
#define RSA_CAPS            (0)
/** OpenSSL未使用時の署名能力ビットマップ
 */
#define SIGN_CAPS           (0)
#endif /*  USE_OPENSSL  */
#endif  /*  G2IPMSG_PUBCRYPT_H  */

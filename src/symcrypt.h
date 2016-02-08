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

#if !defined(G2IPMSG_SYMCRYPT_H)
#define G2IPMSG_SYMCRYPT_H

/** @file 
 * @brief  対称鍵暗号関連定義
 * @author Takeharu KATO
 */ 

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "ipmsg.h"

#if defined(USE_OPENSSL)
#include <openssl/evp.h>

/** 対称鍵種別の数
 */
#define SYMCRYPT_MAX_KEY_TYPE (8)
/** RC2の暗号能力を表すビットマップ
 */
#define RC2_CAPS              (IPMSG_RC2_40|IPMSG_RC2_128|IPMSG_RC2_256)
/** BlowFishの暗号能力を表すビットマップ
 */
#define BLOWFISH_CAPS         (IPMSG_BLOWFISH_128|IPMSG_BLOWFISH_256)
/** AESの暗号能力を表すビットマップ
 */
#define AES_CAPS              (IPMSG_AES_256|IPMSG_AES_192|IPMSG_AES_128)
/** 対称鍵暗号能力を表すビットマップ
 */
#define SYM_CAPS              (RC2_CAPS|BLOWFISH_CAPS|AES_CAPS)

int ipmsg_get_skey_length(const ipmsg_cap_t ,size_t *);
int blowfish_cbc_encrypt_setup(const char *, const size_t , const char *, EVP_CIPHER_CTX **);
int blowfish_cbc_decrypt_setup(const char *, const size_t , const char *, EVP_CIPHER_CTX **);
int aes_cbc_encrypt_setup(const char *, const size_t , const char *, EVP_CIPHER_CTX **);
int aes_cbc_decrypt_setup(const char *, const size_t , const char *, EVP_CIPHER_CTX **);
int rc2_cbc_encrypt_setup(const char *, const size_t , const char *, EVP_CIPHER_CTX **);
int rc2_cbc_decrypt_setup(const char *, const size_t , const char *, EVP_CIPHER_CTX **);
int common_cbc_encrypt(EVP_CIPHER_CTX *, const char *, const int , char **, size_t *);
int common_cbc_decrypt(EVP_CIPHER_CTX *, const char *, const int , char **, size_t *);
int common_cbc_finalize(EVP_CIPHER_CTX **);
int symcrypt_encrypt_message(ipmsg_cap_t , const unsigned char *, char **, size_t *, char **, size_t *);
int symcrypt_decrypt_message(ipmsg_cap_t , const unsigned char *, size_t , const char *, char **, size_t *);
int select_symmetric_key(ipmsg_cap_t , ipmsg_cap_t *, int );
#else
#define SYM_CAPS            (0)
#endif  /*  USE_OPENSSL  */
#endif  /*  G2IPMSG_SYMCRYPT_H  */

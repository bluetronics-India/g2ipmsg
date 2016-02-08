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

#if !defined(G2IPMSG_CRYPTOCOMMON_H)
#define G2IPMSG_CRYPTOCOMMON_H

/** @file 
 * @brief  暗号化処理インターフェース定義
 * @author Takeharu KATO
 */ 

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <sys/types.h>
#include <glib.h>

#include "ipmsg.h"
#include "base64.h"
#include "pubcrypt.h"
#include "symcrypt.h"
#include "pbkdf2.h"


/** 暗号化能力ビットマップ
 */
#define G2IPMSG_CRYPTO_CAP    (RSA_CAPS|SIGN_CAPS|SYM_CAPS)
/** 乱数発生試行回数の最大値
 */
#define CRYPT_RND_MAX_RETRY    (100)
/** 公開鍵/秘密鍵保存ディレクトリ
 */
#define G2IPMSG_KEY_DIR        ".g2ipmsg"

/** 暗号化処理用エラーバッファサイズ
 */
#define G2IPMSG_CRYPT_EBUFSIZ (1024)

/* Common part */

/** 対称鍵暗号能力を暗号化ケイパビリティから取り出す
 *  @param[in]   x             暗号化能力ケイパビリティ
 *  @return                    対称鍵暗号能力を表すビットマップ
 */
#define get_symkey_part(x) ( (x) & (SYM_CAPS) )
/** 公開鍵暗号(非対称鍵暗号)能力を暗号化ケイパビリティから取り出す
 *  @param[in]   x             暗号化能力ケイパビリティ
 *  @return                    公開鍵暗号(非称鍵暗号)能力を表すビットマップ
 */
#define get_asymkey_part(x) ( (x) & (RSA_CAPS) )
/** 署名能力を暗号化ケイパビリティから取り出す
 *  @param[in]   x             暗号化能力ケイパビリティ
 *  @return                    署名能力を表すビットマップ
 */
#define get_sign_part(x) ( (x) & (SIGN_CAPS) )
/** 公開鍵取得までの待ち時間(通信処理に割り当てる1回あたりの経過時間
 * @note 現状では, 1秒間
 */
#define PUBKEY_WAIT_MICRO_SEC (1000UL*1000UL) 
/** 公開鍵取得試行回数
 * @note IPMSG_GETPUBKEY発行後, PUBKEY_WAIT_MICRO_SEC * PUBKEY_MAX_RETRY待つ
 *       (通信処理にCPUを割り当てる). このため, 現状では, 5秒以内に鍵を
 *       獲得できなかった場合は, エラーとなる.
 */
#define PUBKEY_MAX_RETRY                    (5)

#if defined(USE_OPENSSL)
int add_timing_entropy(void);
int generate_rand(unsigned char *, size_t );
int ipmsg_encrypt_message(const char *, const char *, unsigned char **, size_t *);
int ipmsg_decrypt_message(const char *, const char *, unsigned char **, size_t *);
GtkWidget *internal_create_crypt_config_window(void);
int enter_password(void);

#endif  /* USE_OPENSSL  */

#endif  /* IPMSG_CRYPTOCOMMON_H */

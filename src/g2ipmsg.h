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

#if !defined(G2IPMSG_H)
#define G2IPMSG_H

/** @file 
 * @brief  GNOME2 版ipmsgパラメタ定義
 * @author Takeharu KATO
 */ 

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "udp.h"

extern udp_con_t *udp_con;

/** システムトレイ組み込み試行マイクロ秒数
 *  現在のところ1秒
 */
#define STATUS_ICON_WAIT_MICRO_SEC ((1UL)*(1000UL)*(1000UL))
/** グループ情報獲得試行回数
 *  現在のところ20回
 */
#define IPMSG_COMMON_MAX_RETRY (20)
/** 受信通知用アイコン更新間隔
 *  現在のところ500ms
 */
#define MSG_WATCH_INTERVAL (500)
/** g2ipmsgロックファイル
 */
#define IPMG_LOCK_FILE "/tmp/g2ipmsg.lock"
/** キー入力のチャタリング間隔
 * 現在のところ, 300ms
 */
#define IPMSG_KEY_BTN_CHATTER_MS (300)
/** 表示優先度の最小値
 *  -1は非表示を表す
 */
#define USERLIST_PRIO_MIN        (-1)
/** デフォルトの表示優先度
 */
#define USERLIST_DEFAULT_PRIO    (0)
/** 最高表示優先度
 */
#define USERLIST_PRIO_MAX        (4)
/** ダイアログ表示待ち時間(未使用)
 */
#define G2IPMSG_DIALOG_WAIT_MS   (10000)
/** 優先度が健全であることを確認する.
 *  @param[in]  prio_val    環境変数名
 *  @retval  真  健全な優先度である.     
 *  @retval  偽  不正な優先度である.
 */
#define prio_is_valid(prio_val)  ( (prio_val>=USERLIST_PRIO_MIN) && \
				   (prio_val<=USERLIST_PRIO_MAX) )


/** 文字列操作時のデフォルトバッファサイズ
 */
#define IPMSG_BUFSIZ (0x1000)
/** ipmsgのメッセージ交換時のバッファサイズ
 *  @note ソケットバッファのサイズに指定する.
 */
#define _MSG_BUF_SIZE (65536)
/** ipmsgのメッセージ交換時のバッファサイズの最小値
 *  @note ソケットバッファのサイズに指定する.
 */
#define _MSG_BUF_MIN_SIZE ((_MSG_BUF_SIZE)/2)

/** ipmsgのファイル交換時のバッファサイズ(256KB)
 *  @note ソケットバッファのサイズに指定する.
 */
#define _TCP_BUF_SIZE (0x40000) 
/** ipmsgのファイル交換時のバッファサイズの最小値
 *  @note ソケットバッファのサイズに指定する.
 */
#define _TCP_BUF_MIN_SIZE (_TCP_BUF_SIZE/2)
/** ipmsgのファイル交換時のファイルバッファサイズ
 */
#define TCP_FILE_BUFSIZ      (8192)
#if defined(USE_OPENSSL)
/** デフォルトのエントリフラグ(暗号化通信使用時)
 */
#define G2IPMSG_DEFAULT_ENTRY_FLAGS (IPMSG_FILEATTACHOPT|IPMSG_ENCRYPTOPT)
/** デフォルトの送信フラグ(暗号化通信使用時)
 */
#define G2IPMSG_DEFAULT_SEND_FLAGS  (IPMSG_SENDCHECKOPT|IPMSG_ENCRYPTOPT)
#else
/** デフォルトのエントリフラグ
 */
#define G2IPMSG_DEFAULT_ENTRY_FLAGS (IPMSG_FILEATTACHOPT)
/** デフォルトの送信フラグ
 */
#define G2IPMSG_DEFAULT_SEND_FLAGS  (IPMSG_SENDCHECKOPT)
#endif
/** ホストリスト取得長
 */
#define G2IPMSG_DEFAULT_HOST_LIST_LEN 1

/** 起動/メッセージ受信通知音
 */
#define G2IPMSG_SOUND_FILE "g2ipmsg/g2ipmsg.ogg"
/** 不明を表す文字列
 */
#define G2IPMSG_UNKNOWN_NAME _("UnKnown")

int init_ipmsg(void);
void cleanup_ipmsg(void);
int create_lock_file(void);
int release_lock_file(void);
int ipmsg_send_message(const udp_con_t *, const char *, const char *, size_t );
int ipmsg_send_broad_cast(const udp_con_t *, const char *, size_t );
#endif  /*  G2IPMSG_H */

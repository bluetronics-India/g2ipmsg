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

#if !defined(PROTOCOL_H)
/** ヘッダの多重インクルード抑制
 */
#define PROTOCOL_H

/** @file 
 * @brief  IPMSGプロトコル関連定義
 * @author Takeharu KATO
 */ 

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stddef.h>

#include "ipmsg.h"
#include "ipmsg_types.h"
#include "tcp.h"
#include "udp.h"

/** パケット生成時にパケット番号を自動的に割り当てる
 */
#define IPMSG_PROTOCOL_PKTNUM_AUTO    (0)

/** パケット番号文字列長(ヌルターミネート含む)
 */
#define IPMSG_PROTOCOL_PKTNUM_LEN    (11) 
/** パケット番号->文字列変換用書式指定文字列
 */
#define IPMSG_PROTOCOL_PKTNUM_FMT     "%d"
/** エントリ系パケットを表すIPアドレス, 
 *  @note ブロードキャストパケットの場合, 送信先IPアドレスにNULLを指定して
 *        build_ipmsg_packetを呼び出すことで, パケット作成する
 */
#define IPMSG_PROTOCOL_ENTRY_PKT_ADDR   (NULL) 
/** メッセージ部が無い(空文)コマンドであることを示す.
 *  @note メッセージ部が無い(空文)コマンドの場合, メッセージ部にNULLを指定して
 *        build_ipmsg_packetを呼び出すことで, パケット作成する
 */
#define IPMSG_PROTOCOL_MSG_NO_MESSAGE   (NULL) 
/** 拡張部が無い(空文)コマンドであることを示す.
 *  @note 拡張部が無い(空文)コマンドの場合, 拡張部にNULLを指定して
 *        build_ipmsg_packetを呼び出すことで, パケット作成する
 */
#define IPMSG_PROTOCOL_MSG_NO_EXTENSION (NULL) 
/** 生成したパケットのパケット番号を取得しない
 *  @note パケット番号返却アドレスにNULLを指定して
 *        build_ipmsg_packetを呼び出すことで, パケット作成する
 */
#define IPMSG_PROTOCOL_MSG_NONEED_PKTNO (NULL) 


/** IPMSGのパケット形式(全コマンド共通部分)を表す書式指定文字列
 * @note 1:pktno:ユーザ名:ホスト名:送信フラグ を表す文字列.
 */
#define IPMSG_PROTOCOL_COMMON_MSG_FMT    "1:%ld:%s:%s:%lu:"
/** IPMSGのパケット(全体)を作成するための書式指定文字列
 * @note 共通部メッセージ部\0拡張部という形式. 共通部の末尾にデリミタである:を
 *       入れたため, このような定義としている
 */
#define IPMSG_PROTOCOL_ALLMSG_PACKET_FMT "%s%s%c%s"
/** IPMSGのパケットのメッセージ部, 拡張部のデリミタ文字
 * @note 共通部メッセージ部\0拡張部という形式
 */
#define IPMSG_PROTOCOL_MSG_EXT_DELIM      '\0'
/** デバッグ文表示のためのメッセージ部, 拡張部のデリミタ文字
 * @note 共通部メッセージ部\0拡張部という形式
 */
#define IPMSG_PROTOCOL_MSG_EXT_DELIM_SYM  '|'
/** BR_ENTRYの書式指定文字列(通常時の書式)
 * @note 通常時と不在時とで, 書式が変わるため, 別定義を作成している.
 */
#define IPMSG_PROTOCOL_BR_ENTRY_FMT       "%s"
/** BR_ENTRYの書式指定文字列(不在時の書式)
 * @note 通常時と不在時とで, 書式が変わるため, 別定義を作成している.
 */
#define IPMSG_PROTOCOL_BR_ABSENT_FMT      "%s [%s]"
/** 不在文文字列の書式
 */
#define IPMSG_PROTOCOL_BR_ABSENT_FMT_LEN (3) /* 空白 + '[' + ']'の3文字 */
/** 版数返却文字列の書式
 */
#define IPMSG_PROTOCOL_SENDINFO_FMT       "GNOME2 %s (%s)\n%s:%s"


/** 情報系パケット(情報通知)のコマンドを表すビットマップ
 */
#define IPMSG_PROTOCOL_INFOMSG_TYPE       (IPMSG_SENDINFO|IPMSG_SENDABSENCEINFO)
/** 情報系パケット(情報取得要求)のコマンドを表すビットマップ
 */
#define IPMSG_PROTOCOL_GETINFO_TYPE       (IPMSG_GETINFO|IPMSG_GETABSENCEINFO)

/** 何もしないことを表すユーザDB操作(未使用)
 */
#define IPMSG_PROTOCOL_USR_NOP            (0)
/** ユーザ追加を表すユーザDB操作
 */
#define IPMSG_PROTOCOL_USR_ADD            (1)
/** ユーザ情報更新を表すユーザDB操作
 */
#define IPMSG_PROTOCOL_USR_UPDATE         (2)
/** ユーザ情報削除を表すユーザDB操作
 */
#define IPMSG_PROTOCOL_USR_DEL            (3)
/** ホストリストによるユーザ情報追加を表すユーザDB操作
 */
#define IPMSG_PROTOCOL_ADD_WITH_HOSTLIST  (4)
/** IPMSGのコマンド部分を取り出すマスク
 */
#define IPMSG_PROTOCOL_COMMAND_MASK         (0xffUL)
/** IPMSGのコマンドオプション部分を取り出すマスク
 */
#define IPMSG_PROTOCOL_COMMAND_OPT_MASK     (0xffffff00UL)
/** IPMSGのコマンド種別を取り出すマスク
 */
#define IPMSG_PROTOCOL_COMMAND_TYPE_MASK    (0xf0UL)
/** エントリパケット種別
 */
#define IPMSG_PROTOCOL_COMMAND_ENTRY_TYPE   (0x00UL)
/** ホストリストパケット種別
 */
#define IPMSG_PROTOCOL_COMMAND_HLIST_TYPE   (0x10UL)
/** メッセージ送信パケット種別
 */
#define IPMSG_PROTOCOL_COMMAND_SEND_TYPE    (0x20UL)
/** メッセージ受信確認パケット種別
 */
#define IPMSG_PROTOCOL_COMMAND_RECV_TYPE    (0x30UL)
/** 情報送信パケット種別
 */
#define IPMSG_PROTOCOL_COMMAND_INFO_TYPE    (0x40UL)
/** 不在通知パケット種別
 */
#define IPMSG_PROTOCOL_COMMAND_ABSENCE_TYPE (0x50UL)
/** ファイル添付パケット種別
 */
#define IPMSG_PROTOCOL_COMMAND_FILE_TYPE    (0x60UL)
/** 暗号通信基盤パケット種別
 */
#define IPMSG_PROTOCOL_COMMAND_CRYPT_TYPE   (0x70UL)

/** パケットからコマンド種別を取り出す
 * @param[in] flags フラグ値
 * @return フラグ中のコマンド部分
 */
#define ipmsg_protocol_flags_get_command(flags) \
  ( ((ipmsg_command_t)(flags)) & (IPMSG_PROTOCOL_COMMAND_MASK) ) 

/** パケットの送信フラグからコマンドオプションを取り出す
 * @param[in] flags フラグ値
 * @return フラグ中のコマンドオプション部分
 */
#define ipmsg_protocol_flags_get_opt(flags)   \
  ( ((ipmsg_command_t)(flags)) & (IPMSG_PROTOCOL_COMMAND_OPT_MASK) ) 

 
pktno_t ipmsg_get_pkt_no(void);
int ipmsg_send_release_files(const udp_con_t *, const char *, int );
int ipmsg_send_br_entry(const udp_con_t *, const int );
int ipmsg_send_gratuitous_ans_entry(const udp_con_t *, const char *, const int );
int ipmsg_send_br_exit(const udp_con_t *, const int );
int ipmsg_send_br_absence(const udp_con_t *, const int );
int ipmsg_send_read_msg(const udp_con_t *, const char *, pktno_t );
int ipmsg_send_send_msg(const udp_con_t *, const char *, int , int , const char *, const char *);
int ipmsg_dispatch_message(const udp_con_t *, const msg_data_t *);
int ipmsg_send_get_info_msg(const udp_con_t *, const char *, ipmsg_command_t );
int ipmsg_send_getpubkey(const udp_con_t *, const char *);
int ipmsg_send_get_list(const udp_con_t *, const char *, const int );
int ipmsg_construct_getfile_message(const tcp_con_t *, const char *, ipmsg_ftype_t , const char *, ipmsg_send_flags_t , size_t *, char **);
#endif  /* PROTOCOL_H  */

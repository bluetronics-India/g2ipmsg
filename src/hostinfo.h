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

#if !defined(HOSTINFO_H)
#define HOSTINFO_H
#include <glib.h>

#define DEFAULT_PORT 2425

#define PATH "/apps/g2ipmsg"
#define HOSTINFO_KEY_PORT          "/apps/g2ipmsg/portno"
#define HOSTINFO_KEY_GROUP         "/apps/g2ipmsg/group"
#define HOSTINFO_KEY_NICKNAME      "/apps/g2ipmsg/nickname"
#define HOSTINFO_KEY_MSGSEC        "/apps/g2ipmsg/secret"
#define HOSTINFO_KEY_CONFIRM_MSG   "/apps/g2ipmsg/confirm"
#define HOSTINFO_KEY_BROADCASTS    "/apps/g2ipmsg/broadcasts" /* ブロードキャスト */
#define HOSTINFO_KEY_POPUP         "/apps/g2ipmsg/popup"
#define HOSTINFO_KEY_SOUND         "/apps/g2ipmsg/sound"
#define HOSTINFO_KEY_ENCLOSE       "/apps/g2ipmsg/enclose"
#define HOSTINFO_KEY_CITATION      "/apps/g2ipmsg/cite"
#define HOSTINFO_KEY_LOGFILEPATH   "/apps/g2ipmsg/logfilepath"
#define HOSTINFO_KEY_ENABLE_LOG    "/apps/g2ipmsg/enablelog"
#define HOSTINFO_KEY_LOG_NAME      "/apps/g2ipmsg/enablelogname" /* ログオン名記録 */
#define HOSTINFO_KEY_LOG_IPADDR    "/apps/g2ipmsg/enablelogaddr" /* アドレス記録 */
#define HOSTINFO_KEY_CITE_STRING   "/apps/g2ipmsg/citestring" /* 引用文字 */
#define HOSTINFO_KEY_ABS_TITLE   "/apps/g2ipmsg/absence_titles" /* 不在タイトル */
#define HOSTINFO_KEY_ABS_MSGS   "/apps/g2ipmsg/absence_messages" /* 不在文 */
#define HOSTINFO_KEY_DEBUG   "/apps/g2ipmsg/enable_debug" /* デバッグ文 */
#define HOSTINFO_KEY_MSGWIN_WIDTH   "/apps/g2ipmsg/msg_win_width" /* メッセージウィンドウの幅 */
#define HOSTINFO_KEY_MSGWIN_HEIGHT   "/apps/g2ipmsg/msg_win_height" /* メッセージウィンドウの高さ */
#define HOSTINFO_KEY_RECVWIN_WIDTH   "/apps/g2ipmsg/recv_win_width" /* 受信ウィンドウの幅 */
#define HOSTINFO_KEY_RECVWIN_HEIGHT   "/apps/g2ipmsg/recv_win_height" /* 受信ウィンドウの高さ */
#define HOSTINFO_KEY_ATTACHWIN_WIDTH   "/apps/g2ipmsg/attach_win_width" /* 添付ファイルエディタの幅 */
#define HOSTINFO_KEY_ATTACHWIN_HEIGHT   "/apps/g2ipmsg/attach_win_height" /* 添付ファイルエディタの高さ */
#define HOSTINFO_KEY_ABSENCE_WIDTH   "/apps/g2ipmsg/absence_win_width" /* 不在文エディタの幅 */
#define HOSTINFO_KEY_ABSENCE_HEIGHT   "/apps/g2ipmsg/absence_win_height" /* 不在文エディタの高さ */
#define HOSTINFO_KEY_IPV6             "/apps/g2ipmsg/ipv6" /* ipv6モード */
#define HOSTINFO_KEY_DIALUP           "/apps/g2ipmsg/dialup" /* ダイアルアップモード */
#define HOSTINFO_KEY_GET_HLIST        "/apps/g2ipmsg/get_host_list" /* ホストリスト取得を実施する  */
#define HOSTINFO_KEY_ALLOW_HLIST      "/apps/g2ipmsg/allow_host_list" /* ホストリストを送信する  */
#define HOSTINFO_KEY_DEFAULT_PRIO     "/apps/g2ipmsg/default_prio" /* 表示優先度  */
#define HOSTINFO_KEY_USE_SYSTRAY      "/apps/g2ipmsg/use_systray" /* システムトレイに常駐する  */
#define HOSTINFO_KEY_HEADER_VISIBLE   "/apps/g2ipmsg/header_visible" /* ヘッダ表示  */
#define HOSTINFO_KEY_HEADER_ORDER     "/apps/g2ipmsg/header_order" /* ヘッダ順序  */
#define HOSTINFO_KEY_SORT_GROUP     "/apps/g2ipmsg/sort_with_group" /* グループでソートする  */
#define HOSTINFO_KEY_SUB_SORT_ID     "/apps/g2ipmsg/sub_sort_id" /* ソート種別  */
#define HOSTINFO_KEY_SORT_GROUP_DESCENDING "/apps/g2ipmsg/group_sort_descending" /* グループは逆順でソートする  */
#define HOSTINFO_KEY_SUB_SORT_DESCENDING   "/apps/g2ipmsg/sub_sort_descending" /* サブソート種別は逆順にする  */
#define HOSTINFO_KEY_CRYPT_SPEED           "/apps/g2ipmsg/crypt_policy_speed" /* 速度優先で選択 */
#define HOSTINFO_KEY_CIPHER                "/apps/g2ipmsg/ciphers" /* 暗号化方式 */
#define HOSTINFO_KEY_ENCRYPT_PUBKEY        "/apps/g2ipmsg/encrypt_publickey" /* 秘密/公開鍵を暗号化して保存  */
#define HOSTINFO_KEY_LOCK                  "/apps/g2ipmsg/lock" /* 錠を使用する  */
#define HOSTINFO_KEY_ENC_PASSWD            "/apps/g2ipmsg/enc_pass"  /* 公開鍵暗号化鍵パスワード  */
#define HOSTINFO_KEY_LOCK_PASSWD           "/apps/g2ipmsg/lock_pass" /* 錠パスワード  */
#define HOSTINFO_KEY_LOCK_MSGLOG           "/apps/g2ipmsg/loglockedmessage" /* 錠付きメッセージは開封後ログをとる  */
#define HOSTINFO_KEY_ICONIFY_DIALOGS       "/apps/g2ipmsg/iconify_dialogs" /* 通常ダイアログをアイコン化する  */
#define HOSTINFO_KEY_EXTERNAL_ENCODING     "/apps/g2ipmsg/external_encoding" /* 外部エンコード形式  */

#define HOSTINFO_PRIO_SEPARATOR  '@'
#define HEADER_VISUAL_GROUP_ID     0x1
#define HEADER_VISUAL_HOST_ID      0x2
#define HEADER_VISUAL_IPADDR_ID    0x4
#define HEADER_VISUAL_LOGON_ID     0x8
#define HEADER_VISUAL_PRIO_ID     0x10
#define HEADER_VISUAL_GRID_ID     0x20

#define SORT_TYPE_USER              0x0
#define SORT_TYPE_MACHINE           0x1
#define SORT_TYPE_IPADDR            0x2

#define HOSTINFO_PASSWD_TYPE_ENCKEY 0x1
#define HOSTINFO_PASSWD_TYPE_LOCK   0x2

int hostinfo_refer_ipmsg_port(void);
const char *hostinfo_refer_user_name(void);
const char *hostinfo_refer_group_name(void);
const char *hostinfo_refer_nick_name(void);
const char *hostinfo_refer_host_name(void);
const char *hostinfo_refer_ipmsg_cite_string(void);
gboolean hostinfo_refer_ipmsg_default_secret(void);
gboolean hostinfo_refer_ipmsg_default_confirm(void);
gboolean hostinfo_refer_ipmsg_default_popup(void);
gboolean hostinfo_refer_ipmsg_default_sound(void);
gboolean hostinfo_refer_ipmsg_default_enclose(void);
gboolean hostinfo_refer_ipmsg_default_citation(void);
gboolean hostinfo_refer_ipmsg_ipv6_mode(void);
gboolean hostinfo_refer_ipmsg_is_get_hlist(void);
gboolean hostinfo_refer_ipmsg_is_allow_hlist(void);
gboolean hostinfo_refer_ipmsg_dialup_mode(void);
gboolean hostinfo_refer_ipmsg_ipaddr_logging(void);
gboolean hostinfo_refer_ipmsg_logname_logging(void);
gboolean hostinfo_refer_ipmsg_enable_log(void);
gboolean hostinfo_refer_enable_systray(void);
guint    hostinfo_refer_header_state(void);
gboolean hostinfo_refer_ipmsg_is_sort_with_group(void);
gint     hostinfo_refer_ipmsg_sub_sort_id(void);
gboolean hostinfo_refer_ipmsg_group_sort_order(void);
gboolean hostinfo_refer_ipmsg_sub_sort_order(void);
gboolean hostinfo_set_ipmsg_default_secret(gboolean val);
gboolean hostinfo_set_ipmsg_default_confirm(gboolean val);
gboolean hostinfo_set_ipmsg_default_popup(gboolean val);
gboolean hostinfo_set_ipmsg_default_sound(gboolean val);
gboolean hostinfo_set_ipmsg_default_enclose(gboolean val);
gboolean hostinfo_set_ipmsg_default_citation(gboolean val);
gboolean hostinfo_set_ipmsg_ipv6_mode(gboolean val);
gboolean hostinfo_set_ipmsg_is_get_hlist(gboolean val);
gboolean hostinfo_set_ipmsg_is_allow_hlist(gboolean val);
gboolean hostinfo_set_ipmsg_dialup_mode(gboolean val);
gboolean hostinfo_set_ipmsg_ipaddr_logging(gboolean val);
gboolean hostinfo_set_ipmsg_logname_logging(gboolean val);
gboolean hostinfo_set_ipmsg_enable_log(gboolean val);
gboolean hostinfo_set_enable_systray(gboolean val);
gboolean hostinfo_set_header_state(guint val);
gboolean hostinfo_set_ipmsg_sort_with_group(gboolean val);
gboolean hostinfo_set_ipmsg_sub_sort_id(gint val);
gboolean hostinfo_set_ipmsg_group_sort_order(gboolean val);
gboolean hostinfo_set_ipmsg_sub_sort_order(gboolean val);
int hostinfo_set_ipmsg_logfile(const char *file);
int hostinfo_set_ipmsg_broadcast_list(GSList *list);
GSList* hostinfo_get_ipmsg_broadcast_list(void);
const char *hostinfo_refer_ipmsg_logfile(void);
unsigned long hostinfo_get_normal_send_flags(void);
unsigned long hostinfo_get_normal_entry_flags(void);
int hostinfo_set_group_name(const char *groupName);
int hostinfo_set_nick_name(const char *nickName);
int hostinfo_set_ipmsg_port(int port);
int hostinfo_send_broad_cast(const udp_con_t *con,const char *msg,size_t len);

gboolean hostinfo_is_ipmsg_absent(void);
gboolean hostinfo_set_ipmsg_absent(gboolean state);

int hostinfo_get_absent_id(int *index);
int hostinfo_set_absent_id(int index);
int hostinfo_get_absent_title(int index,const char **title);
int hostinfo_set_ipmsg_absent_title(int index,const char *title);
int hostinfo_refer_absent_length(int *max_index);
int hostinfo_refer_absent_message_slots(int *max_index);
int hostinfo_get_absent_message(int index, char **message);
int hostinfo_set_ipmsg_absent_message(int index,const char *message);
int hostinfo_get_current_absence_message(char **message_p);
gboolean hostinfo_refer_debug_state(void);

int hostinfo_get_ipmsg_message_window_size(gint *width,gint *height);
int hostinfo_set_ipmsg_message_window_size(gint width,gint height);
int hostinfo_get_ipmsg_recv_window_size(gint *width,gint *height);
int hostinfo_set_ipmsg_recv_window_size(gint width,gint height);
int hostinfo_get_ipmsg_attach_editor_size(gint *width,gint *height);
int hostinfo_set_ipmsg_attach_editor_size(gint width,gint height);
int hostinfo_get_ipmsg_absence_editor_size(gint *width,gint *height);
int hostinfo_set_ipmsg_absence_editor_size(gint width,gint height);
int hostinfo_get_ipmsg_system_addr_family(void);
int hostinfo_update_ipmsg_ipaddr_prio(const char *ipaddr,int prio);
int hostinfo_get_ipmsg_ipaddr_prio(const char *ipaddr,int *prio);
int hostinfo_get_header_order(int index,int *col_id);
int hostinfo_set_ipmsg_header_order(int index,int col_id);
gboolean hostinfo_refer_ipmsg_crypt_policy_is_speed(void);
int hostinfo_set_ipmsg_crypt_policy_as_speed(gboolean val);
gboolean hostinfo_refer_ipmsg_encrypt_public_key(void);
int hostinfo_set_ipmsg_encrypt_public_key(gboolean val);
gboolean hostinfo_refer_ipmsg_use_lock(void);
int hostinfo_set_ipmsg_use_lock(gboolean val);
unsigned long hostinfo_get_ipmsg_crypt_capability(void);
gboolean is_sound_system_available(void);
int hostinfo_refer_ipmsg_cipher(unsigned long *cipher);
int hostinfo_set_ipmsg_cipher(unsigned long val);
const char *hostinfo_refer_encryption_key_password(void);
int hostinfo_set_encryption_key_password(const char *encoded_password);
const char *hostinfo_refer_lock_password(void);
int hostinfo_set_lock_password(const char *encoded_password);
gboolean hostinfo_refer_ipmsg_iconify_dialogs(void);
gboolean hostinfo_set_ipmsg_iconify_dialogs(gboolean val);
gboolean hostinfo_refer_ipmsg_log_locked_message_handling(void);
gboolean hostinfo_set_log_locked_message_handling(gboolean val);
int hostinfo_set_encoding(const char *encoding);
const char *hostinfo_refer_encoding(void);

int hostinfo_init_hostinfo(void);
void hostinfo_cleanup_hostinfo(void);
#endif  /* HOSTINFO_H  */

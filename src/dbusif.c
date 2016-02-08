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
 * @brief  DBUS通信インターフェース関連関数群
 * @author Takeharu KATO
 */ 

/** 指定されたDBUS バスへのコネクションを取得する.
 *  @param[in]      type   コネクション種別
 *                         DBUS_BUS_SESSION ログインセッションバス
 *                         DBUS_BUS_SYSTEM  システム全体で使用する共通バス
 *                         DBUS_BUS_STARTER 上記以外に独自に開設したバス
 *  @param[in,out]  con_p  コネクション情報へのポインタ変数のアドレス
 *  @retval  0      正常終了
 *  @retval -EINVAL 不正なtypeを指定したか, con_pがNULLである.
 *  @retval -EEXIST コネクション(バス)の生成に失敗
 *  @attention 内部リンケージ
 */
static int
ipmsg_dbus_create_bus(DBusBusType type, DBusGConnection **con_p){
  	DBusGConnection *connection = NULL;
	GError               *error = NULL;
	int                      rc = 0;

	if ( (type != DBUS_BUS_SESSION) &&
	    (type != DBUS_BUS_SYSTEM)   &&
	    (type != DBUS_BUS_STARTER) ) {
		rc = -EINVAL;
		goto error_out;
	}

	if (con_p == NULL) {
		rc = -EINVAL;
		goto error_out;
	}

	/* Get a connection to the session bus */
	error = NULL;
	connection = dbus_g_bus_get(type, &error);
	if (connection == NULL) {
	    dbg_out("Error:%s\n", error->message);
	    g_error_free(error);
	    rc = -EEXIST;
	    goto error_out;
	}
	
	*con_p = connection;  /* コネクションを返却  */
	rc = 0;

error_out:
	return rc;
}

/** 指定されたサービスに接続し, 通信に使用するハンドル(DBUSプロキシ)を返却する
 *  @param[in]       bus       サービスが存在するバス
 *  @param[in]       name      サービス名
 *  @param[in]       path      サービスパス
 *  @param[in]       interface インターフェース名
 *  @param[in, out]  handle_p 通信に使用するハンドル(プロキシ)情報の返却先アドレス
 *  @retval  0      正常終了
 *  @retval -EINVAL bus, name, path, interfaceのいずれかがNULLである.
 *  @retval -ENOENT 接続に失敗した
 *  @attention 内部リンケージ
 */
static int
ipmsg_connect_dbus_service(DBusGConnection *bus, 
    const char *name, const char *path, const char *interface,
    DBusGProxy **handle_p) {
	DBusGProxy *proxy = NULL;
	GError     *error = NULL;
	int            rc = 0;

	if ( (bus == NULL) || (name == NULL) || (path == NULL) 
	    || (interface == NULL) ) {
		rc = -EINVAL;
		goto error_out;
	}

	dbg_out("Create dbus proxy:(name. path, interface) = (%s, %s, %s)\n",
	    name, path, interface);

	proxy = dbus_g_proxy_new_for_name(bus, name, path, interface);
	if (proxy == NULL) {
		rc = -ENOENT;
		goto error_out;
	}

error_out:
	return rc;
}

/** 返り値のない電文をDBUS経由で送信する
 *  @param[in]      proxy 通信に使用するハンドル(プロキシ)情報の返却先アドレス
 *  @param[in]      msg   通信電文
 *  @param[in]      start_arg   可変個引数開始位置
 *  @param[in]      ...   可変個引数
 *  @retval  0      正常終了
 *  @retval -EINVAL proxy, msgのいずれかがNULLである.
 *  @retval -ENOENT 接続に失敗した
 *  @attention 内部リンケージ
 */
static int
ipmsg_send_dbus_no_response(const DBusGProxy *proxy, const gchar *msg, 
    GType start_arg, ...){

}

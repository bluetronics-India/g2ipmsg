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
 * @brief  スクリーンセーバ連系関連関数群
 * @author Takeharu KATO
 */ 

/*
 * Following definitions are obtained from 
 * http://lists.freedesktop.org/archives/xdg/2006-June/006523.html
 */
#define SCREEN_SAVER_DBUS_SERVICE "org.gnome.ScreenSaver"
#define SCREEN_SAVER_DBUS_OBJ_PATH "/org/gnome/ScreenSaver"
#define SCREEN_SAVER_DBUS_IF       "org.gnome.ScreenSaver"
#define SCREEN_SAVER_GETACTIVATE_MSG "GetActive"

/** スクリーンセーバが起動中であることを確認する
 *  @param[in,out]  state_p    スクリーンセーバの動作状態返却領域のアドレス
 *  @retval  0      正常終了
 *  @retval -EEXIST コネクション(バス)の生成に失敗
 *  @retval -ENOENT スクリーンセーバがGetActiveを解釈できなかった.
 */
int
check_screensaver_active(gboolean *state_p) {
	DBusGConnection *connection = NULL;
	GError *error = NULL;
	DBusGProxy *proxy = NULL;
	gboolean ret = FALSE;
	gboolean state = FALSE;
	int         rc = 0;

	/* Get a connection to the session bus */
	error = NULL;
	connection = dbus_g_bus_get(DBUS_BUS_SESSION, &error);
	if (connection == NULL) {
	    dbg_out("Error:%s\n", error->message);
	    g_error_free(error);
	    rc = -EEXIST;
	    goto error_out;
	}

    /* Create proxy */
    proxy = dbus_g_proxy_new_for_name(connection, SCREEN_SAVER_DBUS_SERVICE,
	SCREEN_SAVER_DBUS_OBJ_PATH, SCREEN_SAVER_DBUS_IF);

    /*
     * getActive command specification is described as follow:
     *
     * Name:           getActive
     * Args:           (none)
     * Returns:        DBUS_TYPE_BOOLEAN
     * Descriptions:   Returns the value of the current state of activity.
     *                 (boolean value: TRUE ... active/FALSE ... inactive)
     */
    error = NULL;
    ret = dbus_g_proxy_call(proxy, SCREEN_SAVER_GETACTIVATE_MSG, 
	&error, G_TYPE_INVALID, G_TYPE_BOOLEAN, &state, G_TYPE_INVALID);
        
    if (ret == 0) {
	    /* Error handling */
	    dbg_out("Error :%s %s\n", 
		dbus_g_error_get_name(error), 
		error->message);
	    rc = -ENOENT;
	    g_error_free(error);
	    goto error_out;
    }
    

    g_object_unref(proxy);
    if (state)
	    dbg_out("Screen saver is Active\n");
    else
	    dbg_out("Screen saver is InActive\n");

    *state_p = state;

    rc = 0;

error_out:
    return rc;
}


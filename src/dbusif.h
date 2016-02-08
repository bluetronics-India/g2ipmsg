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

#if !defined(G2IPMSG_DBUSIF_H)
#define G2IPMSG_DBUSIF_H

/** @file 
 * @brief  DBUS通信インターフェース関連定義
 * @author Takeharu KATO
 */ 


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#if defined(USE_DBUS)

#if !defined(DBUS_API_SUBJECT_TO_CHANGE)
#define DBUS_API_SUBJECT_TO_CHANGE
#endif

#include <errno.h>
#include <glib.h>
#include <gnome.h>

#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>

#include "msgout.h"

/** @brief DBUSコネクション管理用構造体
 *  @param[in]      name        サービス名
 *  @param[in]      path        サービスパス
 *  @param[in]      interface   インタフェース名
 *  @param[in, out] bus         バス名
 *  @param[in, out] proxy       ハンドル(プロキシ)
 */
typedef struct _dbus_con_t{
  gchar          *name;
  gchar          *path;
  gchar          *interface;
  DBusGConnection *bus;
  DBusGProxy      *proxy;
}dbus_con_t;

#endif  /*  USE_DBUS  */

#endif  /*  G2IPMSG_DBUSIF_H  */

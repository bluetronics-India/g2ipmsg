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

#ifndef COMMON_H
#define COMMON_H

/** @file 
 * @brief  G2IPMSG common definition
 * @author Takeharu KATO
 */ 

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <arpa/inet.h>
#include <bonobo.h>
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <gconf/gconf-client.h>
#include <gdk/gdkx.h>
#include <glib.h>
#include <gnome.h>
#include <iconv.h>
#include <libgnomeui/gnome-help.h>
#include <libgnomevfs/gnome-vfs.h>
#include <libgnomevfs/gnome-vfs-utils.h>
#include <libgnomevfs/gnome-vfs-mime-handlers.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <pwd.h>
#include <signal.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <sys/ioctl.h> 
#include <sys/mman.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <syslog.h>
#include <time.h>
#include <unistd.h>

#if defined(HAVE_LIBINTL_H)
#include  <libintl.h>
#endif  /*  HAVE_LIBINTL_H  */

#if defined(ENABLE_APPLET)
#include <panel-applet.h>
#endif  /*  ENABLE_APPLET  */

#if defined(USE_OPENSSL)
#include <openssl/bn.h>
#include <openssl/engine.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/md5.h>
#include <openssl/pem.h>
#include <openssl/rand.h>
#include <openssl/rsa.h>
#include <openssl/sha.h>
#endif /* USE_OPENSSL */

#if defined(USE_DBUS)

#if !defined(DBUS_API_SUBJECT_TO_CHANGE)
#define DBUS_API_SUBJECT_TO_CHANGE
#endif  /*  DBUS_API_SUBJECT_TO_CHANGE  */

#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>
#endif  /*  USE_DBUS  */

#include "callbacks.h"
#include "interface.h"
#include "support.h"

#include "ipmsg_types.h"
#include "ipmsg.h"
#include "g2ipmsg.h"
#include "udp.h"
#include "message.h"
#include "userdb.h"
#include "hostinfo.h"
#include "msginfo.h"
#include "logfile.h"
#include "menu.h"
#include "fileattach.h"
#include "tcp.h"
#include "netcommon.h"
#include "fuzai.h"
#include "copying.h"
#include "uicommon.h"
#include "sound.h"
#include "systray.h"
#include "downloads.h"
#include "codeset.h"
#include "protocol.h"
#include "msgout.h"
#include "private.h"
#include "compat.h"
#include "util.h"
#include "base64.h"
#include "cryptcommon.h"
#include "dbusif.h"
#include "screensaver.h"
#endif  /* COMMON_H */

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

#if !defined(DOWNLOADS_H)
#define DOWNLOADS_H

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <glib.h>
#include <gnome.h>

typedef struct _dir_request{
  long header_size;
  gchar *filename;
  off_t file_size;
  unsigned long ipmsg_fattr;
}dir_request_t;
#define download_base_cmd(attr) ((attr)&(0xff))
#define DOWNLOAD_FILEID_NUM     0
#define DOWNLOAD_FILENAME_NUM   1
#define DOWNLOAD_RECVSIZE_NUM   2
#define DOWNLOAD_FILESIZE_NUM   3
#define DOWNLOAD_PROGRESS_NUM   4
#define DOWNLOAD_FILEATTR_NUM   5
#define DOWNLOAD_FILETYPE_NUM   6

int internal_create_download_window(GtkWindow *recvWin,GtkWindow **window);
int download_dir_open_operation(const char *dir);
int download_file_ok_operation(const char *file,const char *filepath, off_t size,const char *dir);
int download_file_failed_operation(const char *filepath,const char *basename);
int post_download_operation(int code,const char *filepath, off_t size, off_t all_size,const char *dir,gboolean exec);
int do_download_regular_file(tcp_con_t *con, const char *filepath, off_t file_size, off_t *written,GtkTreeModel *model,GtkTreeIter *iter);
int send_download_request(tcp_con_t *con, const char *ipaddr, unsigned long ftype,long pkt_no,int fileid);
int do_download_directory(tcp_con_t *con, const char *top_dir,GtkTreeModel *model,GtkTreeIter *iter);
gboolean is_supported_file_type(unsigned long ipmsg_fattr);
void recv_attachments(gpointer data);
#endif 

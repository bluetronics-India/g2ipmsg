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

#if !defined(FILEATTACH_H)
#define FILEATTACH_H

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <glib.h>

#include "ipmsg_types.h"

#define FILE_ATTCH_ONE_EXT_SIZE 512
#define FATTACH_BUFF_LEN 1024
#define DOWNLOAD_VIEW_FNAME  (0)
#define DOWNLOAD_VIEW_REMAIN (1)
#define DOWNLOAD_VIEW_USER   (2)
#define DOWNLOAD_VIEW_PKTNO  (3)

typedef struct _attach_file_block{
  guint count;
  GMutex *mutex;
  int max_id;
  pktno_t pkt_no;
  GList *files;  /*ファイル情報*/
  gchar *ipaddr; 
}attach_file_block_t;

typedef struct _download_file_info{ /* ダウンロードファイル情報 */
  int fileid;
  gchar *filename;  
  off_t size;
  time_t m_time;
  ipmsg_ftype_t ipmsg_fattr;
}download_file_block_t;

typedef struct _file_info{
  GMutex *mutex;
  attach_file_block_t *main_info_ref;
  download_file_block_t *dfcb_ref;
  int fileid;
  gchar *filepath;
  gchar *filename;
  off_t size;
  time_t m_time;
  ipmsg_ftype_t ipmsg_fattr;
  off_t offset;  /*未使用*/
  GList *xattrs;  /*未使用*/
}file_info_t;

int create_attach_file_block(attach_file_block_t **afcb);
int destroy_attach_file_block(attach_file_block_t **afcb);
int release_attach_file_block(const pktno_t pktno,gboolean force);
int ref_attach_file_block(pktno_t pktno,const char *ipaddr);
int unref_attach_file_block(pktno_t pktno);
int set_attch_file_block_pktno(attach_file_block_t *afcb,const pktno_t pktno);
int add_attach_file(attach_file_block_t *afcb,const gchar *path);
int remove_attach_file(attach_file_block_t *afcb,const gchar *path);
int release_attach_file(const pktno_t pktno,int fileid);
int get_attach_file_extention(attach_file_block_t *afcb,const gchar **ext_string);
int refer_attach_file(const pktno_t pktno,int fileid, ipmsg_ftype_t *ipmsg_fattr,const char **path,off_t *size);
int destroy_download_list(const GList **download_list);
int parse_download_string(const char *string, GList **list);
int get_file_info(const gchar *path, off_t *size, time_t *mtime, ipmsg_ftype_t *ipmsg_type);
void show_file_list(attach_file_block_t *afcb);
int add_upload_queue(pktno_t pktno,attach_file_block_t *afcb) ;
GList *get_download_monitor_info(void);
const gchar *get_file_type_name(ipmsg_ftype_t fattr);
#endif  /*  FILEATTACH_H  */

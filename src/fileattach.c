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

/** 不明なファイル種別
 */
#define IPMSG_UNKNWON_FATTR (0)

/** ファイル種別コード->ファイルしゅ別文字列変換テーブル
 *  @attention 内部リンケージ
 */
static const gchar *str_ipmsg_fattr[]={
  "UnKnown",
  "Regular",
  "Directory",
  "ReturnParent",
  "SymLink",
  "Character Device",
  "Block Device",
  "FIFO",
  "ResFork",};

/** アップロードリストのスレッド間排他ロック
 *  @attention 内部リンケージ
 */
GList *uploads=NULL;
/** ダウンロードリストスレッド間排他ロック
 *  @attention 内部リンケージ
 */
GList *downloads=NULL;
/** アップロードキュースレッド間排他ロック
 *  @attention 内部リンケージ
 */
GStaticMutex upload_queue_mutex = G_STATIC_MUTEX_INIT;
/** ダウンロードロードキュースレッド間排他ロック
 *  @attention 内部リンケージ
 */
GStaticMutex download_queue_mutex = G_STATIC_MUTEX_INIT;
/* utils */

/* basic operations */
static int
create_file_info(file_info_t **file_info) {
	int rc = 0;
	file_info_t *new_info = NULL;

	if (file_info == NULL) {
		rc = -EINVAL;
		goto no_free_out;
	}

	new_info = g_slice_new(file_info_t);
	if (new_info == NULL) {
		rc = -ENOMEM;
		goto no_free_out;
	}
	memset(new_info, 0, sizeof(file_info_t));
	new_info->mutex = g_mutex_new();

	if (new_info->mutex == NULL) {
		rc = -ENOMEM;
		goto free_info_out;
	}

	*file_info = new_info;

	goto no_free_out;

free_info_out:
	dbg_out("Free: %x\n", (unsigned int)new_info);

	g_slice_free(file_info_t, new_info);

no_free_out:
	return rc;
}

static int
destroy_file_info(file_info_t *file_info) {
  GList *entry,*xattr_list;

  if (!file_info)
    return -EINVAL;

  if (!g_mutex_trylock(file_info->mutex))
    return -EBUSY;

  xattr_list=file_info->xattrs;
  for(entry=g_list_first(xattr_list);entry;entry=g_list_next(entry)) {
    if (entry->data) 
      g_free(entry->data);
  }
  g_list_free(xattr_list);
  g_assert(file_info->xattrs==NULL);

  g_mutex_unlock(file_info->mutex);
  dbg_out("Destroy :%x\n",(unsigned int)file_info);
  g_mutex_free (file_info->mutex);

  if (file_info->filepath)
    g_free(file_info->filepath);
  if (file_info->filename)
    g_free(file_info->filename);

  dbg_out("Free: %x\n",(unsigned int)file_info);
  g_slice_free(file_info_t,file_info);
  memset(file_info,0,sizeof(file_info_t));
  return 0;
}
static gint
check_duplicate_attach(gconstpointer a,gconstpointer b){
  file_info_t *info_a,*info_b;

  if ( (!a) || (!b) )
    return -EINVAL;

  info_a=(file_info_t *)a;
  info_b=(file_info_t *)b;
  
  if ( (!(info_a->filepath)) ||  (!(info_b->filepath)) )
        return -EINVAL;

  return strcmp(info_a->filepath,info_b->filepath);
}
static int
setup_file_info(int newid,file_info_t *new_info,const gchar *path){
  int rc;
  char *filename;
  char *fname_sp;
  char *new_path;
  off_t this_size;
  time_t this_mtime;
  ipmsg_ftype_t  this_type;

  if ( (!path) || (!new_info) )
    return -EINVAL;

  rc=get_file_info(path, &this_size, &this_mtime, &this_type);
  if (rc<0)
    return rc;

  rc=-ENOMEM;
  fname_sp=strrchr((const char *)path,'/');
  if (fname_sp) {
    filename=g_strdup(fname_sp+1);
    if (!(filename))
      return rc;
  }else{
    filename=g_strdup(path);
    if (!(filename))
      return rc;
  }
  new_path=g_strdup(path);
  if (!new_path)
    goto free_filename_out;

  rc=-EBUSY;
  if (!g_mutex_trylock(new_info->mutex)) {
    goto free_filepath_out;    
  }
  new_info->fileid=newid;
  new_info->filename=filename;
  new_info->filepath=new_path;
  new_info->size=this_size;
  new_info->m_time=this_mtime;
  new_info->ipmsg_fattr=this_type;
  dbg_out("New attached file:id=%d,name=%s,path=%s,size=0x%llx,mtime=%x\n",
	  new_info->fileid,
	  new_info->filename,
	  new_info->filepath,
	  new_info->size,
	  (unsigned int)new_info->m_time);
  g_mutex_unlock(new_info->mutex);
  rc=0;
  return rc;
 free_filepath_out:
  g_free(new_path);
 free_filename_out:
  g_free(filename);

  return rc;
}
/* public */
const gchar *
get_file_type_name(unsigned long fattr) {
	const gchar *name = str_ipmsg_fattr[IPMSG_UNKNWON_FATTR];

	if ( (fattr<IPMSG_FILE_REGULAR) || (fattr>IPMSG_FILE_RESFORK) )
		goto error_out;

	name = _(str_ipmsg_fattr[(int)fattr]);

error_out:  
	return name;
}

int 
get_file_info(const gchar *path, off_t *size, time_t *mtime, ipmsg_ftype_t *ipmsg_type) {
	int        rc = 0;
	int        fd = 0;
	ipmsg_ftype_t     ftype = 0;
	off_t   fsize = 0;
	time_t fmtime = 0;
	struct stat stat_buf;
  
	if ( (path == NULL) || (size == NULL) || 
	    (mtime == NULL) || (ipmsg_type == NULL) ) {
		rc = -EINVAL;
		goto error_out;
	}

	/*
	 * ファイル属性を取得するためにopenする。
	 */
	fd = open(path, O_RDONLY);
	if (fd < 0) {
		rc = -errno;
		goto error_out;
	}

	/*
	 * ファイル属性を獲得する.
	 */
	rc = fstat(fd, &stat_buf);
	if (rc < 0) {
		rc = -errno;
		goto close_out;
	}

	/*
	 * 最終更新時刻/サイズを獲得する
	 */
	fmtime = stat_buf.st_mtime;
	fsize = stat_buf.st_size;

	/*
	 * ファイル種別からIPMSGのファイル属性を設定する
	 */
	if ( S_ISREG(stat_buf.st_mode) ) 
		ftype = IPMSG_FILE_REGULAR; /* 通常ファイル */
	else if ( S_ISDIR(stat_buf.st_mode) ) {
		ftype = IPMSG_FILE_DIR;     /* ディレクトリ */
		fsize = 0; /* IPMSGでは, ディレクトリサイズは0として扱う */
	} else if ( S_ISCHR(stat_buf.st_mode) )
		ftype = IPMSG_FILE_CDEV; /* キャラクタ型デバイス  */
	else if ( S_ISFIFO(stat_buf.st_mode) )
		ftype = IPMSG_FILE_FIFO; /* FIFO(名前付きパイプ) */
	else
		ftype = 0;

	/*
	 * シンボリックリンクを検出するためにlstatを実行
	 */
	rc = lstat(path, &stat_buf);
	if (rc < 0) {
		rc = -errno;
		goto close_out;
	}


	if ( S_ISLNK(stat_buf.st_mode) )
		ftype = IPMSG_FILE_SYMLINK;

	/*
	 * 返却
	 */
	*size = fsize;
	*mtime = fmtime;
	*ipmsg_type = ftype;

close_out:
	close(fd);	
error_out:  
	return rc;
}

int 
create_attach_file_block(attach_file_block_t **afcb){
  int rc;
  attach_file_block_t *new_block;

  if (!afcb)
    return -EINVAL;

  rc=-ENOMEM;
  new_block=g_slice_new(attach_file_block_t);
  if (!new_block)
    return rc;

  memset(new_block,0,sizeof(attach_file_block_t));

  new_block->mutex=g_mutex_new();
  if (!(new_block->mutex))
    goto free_block_out;
  new_block->count=0;
  *afcb=new_block;
  
  return 0;

 free_block_out:
  dbg_out("Free: %x\n",(unsigned int)new_block);
  g_slice_free(attach_file_block_t,new_block);
  
  return rc;
}
int 
set_attch_file_block_pktno(attach_file_block_t *afcb,const pktno_t pktno){
  if (!afcb)
    return -EINVAL;

  if (!g_mutex_trylock(afcb->mutex)) {
    return -EBUSY;
  }
  g_mutex_unlock(afcb->mutex);

  return 0;
}
int 
destroy_attach_file_block(attach_file_block_t **afcb){
  int rc;
  attach_file_block_t *removed_block;
  GList *entry,*file_list;

  if ( (!afcb) || (!(*afcb)) )
    return -EINVAL;

  removed_block=*afcb;

  if (!g_mutex_trylock(removed_block->mutex))
    return -EBUSY;

  if (removed_block->ipaddr) {
    g_free(removed_block->ipaddr);
    removed_block->ipaddr=NULL;
  }
  /* files */
  file_list=removed_block->files;
  for(entry=g_list_first(file_list);entry;entry=g_list_next(entry)) {
    if (entry->data) {
      file_info_t *file_info;

      file_info=entry->data;
      rc=destroy_file_info(file_info);
      g_assert(rc==0);
      entry->data=NULL;
    }
  }
  g_list_free(file_list);
  g_mutex_unlock(removed_block->mutex);
  /* mutex */
  g_mutex_free (removed_block->mutex);
  dbg_out("Destroy :%x\n",(unsigned int)removed_block);
  dbg_out("Free: %x\n",(unsigned int)removed_block);
  g_slice_free(attach_file_block_t,removed_block);
  *afcb=NULL;

  return 0;
}
int 
add_attach_file(attach_file_block_t *afcb,const gchar *path){
  int rc;
  int new_id;
  file_info_t *new_info;

  dbg_out("here\n");
  if ( (!afcb) || (!path) )
    return -EINVAL;

  rc=create_file_info(&new_info);
  if (rc<0)
    return rc;

  new_id=afcb->max_id;
  rc=setup_file_info(new_id,new_info,path);
  if (rc<0)
    goto destroy_new_item_out;

  rc=-EBUSY;
  if (!g_mutex_trylock(afcb->mutex)) {
    goto destroy_new_item_out;
  }

  rc=-EEXIST;
  if (g_list_find_custom(afcb->files,new_info,check_duplicate_attach)) {
    g_mutex_unlock(afcb->mutex);
    dbg_out("Already exist:%s\n",path);
    goto destroy_new_item_out;
  }
  afcb->files=g_list_append(afcb->files,new_info);
  ++afcb->max_id;
  new_info->main_info_ref=afcb;
  g_mutex_unlock(afcb->mutex);
  dbg_out("new state:\n");
  show_file_list(afcb);
  return 0;
 destroy_new_item_out:
  destroy_file_info(new_info);
  return rc;
}
int 
remove_attach_file(attach_file_block_t *afcb,const gchar *path){
  file_info_t check_info;
  GList *node;

  if ( (!afcb) || (!path) )
    return -EINVAL;

  memset(&check_info,0,sizeof(file_info_t));
  check_info.filepath=(gchar *)path;
  node=g_list_find_custom(afcb->files,&check_info,check_duplicate_attach);
  if (!node) {
    dbg_out("No such file:%s\n",path);
    return -ENOENT;
  }

  if (!g_mutex_trylock(afcb->mutex)) {
    return -EBUSY;
  }
  destroy_file_info(node->data);
  afcb->files=g_list_remove(afcb->files,node);
  g_mutex_unlock(afcb->mutex);

  return 0;
}
static int
create_one_file_string(file_info_t *info,const gchar **string){
  int rc;
  char buff[FILE_ATTCH_ONE_EXT_SIZE];
  attach_file_block_t *afcb;
  char *new_string;
  gchar *ext_file_encoding;

  if ( (!info) || (!string) )
    return -EINVAL;

  if (!g_mutex_trylock(info->mutex)) {
    return -EBUSY;
  }

  afcb=info->main_info_ref;
  rc=-EINVAL;
  if (!afcb) 
    goto error_out;

  rc=-ENOMEM;
  ext_file_encoding=NULL;
  memset(buff,0,FILE_ATTCH_ONE_EXT_SIZE);
  snprintf(buff,FILE_ATTCH_ONE_EXT_SIZE-2,"%d:%s:%x:%x:%x:%c",
	   info->fileid,
	   info->filename,
	   (unsigned int)info->size,
	   (unsigned int)info->m_time,
	   (unsigned int)info->ipmsg_fattr,FILELIST_SEPARATOR);
  buff[FILE_ATTCH_ONE_EXT_SIZE-1]='\0';
  dbg_out("attach-file:%s=%s\n",info->filepath,buff);
  new_string=g_strdup(buff);
  if (ext_file_encoding != NULL)
    g_free(ext_file_encoding);

  if (!new_string) 
    goto error_out; /* -ENOMEM */

  *string=new_string;
  rc=0;
 error_out:
  g_mutex_unlock(info->mutex);
  return rc;
}
int 
get_attach_file_extention(attach_file_block_t *afcb,const gchar **ext_string){
  int rc;
  gchar *all_files=NULL;
  size_t len,total_len,buff_len;
  file_info_t *info;
  GList *file_list,*entry;

  if ( (!afcb) || (!ext_string) )
    return -EINVAL;

  all_files=g_malloc(FATTACH_BUFF_LEN);
  if (!all_files)
    return -ENOMEM;
  memset(all_files,0,FATTACH_BUFF_LEN);
  buff_len=FATTACH_BUFF_LEN;
  total_len=0;
  file_list=afcb->files;
  for(entry=g_list_first(file_list);entry;entry=g_list_next(entry)) {
    gchar *string;
    info=entry->data;
    rc=create_one_file_string(info,(const gchar **)&string);
    if (rc<0)
      goto free_all_files;
    len=strlen(string);
    while(buff_len <= (total_len+len)) { /* null終端を考慮して=を入れている */

      if (internal_extend_memory(&all_files,(buff_len + FATTACH_BUFF_LEN),buff_len,TRUE)){
	rc=-ENOMEM;
	g_free(string);
	goto free_all_files;
      }
      buff_len += FATTACH_BUFF_LEN;
    }
    strncat(all_files,string,len);
    dbg_out("New string state:%s(addr:0x%x)\n",
	    all_files,
	    (unsigned int)all_files);
    g_free(string);
    total_len+=len;
    all_files[total_len]='\0';
  }
  len=strlen(all_files);
  while ( (len>1) && (all_files[len-1] == FILELIST_SEPARATOR) ) {
    all_files[len-1]='\0';
    --len;
  }
  *ext_string=all_files;
  dbg_out("attach-file-ext:%s\n",*ext_string);
  return 0;

 free_all_files:
  if (all_files)
    g_free(all_files);
  return rc;
}
static gint
find_attach_file_block(gconstpointer a,gconstpointer b){
  attach_file_block_t *blk_a,*blk_b;

  if ( (!a) || (!b) )
    return -EINVAL;

  blk_a=(attach_file_block_t *)a;
  blk_b=(attach_file_block_t *)b;
  
  return (!(blk_a->pkt_no == blk_b->pkt_no));
}
int
ref_attach_file_block(pktno_t pktno,const char *ipaddr) {
  GList *node;
  attach_file_block_t chk_blk;
  attach_file_block_t *updated_afcb;
  int rc;

  if ( (!pktno) || (!ipaddr) )
    return -EINVAL;

  chk_blk.pkt_no=pktno;
  
  g_static_mutex_lock(&upload_queue_mutex);
  rc=-ENOENT;
  node=g_list_find_custom(uploads,&chk_blk,find_attach_file_block);

  if (!node)
    goto unlock_out;

  g_assert(node->data);
  updated_afcb=node->data;
  dbg_out("Ref afcb:%ld %s\n",pktno,ipaddr);
  if (!(updated_afcb->ipaddr))
    updated_afcb->ipaddr=g_strdup(ipaddr);

  ++(updated_afcb->count);
  dbg_out("ref update count:pktno=%ld current count:%d\n",pktno,updated_afcb->count);
  rc=0;
 unlock_out:
  g_static_mutex_unlock(&upload_queue_mutex);
  if (!rc)
    download_monitor_update_state();

  return rc;
}
int
unref_attach_file_block(pktno_t pktno) {
  GList *node;
  attach_file_block_t chk_blk;
  attach_file_block_t *updated_afcb;
  int rc;

  chk_blk.pkt_no=pktno;
  
  g_static_mutex_lock(&upload_queue_mutex);
  rc=-ENOENT;
  node=g_list_find_custom(uploads,&chk_blk,find_attach_file_block);

  if (!node)
    goto unlock_out;

  g_assert(node->data);
  updated_afcb=node->data;
  /*  強制的なカウントの減算を行う場合は, どこかでカウンタを
   *  インクリメントしているはず
   */
  g_assert (updated_afcb->count>0); 
  --(updated_afcb->count);

  dbg_out("unref update count:pktno=%ld current count:%d\n",pktno,updated_afcb->count);

 unlock_out:
  g_static_mutex_unlock(&upload_queue_mutex);

  return rc;
}
void
show_file_list(attach_file_block_t *afcb) {
  GList *file_list,*node;
  char   time_buff[27];

  g_assert(afcb);

  g_mutex_lock(afcb->mutex);
  file_list=afcb->files;
  for(node=g_list_first (file_list);node;node=g_list_next(node)) {
    file_info_t *info;
    if (node) {
      info=node->data;
      if (info) {
	g_mutex_lock(info->mutex);
	dbg_out("\t owner:0x%x id:%d type:%d path:%s filename:%s size:%lld(0x%llx) mtime:%s(0x%x)\n",
		(unsigned int)info->main_info_ref,
		info->fileid,
		info->ipmsg_fattr,
		info->filepath,
		info->filename,
		info->size,
		info->size,
		ctime_r(&(info->m_time), time_buff),
		(unsigned int)info->m_time);
	g_mutex_unlock(info->mutex);
      }
    }
  }  
  g_mutex_unlock(afcb->mutex);
}
void
show_upload_queue(void) {
  GList *node;
  int count=0;
  attach_file_block_t *blk;

  g_static_mutex_lock(&upload_queue_mutex);
  for(node=g_list_first (uploads);node;node=g_list_next(node),++count) {
    blk=node->data;
    if (blk) {
      g_mutex_lock(blk->mutex);
      dbg_out("%d: pktno:%ld\n",count,blk->pkt_no);
      g_mutex_unlock(blk->mutex);
    } else {
      g_assert_not_reached();
    }
  }
  g_static_mutex_unlock(&upload_queue_mutex);
}
int
add_upload_queue(pktno_t pktno,attach_file_block_t *afcb) {
  if (!afcb)
    return -EINVAL;

  afcb->pkt_no=pktno;
  g_static_mutex_lock(&upload_queue_mutex);
  uploads=g_list_append(uploads,afcb);
  g_static_mutex_unlock(&upload_queue_mutex);

  show_upload_queue();
  return 0;
}
int
remove_link_from_upload_queue(pktno_t pktno,GList **r_node) {
  GList *node;
  attach_file_block_t chk_blk;
  int rc;

  if (!r_node)
    return -EINVAL;

  chk_blk.pkt_no=pktno;
  
  g_static_mutex_lock(&upload_queue_mutex);
  rc=-ENOENT;
  node=g_list_find_custom(uploads,&chk_blk,find_attach_file_block);

  if (!node)
    goto unlock_out;

  uploads=g_list_remove_link(uploads,node);
  *r_node=node;

 unlock_out:
  g_static_mutex_unlock(&upload_queue_mutex);

  show_upload_queue();

  return rc;
}
static gint
find_attach_file_info(gconstpointer a,gconstpointer b){
  file_info_t *info_a,*info_b;

  if ( (!a) || (!b) )
    return -EINVAL;

  info_a=(file_info_t *)a;
  info_b=(file_info_t *)b;
  
  return (!(info_a->fileid == info_b->fileid));
}

int 
release_attach_file_block(const pktno_t pktno,gboolean force){
  GList *node;
  attach_file_block_t chk_blk;
  attach_file_block_t *afcb;
  int rc=-ENOENT;

  chk_blk.pkt_no=pktno;
  
  g_static_mutex_lock(&upload_queue_mutex);
  rc=-ENOENT;
  node=g_list_find_custom(uploads,&chk_blk,find_attach_file_block);

  if (!node)
    goto unlock_out;
  
  afcb=(attach_file_block_t *)node->data;
  g_assert(afcb);

  g_mutex_lock(afcb->mutex);
  if (afcb->count>0) 
    --afcb->count; /*  1度も送信されていなければ, 0になりうるため判定後にデクリメント  */

  if (force)  /* 強制開放  */
    afcb->count=0;    

  if (afcb->count>0) {
    dbg_out("Attach file still alive:count=%d\n",afcb->count);
    g_mutex_unlock(afcb->mutex);
    goto unlock_out;
  }
  g_mutex_unlock(afcb->mutex);

  uploads=g_list_remove_link(uploads,node);
  g_list_free_1(node);

  rc=destroy_attach_file_block(&afcb);

 unlock_out:
  g_static_mutex_unlock(&upload_queue_mutex);

  return rc;
}
/*
 *download_monitor_release_attach_file経由以外で呼び出してはならない.
 */
int
release_attach_file(const pktno_t pktno,int fileid){
  GList *node;
  GList *fnode,*flist;
  attach_file_block_t chk_blk;
  attach_file_block_t *afcb;
  file_info_t chk_info;
  file_info_t *finfo;
  int rc=-ENOENT;
  int remains=-ENOENT;

  chk_blk.pkt_no=pktno;
  
  g_static_mutex_lock(&upload_queue_mutex);
  rc=-ENOENT;
  node=g_list_find_custom(uploads,&chk_blk,find_attach_file_block);

  if (!node)
    goto unlock_out;
  afcb=node->data;
  g_assert(afcb);

  g_mutex_lock(afcb->mutex);
  flist=afcb->files;
  chk_info.fileid=fileid;

  fnode=g_list_find_custom(flist,&chk_info,find_attach_file_info);
  if (!fnode)
    goto afcb_unlock_out;

  finfo=fnode->data;

  g_mutex_lock(finfo->mutex);
  g_assert(finfo);
  g_assert(finfo->filepath);

  dbg_out("fileinfo found: path=%s size=%d(%x)\n",
	  finfo->filepath,
	  finfo->size,
	  finfo->size);
  afcb->files=g_list_remove(afcb->files,finfo);

  g_mutex_unlock(finfo->mutex);

  destroy_file_info(finfo);

  remains=g_list_length(afcb->files);
  rc=0;
 afcb_unlock_out:
  g_mutex_unlock(afcb->mutex);
 unlock_out:
  g_static_mutex_unlock(&upload_queue_mutex);

  if (remains==0)
    rc=release_attach_file_block(pktno,FALSE);

  show_upload_queue();

  return rc;
}
int 
refer_attach_file(const pktno_t pktno,int fileid,unsigned long *ipmsg_fattr,const char **path, off_t *size){
  GList *node;
  GList *fnode,*flist;
  attach_file_block_t chk_blk;
  attach_file_block_t *afcb;
  file_info_t chk_info;
  file_info_t *finfo;
  char *fpath;
  int rc=-ENOENT;

  if ( (!path) || (!size) )
    return -EINVAL;

  chk_blk.pkt_no=pktno;
  
  g_static_mutex_lock(&upload_queue_mutex);
  rc=-ENOENT;
  node=g_list_find_custom(uploads,&chk_blk,find_attach_file_block);

  if (!node)
    goto unlock_out;
  afcb=node->data;
  g_assert(afcb);

  g_mutex_lock(afcb->mutex);
  flist=afcb->files;
  chk_info.fileid=fileid;

  fnode=g_list_find_custom(flist,&chk_info,find_attach_file_info);
  if (!fnode)
    goto afcb_unlock_out;

  finfo=fnode->data;
  g_mutex_lock(finfo->mutex);
  g_assert(finfo);
  g_assert(finfo->filepath);

  dbg_out("fileinfo found: pktno=%d id:%d type=%d (%s) path=%s size=%lld(%llx)\n",
	  pktno,
	  fileid,
	  finfo->ipmsg_fattr,
	  get_file_type_name(finfo->ipmsg_fattr),
	  finfo->filepath,
	  finfo->size,
	  finfo->size);
  *ipmsg_fattr=finfo->ipmsg_fattr;
  fpath=g_strdup(finfo->filepath);
  rc=-ENOMEM;
  if (!fpath)
    goto finfo_unlock_out;

  *path=fpath;
  *size=finfo->size;

  rc=0;
 finfo_unlock_out:
  g_mutex_unlock(finfo->mutex);
 afcb_unlock_out:
  g_mutex_unlock(afcb->mutex);
 unlock_out:
  g_static_mutex_unlock(&upload_queue_mutex);

  show_upload_queue();

  return rc;
}

static int
destroy_download_file_info(download_file_block_t **ret_info) {
	download_file_block_t *remove_info = NULL;
  
	if ( (ret_info == NULL) || ( (*ret_info) == NULL ) )
		return -EINVAL;

	remove_info = *ret_info;

	if (remove_info->filename != NULL) {
		
		g_free(remove_info->filename);

		remove_info->filename=NULL;

	}

	dbg_out("Free: %x\n",(unsigned int)remove_info);

	g_slice_free(download_file_block_t, remove_info);

	return 0;
}
static int
init_download_file_info(download_file_block_t **ret_info) {
	download_file_block_t *new_info = NULL;
	int                          rc = 0;
  
	if (ret_info == NULL)
		return -EINVAL;

	rc = -ENOMEM;
	new_info = g_slice_new(download_file_block_t);

	if (new_info == NULL)
		return rc;
  
	memset(new_info, 0, sizeof(download_file_block_t));
	*ret_info = new_info;

	return 0;

 free_out:
	dbg_out("Free: %x\n", (unsigned int)new_info);

	g_slice_free(download_file_block_t, new_info);

	return rc;
}

static int 
get_one_download_file_info(const gchar *string,download_file_block_t **new_info_ref){
	char                         *sp = NULL;
	char                         *ep = NULL;
	char                     *buffer = NULL;
	download_file_block_t * new_info = NULL;
	int                           rc = 0;
	size_t                       len = 0;
	long                     int_val = 0;
	off_t                   size_val = 0;
	size_t                   remains = 0;
	char                   time_buff[27];

	if ( (string == NULL) || (new_info_ref == NULL) )
		return -EINVAL;

	rc = init_download_file_info(&new_info);
	if (rc < 0)
		return rc;

	rc = -ENOMEM;

	buffer = g_strdup(string);
	if (buffer == NULL)
		goto free_slice_out;

	len = strlen(string);
	remains = len;

	rc = -ENOENT;
	sp = buffer;

	/*
	 * file id
	 */
	ep = memchr(sp, ':', remains);
	rc = -EINVAL;
	if (ep == NULL) 
		goto free_out;

	*ep = '\0';
	errno = 0;
	int_val = strtol(sp, (char **)NULL, 10);
	g_assert(errno == 0);

	new_info->fileid = int_val;
	dbg_out("file id:%d(%x)\n", new_info->fileid, new_info->fileid);
	sp = ++ep;
	remains =len - ((unsigned long)ep - (unsigned long)buffer);

	/*
	 * file name
	 */
	ep = memchr(sp, ':', remains);
	rc = -EINVAL;
	if (ep == NULL) 
		goto free_out;

	*ep = '\0';

	new_info->filename = g_strdup(sp);
	rc = -ENOMEM;
	if (new_info->filename == NULL)
		goto free_out;

	dbg_out("file name:%s\n", new_info->filename);
	sp = ++ep;  

	/*
	 * size
	 */
	ep = memchr(sp, ':', remains);
	rc = -EINVAL;
	if (ep == NULL) 
		goto free_out;

	*ep = '\0';
	errno = 0;
	size_val = strtoll(sp, (char **)NULL, 16);
	g_assert(!errno);

	new_info->size = size_val;
	dbg_out("size:%d(%x)\n", new_info->size, new_info->size);
	sp = ++ep;
	remains =len - ((unsigned long)ep - (unsigned long)buffer);

	/*
	 * m_time
	 */
	ep = memchr(sp, ':', remains);
	rc = -EINVAL;
	if (ep == NULL) 
		goto free_out;

	*ep = '\0';

	errno = 0;
	int_val = strtol(sp, (char **)NULL, 16);
	g_assert(!errno);

	new_info->m_time = int_val;
	dbg_out("mtime:%s(%x)\n", ctime_r(&(new_info->m_time), time_buff),
	    (unsigned int)new_info->m_time);

	sp = ++ep;
	remains =len - ((unsigned long)ep - (unsigned long)buffer);

	/*
	 * type
	 */
	ep = memchr(sp, ':', remains);
	rc = -EINVAL;
	if (ep != NULL)  /*  拡張属性がある場合を考慮してこの判定は他とは逆になる */
		*ep = '\0';

	errno = 0;
	int_val=strtol(sp, (char **)NULL, 16);
	g_assert(!errno);

	new_info->ipmsg_fattr = int_val;
	dbg_out("type:%x\n", new_info->ipmsg_fattr);

	/*  拡張属性は無視する(FSによって使用できないことがあるので)  */

	*new_info_ref = new_info;

	if (buffer != NULL)
		g_free(buffer);
	
	return 0; /* 正常終了  */

 free_out:
	if (buffer != NULL)
		g_free(buffer);

 free_slice_out:
	dbg_out("Free: %x\n", (unsigned int)new_info);
	g_slice_free(download_file_block_t, new_info);

	return rc;
}

static int 
show_download_list(const GList *download_list) {
	GList                 *node = NULL;
	download_file_block_t *info = NULL;
	char              time_buff[27];

	if (download_list == NULL)
		return -EINVAL;

	for(node = g_list_first ((GList *)download_list); 
	    node != NULL; node = g_list_next(node) ) {

		info = (download_file_block_t *)(node->data);
		dbg_out("file-id:%x filename:%s size:%lld date:%s\n",
		    info->fileid,
		    info->filename,
		    info->size,
			ctime_r(&(info->m_time), time_buff));
	}

	return 0;
}

int 
destroy_download_list(const GList **download_list) {
	GList                 *node = NULL;
	download_file_block_t *info = NULL;
	GList          *remove_list = NULL;
	char              time_buff[27];

	if ( (download_list == NULL) || ( (*download_list) == NULL) )
		return -EINVAL;

	/*
	 * リスト内を探査し, データ参照ポインタの先にあるダウンロードファイル情報を開放する
	 */
	remove_list = (GList *)(*download_list);
	for(node = g_list_first(remove_list); node; node = g_list_next(node) ) {

		info = (download_file_block_t *)(node->data);

		dbg_out("remove file-id:%x filename:%s size:%lld date:%s\n",
		    info->fileid,
		    info->filename,
		    info->size,
		    ctime_r(&(info->m_time), time_buff));

		dbg_out("Free: %x\n", (unsigned int)info);
		g_slice_free(download_file_block_t, info);
	}
	
	/*
	 * リストを開放し, ポインタをNULLに設定しておく
	 */
	g_list_free(remove_list);
	*download_list = NULL;  

	return 0;
}

int
parse_download_string(const char *string, GList **list){
	char                        *sp = NULL;
	char                        *ep = NULL;
	char                    *buffer = NULL;
	download_file_block_t *new_info = NULL;
	GList                 *new_list = NULL;
	ssize_t                     len = 0;
	int                          rc = 0;

	if ( (string == NULL) || (list == NULL) ) {
		rc = -EINVAL;
		goto error_out;
	}

	dbg_out("attachment string:%s\n", string);

	/*
	 * 編集用に複製を作成
	 */
	buffer = g_strdup(string);
	if (buffer == NULL) {
		rc = -ENOMEM;
		goto error_out;
	}


	/*
	 * 1エントリの終端を探査
	 */
	sp = buffer;
	do {	
		/*
		 * 終端検出処理
		 * 1エントリの終端を探査
		 */
		len = strlen(sp);
		ep = memchr(sp, FILELIST_SEPARATOR, len);		

		/*
		 * 次の要素が存在する場合
		 */
		if (ep != NULL) {
			*ep = '\0'; /* 終端化  */
		}

		dbg_out("new string:%s\n", sp);

		/*
		 * 1エントリの内容を解析し, ダウンロード候補のリストを得る
		 */
		
		new_info = NULL;

		rc = get_one_download_file_info(sp, &new_info);

		if (rc != 0) {
			if (new_info != NULL)
				destroy_download_file_info(&new_info);
			continue;
		}

		new_list = g_list_append(new_list, new_info);

		/*
		 * 次の要素へ 
		 */
		if (ep != NULL) {
			sp = ++ep;     
		}
	} while (ep != NULL);

	/*
	 * 関数後処理
	 */
	show_download_list(new_list); /* 更新結果を表示  */

	*list = new_list; /* リスト返却 */
	
	rc = 0;   /* 正常終了 */
	
free_buffer_out:
	if (buffer != NULL)
		g_free(buffer);
error_out:
	return rc;
}

static int
get_filename_list(attach_file_block_t *afcb, gchar **string){
	size_t        len = 0;
	GList       *node = NULL;
	file_info_t *info = NULL;
	gchar *fnames = NULL;

	if ( (afcb == NULL) || (string == NULL) )
		return -EINVAL;

	len = 0;

	for(node=g_list_first(afcb->files); node; node = g_list_next(node)) {
		info = (file_info_t *)(node->data);

		g_assert(info);

		g_mutex_lock(info->mutex);

		if (info->filename != NULL) {
			/* null終端と空白があるので+2 */
			len += (strlen(info->filename) + 2); 
		}
		g_mutex_unlock(info->mutex);
	}

	/*
	 * ファイル名保存領域を獲得する.
	 */	
	fnames = g_malloc(len);
	if (fnames == NULL)
		return -ENOMEM;

	memset(fnames,0,len);
	for(node=g_list_first(afcb->files); node != NULL;
	    node=g_list_next(node) ) {
		
		info=(file_info_t *)(node->data);

		g_assert(info != NULL);

		g_mutex_lock(info->mutex);

		if (info->filename) {
			strncat(fnames, info->filename, len - 1);
			strncat(fnames, " ", len - 1);
		}

		g_mutex_unlock(info->mutex);
	}

	fnames[len-1] = '\0';

	*string = fnames;


	return 0;
}

static int
afcb_get_username(attach_file_block_t *afcb, gchar **string){
	int            rc = 0;
	gchar   *username = NULL;
	userdb_t   *entry = NULL;
	char         buff[256];

	if ( (afcb == NULL) || (string == NULL) )
		return -EINVAL;


	dbg_out("retrive:%s\n", afcb->ipaddr);
	userdb_search_user_by_addr(afcb->ipaddr, (const userdb_t **)&entry);

	if (entry != NULL) {
		dbg_out("get:%s %s\n", entry->nickname, entry->ipaddr);
		memset(buff, 0, 256);
		snprintf(buff, 256, "%s@%s(%s)", 
		    entry->nickname, entry->group, entry->ipaddr);

		buff[255]='\0';

		destroy_user_info(entry);
		username = g_strdup(buff);

		if (username != NULL)
			*string = username;
		else
			rc = -ENOMEM;
	}

	return rc;
}

void 
update_download_view(GtkWidget *window) {
  attach_file_block_t *afcb;
  GList *node;
  GtkWidget *view;
  GtkTreeModel *model;
  GtkTreeIter iter;
  attach_file_block_t *blk;

  dbg_out("here\n");

  g_static_mutex_lock(&upload_queue_mutex);

  g_assert(window);
  view=lookup_widget(GTK_WIDGET(window),"treeview5");
  g_assert(view);


  model = gtk_tree_view_get_model(GTK_TREE_VIEW(view));
  if (gtk_tree_model_get_iter_first(model,&iter)) {
    gtk_list_store_clear(GTK_LIST_STORE(model));
  }
  model = gtk_tree_view_get_model(GTK_TREE_VIEW(view));
  gtk_tree_model_get_iter_first(model,&iter);

  for(node=g_list_first (uploads);node;node=g_list_next(node)) {
    blk=node->data;

    if (blk){
      char *files=NULL;
      char *name=NULL;
      int count=0;

      g_mutex_lock(blk->mutex); 
      if (get_filename_list(blk, &files))
	goto unlock_blk;
      if (afcb_get_username(blk, &name))
      	goto free_files;

      count=g_list_length(blk->files);
      gtk_list_store_append(GTK_LIST_STORE(model), &iter);
      gtk_list_store_set(GTK_LIST_STORE(model), &iter,
			 DOWNLOAD_VIEW_FNAME,files,
			 DOWNLOAD_VIEW_REMAIN,count,
			 DOWNLOAD_VIEW_USER,name, 
			 DOWNLOAD_VIEW_PKTNO,blk->pkt_no,
			 -1);
      if (name)
	g_free(name);
    free_files:      
      if (files)
	g_free(files);
    unlock_blk:
      g_mutex_unlock(blk->mutex);
    }
  } 
  g_static_mutex_unlock(&upload_queue_mutex);
}

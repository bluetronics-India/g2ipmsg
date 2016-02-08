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

/* static */
static int
release_dir_info_contents(dir_request_t *info){
  if (info->filename)
    g_free(info->filename);
  return 0;
}
static int
parse_dir_request(const char *message,dir_request_t *ret,size_t max_len){
  int rc;
  char *buff;
  char *token;
  long int_val;
  off_t size_val;
  char *sp=NULL;
  char *ep=NULL;
  char *basename;
  char *filename;
  ssize_t remains;

  if ( (!message) || (!ret) || (!max_len) )
    return -EINVAL;

  
  buff=g_malloc(max_len+1); /* 最終パケットは\0終端されていないことに注意 */
  if (!buff)
    return -ENOMEM;
  memset(ret,0,sizeof(dir_request_t)); /*  念のため */
  memcpy(buff,message,max_len);
  buff[max_len]='\0';
  dbg_out("buff(len:%d):%s\n",strlen(buff),buff);

  remains=max_len;

  /*書式: header-size:filename:file-size:fileattr:*/  
  /*
   * ヘッダサイズ
   */
  sp=buff;
  ep=memchr(sp, ':', remains);
  if (!ep) {
    rc=-EINVAL;
    goto error_out;
  }

  *ep='\0';
  remains =max_len - ((unsigned long)ep-(unsigned long)buff);
  if (remains<=0) {
    rc=-EINVAL;
    goto error_out;
  }
  ++ep;

  int_val=strtol(sp, (char **)NULL, 16);
  ret->header_size=int_val;
  dbg_out("header size:%d(%x)\n",ret->header_size,ret->header_size);
  sp=ep;

  /*
   * ファイル名
   */
  ep=memchr(sp, ':', remains);
  if (!ep) {
    rc=-EINVAL;
    goto error_out;
  }
  *ep='\0';
  remains =max_len - ((unsigned long)ep-(unsigned long)buff);
  if (remains<=0) {
    rc=-EINVAL;
    goto error_out;
  }
  ++ep;

  rc=-ENOMEM;
  basename=g_path_get_basename(sp);
  if  (!basename)
    goto error_out;
  rc=convert_string_internal(basename,(const gchar **)&filename);
  if (rc<0) {
    err_out("Can not convert this request:%s\n",sp);
    err_out("Can not convert file name:%s\n",basename);
    g_free(basename);
    goto error_out;
  }
    g_free(basename);
  ret->filename=filename;
  dbg_out("filename:%s\n",ret->filename);
  sp=ep;

  /*
   * ファイルサイズ
   */
  ep=memchr(sp, ':', remains);
  if (!ep) {
    rc=-EINVAL;
    goto error_out;
  }

  *ep='\0';
  remains =max_len - ((unsigned long)ep-(unsigned long)buff);
  if (remains<=0) {
    rc=-EINVAL;
    goto error_out;
  }
  ++ep;

  size_val = strtoll(sp, (char **)NULL, 16);
  ret->file_size = size_val;
  dbg_out("file size:%lld(%llx)\n", ret->file_size, ret->file_size);
  sp=ep;

  /*
   * ファイル種別
   */
  ep=memchr(sp, ':', remains);
  if (!ep) {
    rc=-EINVAL;
    goto error_out;
  }

  *ep='\0';
  remains =max_len - ((unsigned long)ep-(unsigned long)buff);
  if (remains<=0) {
    rc=-EINVAL;
    goto error_out;
  }
  ++ep;

  int_val=strtol(sp, (char **)NULL, 16);
  ret->ipmsg_fattr=int_val;
  dbg_out("ipmsg file attr:%d(%x)\n",ret->ipmsg_fattr,ret->ipmsg_fattr);
  sp=ep;
  if (download_base_cmd(ret->ipmsg_fattr) != IPMSG_FILE_RETPARENT) {
    rc=-EPERM;

    if (!strcmp(G_DIR_SEPARATOR_S,basename)) {
      g_free(basename);
      goto error_out;
    }

    if (!strcmp(basename,"..")){
      g_free(basename); /* 上位への移動は禁止 */
      goto error_out;
    }
    if (!strcmp(basename,".")){
      g_free(basename); /* コマンド以外でのカレントへの移動は禁止 */
      goto error_out;
    }
  }
  rc=0;

 error_out:
  g_free(buff);

  return rc;
}
static int
emulate_chdir(const char *current_dir,char *subdir,char **new_dir){
  int rc;
  char *buff;
  gchar *dir_uri;
  GnomeVFSResult result;

  if ( (!current_dir) || (!subdir) || (!new_dir) ) 
    return -EINVAL;

  if (!strcmp(subdir,".."))
    return -EINVAL; /* コマンド以外で上位には移動しない  */

  if (!strcmp(subdir,"."))
    return -EINVAL; /* コマンド以外でカレントには移動しない  */

  if (!g_path_is_absolute(current_dir))
    return -EINVAL;

  buff=g_build_filename(current_dir,subdir,NULL);
  if (!buff)
    return -ENOMEM;

  rc=-ENOMEM;
  dir_uri=gnome_vfs_get_uri_from_local_path(buff);
  if (!dir_uri) 
    goto error_out;

  result=gnome_vfs_make_directory(dir_uri,GNOME_VFS_PERM_USER_ALL);
  if ( (result != GNOME_VFS_OK) && (result != GNOME_VFS_ERROR_FILE_EXISTS) ) {
    err_out("Can not create directory:%s\n",gnome_vfs_result_to_string(result));
    rc=-result;
    goto error_out;
  }
  if ( (result == GNOME_VFS_ERROR_FILE_EXISTS) &&
       (download_overwrite_confirm_generic(buff)) ) {
    rc=-result;
    goto error_out;
  }

  *new_dir=buff;

  return 0;
 error_out:
  g_free(buff);
  return rc;
}

static int
create_parent_dir(const char *current_dir,char **parent){
  int rc;
  char *buff;
  char *new_dir;
  gchar *ep;

  if ( (!current_dir) || (!parent) )
    return -EINVAL;

  buff=g_strdup(current_dir);
  if (!buff)
    return -ENOMEM;

  rc=-ENOENT;
  ep=g_strrstr(buff,G_DIR_SEPARATOR_S);
  if ( (!ep) || (buff == ep) )
    goto error_out;
  *ep='\0';

  rc=-ENOMEM;
  new_dir=g_strdup(buff);
  if (!new_dir)
    goto error_out;

  *parent=new_dir;

  rc=0;
 error_out:
  g_free(buff);
  return rc;
}
static int
check_end_condition(const char *top_dir, const char *current_dir, gboolean *is_cont){
	size_t top_len = 0;
	size_t dir_len = 0;
  
	if ( (top_dir == NULL) || (current_dir == NULL) || (is_cont == NULL) )
		return -EINVAL;

	*is_cont = TRUE;
	top_len = strlen(top_dir);
	dir_len = strlen(current_dir);

	if (top_len >= dir_len)
		*is_cont = FALSE;

	return 0;
}
static int
update_file_display(unsigned long filetype,const char *filename, off_t size,GtkTreeModel *model,GtkTreeIter *iter){
	if ( (filename == NULL) || (model == NULL) || (iter == NULL) )
		return -EINVAL;

	if ( !GTK_IS_LIST_STORE (model) ) {
		war_out("Download window is destroyed.\n");
		return -EFAULT; /*  仮にEFAULTを返す  */
	}

	gtk_list_store_set(GTK_LIST_STORE(model), iter, 
	    DOWNLOAD_FILENAME_NUM, filename, 
	    DOWNLOAD_RECVSIZE_NUM, (int64_t)0,
	    DOWNLOAD_FILESIZE_NUM, (int64_t)size,
	    DOWNLOAD_PROGRESS_NUM, 0,
	    DOWNLOAD_FILEATTR_NUM, filetype,
	    DOWNLOAD_FILETYPE_NUM, get_file_type_name(filetype),
	    -1);

	ipmsg_update_ui();

	return 0;
}

int
do_download(GtkTreeModel *model,
	       GtkTreePath *path,
	       GtkTreeIter *iter,
	       gpointer data){
 
 	tcp_con_t                        con;
 	ipmsg_private_data_t           *priv = NULL;
 	GtkWidget                     *entry = NULL;
 	GtkWidget                    *window = NULL;
 	char                        *dirname = NULL;
 	char                       *filename = NULL;
 	char                       *basename = NULL;
 	GtkWidget                 *all_check = NULL;
 	ipmsg_recvmsg_private_t *sender_info = NULL;
 	char                    *req_message = NULL;
 	int                               rc = 0;
 	ipmsg_ftype_t                  ftype = 0;
 	long                          pkt_no = 0;
 	int                           fileid = 0;
 	off_t                       may_read = 0;
 	int                         is_retry = 0;
 	off_t                    total_write = 0;
 	gboolean              is_interactive = FALSE;
 	char filepath[PATH_MAX];
   
 	window = GTK_WIDGET(data);
 	if ( !GTK_IS_WINDOW(window) )
 		return -EFAULT; 
 	entry = lookup_widget(GTK_WIDGET(window), "DownLoadDirectoryEntry");
 	if ( !GTK_IS_ENTRY(entry) )
 		return -EFAULT; 
 	all_check = lookup_widget(GTK_WIDGET(window), "DownLoadAllCheckBtn");
 	if ( !GTK_IS_CHECK_BUTTON(all_check) )
 		return -EFAULT; 
 
 	priv = (ipmsg_private_data_t *)lookup_widget(GTK_WIDGET(window), 
 	    "senderInfo");
 	g_assert(priv != NULL);
 
 	sender_info = priv->data; 
 	g_assert(sender_info->ipaddr != NULL);
 
 	pkt_no = sender_info->pktno;
 
 	gtk_tree_model_get (model, iter, 
 	    DOWNLOAD_FILEID_NUM, &fileid,
 	    DOWNLOAD_FILENAME_NUM, &filename, 
 	    DOWNLOAD_FILESIZE_NUM, (int64_t *)&may_read, 
 	    DOWNLOAD_FILEATTR_NUM, &ftype, 
 	    -1);
 
 	basename = g_path_get_basename(filename);
 	g_free(filename);
 
 	if (basename == NULL) {
 		return -ENOMEM;
 	}
 
 	dirname = (char *)gtk_entry_get_text(GTK_ENTRY(entry));
 	snprintf(filepath, PATH_MAX - 1, "%s/%s", dirname, basename);
 	filepath[PATH_MAX - 1] = '\0';
 	g_free(basename);
 
 	dbg_out("download %d th element of %ld from %s into %s\n", 
 	    fileid, pkt_no, sender_info->ipaddr, filepath);
 
retry_this_file:
 	memset(&con, 0, sizeof(tcp_con_t));
 	rc = tcp_setup_client(hostinfo_get_ipmsg_system_addr_family(), 
 	    sender_info->ipaddr, hostinfo_refer_ipmsg_port(), &con);
 	if (rc < 0) {
 		rc = -errno;
 		goto error_out;
 	}
 
 	/* 
 	 * ロックアップ対策のためのタイムアウト設定
 	 */
 	rc = sock_recv_time_out(con.soc, TCP_CLIENT_TMOUT_MS); 
 	if (rc < 0) {
 		rc = -errno;
 		goto error_out;
 	}
 
 	/*  リクエスト送信  */
 	send_download_request(&con, sender_info->ipaddr, ftype, pkt_no, fileid);
 	is_interactive=
 		((gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(all_check)))?
 		    (FALSE) : (TRUE) );
 
 	switch(download_base_cmd(ftype)) {
 	case IPMSG_FILE_DIR:       /*  階層ディレクトリ受信  */
 		ipmsg_update_ui();
 		dbg_out("This is directory ignored.\n");
 		rc = do_download_directory(&con, dirname, model, iter);
 
 		if ( (rc == 0) && (is_interactive) )
 			rc = download_dir_open_operation(dirname);
 		break;
 
 	case IPMSG_FILE_REGULAR:   /*  通常ファイル受信  */
 		rc = do_download_regular_file(&con, filepath, 
 		    may_read, &total_write, model, iter);
 
 		dbg_out("Retry:rc=%d file:%s write:%lld "
 		    "expected read :%lld dir:%s\n",
 		    rc, filepath, total_write, may_read, dirname);
 
 		is_retry = post_download_operation(rc,
 		    filepath, total_write, may_read, dirname, is_interactive);
 		if ( (rc != 0) && (is_retry == -EAGAIN) ) {
 			/* 0リセット */
 			gtk_list_store_set(GTK_LIST_STORE(model), iter, 
 			    DOWNLOAD_RECVSIZE_NUM, (int64_t)0, -1);
 			ipmsg_update_ui();
 			close(con.soc);
 			goto retry_this_file;
 		}
 		break;
 	default:
 		err_out("Unknown file type:%d\n", ftype);
 		break;
 	}
 	close(con.soc); /* 端点のクローズは, 受信側で行うのがipmsgの仕様  */
 
 error_out:
 	return rc;
}
static void
progress_cell_data_func(GtkTreeViewColumn *col,
                    GtkCellRenderer   *renderer,
                    GtkTreeModel      *model,
                    GtkTreeIter       *iter,
                    gpointer           user_data)
{
	GtkTreeView *view = NULL;
	GtkEntry   *entry = NULL;
	GtkWidget *window = NULL;
	int            fd = 0;
	off_t        sent = 0;
	off_t        size = 0;
	gint      persent = 0;
	int     fattr_num = 0;
	gchar size_string[32];
	gchar path_string[PATH_MAX];
	gchar   *filename = NULL;
	gboolean is_exist = FALSE;

	g_assert(user_data);

	view = (GtkTreeView *)user_data;

	window = lookup_widget(GTK_WIDGET(view), "downloadWindow");
	g_assert(window != NULL);

	entry = GTK_ENTRY(lookup_widget(GTK_WIDGET(view), 
		"DownLoadDirectoryEntry"));
	g_assert(entry != NULL);

	gtk_tree_model_get(model, iter, 
	    DOWNLOAD_FILENAME_NUM, &filename,
	    DOWNLOAD_RECVSIZE_NUM, (int64_t *)&sent,
	    DOWNLOAD_FILESIZE_NUM, (int64_t *)&size,
	    DOWNLOAD_FILEATTR_NUM, &fattr_num,
	    -1);

	snprintf(path_string, PATH_MAX, "%s/%s", 
	    gtk_entry_get_text(GTK_ENTRY(entry)), filename);

	path_string[PATH_MAX - 1] = '\0';

	dbg_out("Check path:%s\n", path_string);


	/*
	 * ファイル種別毎のファイルサイズ調整処理を実施
	 */
	switch(fattr_num) {

	default:
	case IPMSG_FILE_REGULAR:
		break;

	case IPMSG_FILE_DIR:
		/* ディレクトリのダウンロードが完了したら
		 * 親ディレクトリへの復帰になるはずなので
		 * 0として扱う.
		 */   
		persent = 0;
		goto output_out; /* 準備完了, 出力処理へ移行  */
		break;

	case IPMSG_FILE_RETPARENT:
		/* ディレクトリの完了通知なので, 100%とみなす.
		 * 厳密にいえば手抜きだが, 
		 * 中途のディレクトリも同様にあつかう    
		 */   
		persent = 100;
		goto output_out; /* 準備完了, 出力処理へ移行  */
		break;
	}

	/*
	 * 通常ファイルの処理
	 */
	if (size > 0)
		persent = (gint)( (sent /(double)size) * 100);
	else { /* 0バイトファイル対策  */
		persent = 0;

		fd = open(path_string, O_RDONLY);
		if (fd > 0) {
			is_exist = TRUE;
			close(fd);
			persent = 100;
		}
	}

 output_out:
	if (filename != NULL)
		g_free(filename);

	/*
	 * パーセント値の補正
	 */
	if (persent < 0)
		persent = 0;
	if (persent > 100)
		persent = 100;

	/*
	 * 文字列表現の作成
	 */
	snprintf(size_string, 31, "%d %%", persent);
	size_string[31] = '\0';

	/*
	 * パーセンテージ表示部分, 表示内容の数値とその文字列表現を設定
	 */
	g_object_set(renderer, "value", persent, NULL);
	g_object_set(renderer, "text", size_string, NULL);
}

static int
setup_download_view(GtkTreeView *view,GList *dlist) {
  GList *node;
  GtkTreeModel *model;
  GtkListStore        *liststore;
  GtkTreeViewColumn   *col;
  GtkCellRenderer     *renderer;

  if ( (!view) || (!dlist) )
    return -EINVAL;

  liststore = gtk_list_store_new(7, 
				 G_TYPE_INT,
				 G_TYPE_STRING,
				 G_TYPE_INT64,
				 G_TYPE_INT64,
				 G_TYPE_ULONG,
				 G_TYPE_ULONG,
				 G_TYPE_STRING);
  g_assert(liststore);
  
  gtk_tree_view_set_model(GTK_TREE_VIEW(view), GTK_TREE_MODEL(liststore));
  g_object_unref(liststore); /* bind to view */


  /*
   * タイトル設定
   */
  /* --- Column #1 --- */

  col = gtk_tree_view_column_new();

  gtk_tree_view_column_set_title(col, _("FileID"));

  /* pack tree view column into tree view */
  gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);

  renderer = gtk_cell_renderer_text_new();

  /* pack cell renderer into tree view column */
  gtk_tree_view_column_pack_start(col, renderer, TRUE);

  /* connect 'text' property of the cell renderer to
   *  model column that contains the id */
  gtk_tree_view_column_add_attribute(col, renderer, "text", DOWNLOAD_FILEID_NUM);

  gtk_tree_view_column_set_resizable (col,TRUE);

  /* --- Column #2 --- */

  col = gtk_tree_view_column_new();

  gtk_tree_view_column_set_title(col, _("FileType"));

  /* pack tree view column into tree view */
  gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);

  renderer = gtk_cell_renderer_text_new();

  /* pack cell renderer into tree view column */
  gtk_tree_view_column_pack_start(col, renderer, TRUE);

  /* connect 'text' property of the cell renderer to
   *  model column that contains the file type */
  gtk_tree_view_column_add_attribute(col, renderer, "text", DOWNLOAD_FILETYPE_NUM);

  gtk_tree_view_column_set_resizable (col,TRUE);

  /* --- Column #3 --- */

  col = gtk_tree_view_column_new();

  gtk_tree_view_column_set_title(col, _("FileName"));

  /* pack tree view column into tree view */
  gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);

  renderer = gtk_cell_renderer_text_new();

  /* pack cell renderer into tree view column */
  gtk_tree_view_column_pack_start(col, renderer, TRUE);

  /* connect 'text' property of the cell renderer to
   *  model column that contains the file name */
  gtk_tree_view_column_add_attribute(col, renderer, "text", DOWNLOAD_FILENAME_NUM);

  gtk_tree_view_column_set_resizable (col,TRUE);

  /* --- Column #4 --- */

  col = gtk_tree_view_column_new();

  gtk_tree_view_column_set_title(col, _("Recv Size"));

  /* pack tree view column into tree view */
  gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);

  renderer = gtk_cell_renderer_text_new();

  /* pack cell renderer into tree view column */
  gtk_tree_view_column_pack_start(col, renderer, TRUE);

  /* connect 'text' property of the cell renderer to
   *  model column that contains the recv size */
  gtk_tree_view_column_add_attribute(col, renderer, "text", DOWNLOAD_RECVSIZE_NUM);

  gtk_tree_view_column_set_resizable (col,TRUE);

  /* --- Column #5 --- */

  col = gtk_tree_view_column_new();

  gtk_tree_view_column_set_title(col, _("FileSize"));

  /* pack tree view column into tree view */
  gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);

  renderer = gtk_cell_renderer_text_new();

  /* pack cell renderer into tree view column */
  gtk_tree_view_column_pack_start(col, renderer, TRUE);

  /* connect 'text' property of the cell renderer to
   *  model column that contains the file size */
  gtk_tree_view_column_add_attribute(col, renderer, "text", DOWNLOAD_FILESIZE_NUM);

  gtk_tree_view_column_set_resizable (col,TRUE);

  /* --- Column #6 --- */

  col = gtk_tree_view_column_new();

  gtk_tree_view_column_set_title(col, _("Progress"));

  /* pack tree view column into tree view */
  gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);

  renderer = gtk_cell_renderer_progress_new ();

  /* pack cell renderer into tree view column */
  gtk_tree_view_column_pack_start(col, renderer, TRUE);

  /* connect 'text' property of the cell renderer to
   *  model column that contains the progress */
  gtk_tree_view_column_add_attribute(col, renderer, "text", DOWNLOAD_PROGRESS_NUM);

  gtk_tree_view_column_set_cell_data_func(col, renderer, progress_cell_data_func, GTK_TREE_VIEW(view), NULL);

  gtk_tree_view_column_set_resizable (col,TRUE);
  
  
  for(node=g_list_first (dlist);node;node=g_list_next(node)) {
      GtkTreeIter   newrow;
      download_file_block_t  *element = node->data;
      gchar *filename=NULL;
      const gchar *file_type_name = get_file_type_name(element->ipmsg_fattr);

      dbg_out("add view:%d,%s,%lld\n",element->fileid,element->filename,element->size);
      model = gtk_tree_view_get_model(GTK_TREE_VIEW(view));
      gtk_list_store_append(GTK_LIST_STORE(model), &newrow);

      if (!convert_string_internal(element->filename,(const gchar **)&filename)) {
	      gtk_list_store_set(GTK_LIST_STORE(model), &newrow, 
		  DOWNLOAD_FILEID_NUM, element->fileid, 
		  DOWNLOAD_FILENAME_NUM, filename, 
		  DOWNLOAD_RECVSIZE_NUM, (int64_t)0,
		  DOWNLOAD_FILESIZE_NUM, (int64_t)element->size,
		  DOWNLOAD_PROGRESS_NUM, 0,
		  DOWNLOAD_FILEATTR_NUM, element->ipmsg_fattr,
		  DOWNLOAD_FILETYPE_NUM, file_type_name,
			   -1);
	g_free(filename);
      }
  }
  gtk_tree_selection_set_mode(gtk_tree_view_get_selection(GTK_TREE_VIEW(view)),
			      GTK_SELECTION_MULTIPLE );  
  return 0;
}
/* end static */
/* begin public */
int 
internal_create_download_window(GtkWindow *recvWin,GtkWindow **window){
 	GtkWidget *new_window = NULL;
 	GtkWidget *view = NULL;
 	ipmsg_recvmsg_private_t *new_sender = NULL;
 	ipmsg_recvmsg_private_t *sender_info = NULL;
 	ipmsg_private_data_t *recv_priv = NULL;
 	ipmsg_private_data_t *new_priv = NULL;
 	GtkWidget *entry = NULL;
 	char cwd[PATH_MAX];
 	GList *downloads = NULL;


  if ( (!recvWin) && (!window) )
    return -EINVAL;

  dbg_out("here\n");

  if (getcwd(cwd,PATH_MAX)<0)
    cwd[0]='\0';
  else
    cwd[PATH_MAX-1]='\0';

  recv_priv=(ipmsg_private_data_t *)lookup_widget(GTK_WIDGET(recvWin),"senderInfo");
  g_assert(recv_priv);
  sender_info=recv_priv->data; 
  g_assert( (sender_info->ipaddr) && (sender_info->ext_part) );
  g_assert(!init_recvmsg_private(sender_info->ipaddr,sender_info->flags,sender_info->pktno,&new_priv));
  g_assert((new_priv) && (new_priv->data));
  new_sender=(ipmsg_recvmsg_private_t *)new_priv->data;
  dbg_out("new_priv:%x new_priv->ext_part:%x\n",
	  (unsigned int)new_priv,
	  (unsigned int)new_sender->ext_part);
  g_assert((new_sender) && (!(new_sender->ext_part)));
  new_sender->ext_part=g_strdup(sender_info->ext_part);
  if (!(new_sender->ext_part)) {
    destroy_ipmsg_private(new_priv);
    return -ENOMEM;
  }

  g_assert(!parse_download_string(sender_info->ext_part, &downloads));    
  if (!downloads)
    return -ENOENT;

  new_window=GTK_WIDGET(create_downloadWindow ());
  g_assert(new_window);

  dbg_out("Hook private:%x\n",(unsigned int)&new_priv);
  IPMSG_HOOKUP_DATA(new_window,new_priv,"senderInfo"); /* pktid用  */


  view=lookup_widget(GTK_WIDGET(new_window),"DownLoadFileTree");
  g_assert(view);
  setup_download_view(GTK_TREE_VIEW(view),downloads);

  entry=lookup_widget(new_window,"DownLoadDirectoryEntry");
  gtk_entry_set_text(GTK_ENTRY(entry),cwd); 

  *window=GTK_WINDOW(new_window);  
  return 0;
}
int
download_dir_open_operation(const char *dir){
  GtkWidget *dialog=create_downloadOpenDirOnlydialog();
  GtkWidget *dir_label;
  gint result;
  gchar *dir_url;
  gchar buffer[1024];
  int rc;

  if (!dir)
    return -EINVAL;

  dir_url=gnome_vfs_get_uri_from_local_path(dir);
  if (!dir_url)
    return -EINVAL;

  dir_label=lookup_widget(dialog,"downloadOpenDirNameLabel");
  g_assert(dir_label);
  snprintf(buffer,1023,"%s : %s",_("Directory"),dir);
  buffer[1023]='\0';
  gtk_label_set_text(GTK_LABEL(dir_label),buffer);

  result=gtk_dialog_run (GTK_DIALOG (dialog));
    
  switch (result) {
  case GTK_RESPONSE_OK:
    dbg_out("Accept for directory\n");
    break;
  case GTK_RESPONSE_CANCEL:
  default:
    dbg_out("response:%d cancel\n",result);
    goto error_out;
    break;
  }

  rc=gnome_vfs_url_show(dir_url);
  if ( rc != GNOME_VFS_OK)
    rc=-rc;
  else
    rc=0;
  
 error_out:
  gtk_widget_destroy (dialog);
  g_free(dir_url);

  return rc;
}
int
download_file_ok_operation(const char *file,const char *filepath, off_t size,const char *dir){
  GtkWidget *dialog=create_DownloadConfirmDialog();
  GtkWidget *file_label;
  GtkWidget *size_label;
  GtkWidget *app_label;
  GtkWidget *exec_btn;
  GtkWidget *dir_btn;
  char buff[1024];
  gint result;
  gchar *url;
  gchar *file_url;
  gchar *dir_url;
  gchar *utf8_fname;
  char *mime_type;
  GnomeVFSMimeApplication *app;
  int rc=-EINVAL;
  int exec_ok;

  if ( (!file) || (!filepath) || (!dir) )
    return -EINVAL;

  dbg_out("Post down load opetation filepath:%s dir:%s\n",filepath,dir);
  g_assert(dialog);
  file_label=GTK_WIDGET(lookup_widget(dialog,"DownLoadCompleteFileLabel"));
  size_label=GTK_WIDGET(lookup_widget(dialog,"DownLoadConfirmFileSizeLabel"));
  app_label=GTK_WIDGET(lookup_widget(dialog,"DownLoadConfirmAppLabel"));
  exec_btn=GTK_WIDGET(lookup_widget(dialog,"DownLoadConfirmExec"));
  dir_btn=GTK_WIDGET(lookup_widget(dialog,"DownLoadConfirmOpen"));
  g_assert(file_label);
  g_assert(size_label);
  g_assert(app_label);
  g_assert(exec_btn);
  g_assert(dir_btn);

  buff[1023]='\0';
  snprintf(buff,1023,"%s : %s",_("Downloaded File"),file);
  gtk_label_set_text(GTK_LABEL(file_label),buff);

  snprintf(buff,1023,"%s : %lld (%lld KB)",
      _("Downloaded Size"), size, (size/1024LL));
  gtk_label_set_text(GTK_LABEL(size_label),buff);


  file_url=gnome_vfs_get_uri_from_local_path(filepath);
  if (!file_url)
    return -EINVAL;

  dir_url=gnome_vfs_get_uri_from_local_path(dir);
  if (!dir_url)
    goto free_url_out;

  mime_type=gnome_vfs_get_mime_type(file_url);
  if (mime_type) {
    app=gnome_vfs_mime_get_default_application_for_uri(file_url,mime_type);
    if (app){
      snprintf(buff,1023,"%s : %s",_("Helper Application"),gnome_vfs_mime_application_get_name(app));
      gnome_vfs_mime_application_free(app);
    }else{
      snprintf(buff,1023,"%s : %s",_("Helper Application"),_("Undefined"));
      exec_ok=0;
      gtk_widget_set_sensitive(exec_btn, FALSE);
    }
    g_free(mime_type);
  }
  gtk_label_set_text(GTK_LABEL(app_label),buff);
  
  mime_type=gnome_vfs_get_mime_type(dir_url);
  if (mime_type) {

    app=gnome_vfs_mime_get_default_application_for_uri(dir_url,mime_type);
    if (!app)
      gtk_widget_set_sensitive(dir_btn, FALSE);

    gnome_vfs_mime_application_free(app);
    g_free(mime_type);
  }

  result=gtk_dialog_run (GTK_DIALOG (dialog));

  switch (result) {
  case GTK_RESPONSE_OK:
    dbg_out("Accept for directory\n");
    url=dir_url;
    break;
  case GTK_RESPONSE_ACCEPT:
    dbg_out("Accept for exec\n");
    url=file_url;
    break;
  case GTK_RESPONSE_CLOSE:
  default:
    dbg_out("response:%d close\n",result);
    gtk_widget_destroy (dialog);
    rc=-ENOENT;
    goto free_dir_url_out;
    break;
  }
  gtk_widget_destroy (dialog);

  rc=gnome_vfs_url_show(url);
  if ( rc != GNOME_VFS_OK)
    rc=-rc;
  else
    rc=0;

 free_dir_url_out:
  g_free(dir_url);
 free_url_out:
  g_free(file_url);
 no_free_out:
  return rc;
}
int
download_file_failed_operation(const char *filepath, const char *basename){
  GtkWidget *dialog=create_downLoadFailDialog ();
  GtkWidget *flabel;
  gint result;
  int rc;

  if (!dialog) {
    err_out("Can not create dialog\n");
    return -ENOMEM;
  }
  flabel=lookup_widget(dialog,"downLoadFailFileLabel");
  g_assert(flabel);
  if (basename)
    gtk_label_set_text(GTK_LABEL(flabel),basename);
  else
    gtk_label_set_text(GTK_LABEL(flabel),_("UnKnown"));

  rc=0;
  result=gtk_dialog_run (GTK_DIALOG (dialog));

  switch (result) {
  case GTK_RESPONSE_OK:
    dbg_out("response:%d ok\n",result);
    rc=-EAGAIN;
    break;
  case GTK_RESPONSE_CANCEL:
  default:
    dbg_out("response:%d cancel\n",result);
    if (filepath) {
    dbg_out("remove:%s\n",filepath);
      unlink(filepath);
    }
    break;
  }
  gtk_widget_destroy (dialog);

  return rc;
}
int
post_download_operation(int code, const char *filepath, off_t size, off_t all_size, const char *dir, gboolean exec){
	int     rc = 0;
	char *file = NULL;
  
	if ( (filepath == NULL) || (dir == NULL) )
		return -EINVAL;

	file = g_path_get_basename(filepath);
	if (file == NULL)
		return -ENOMEM;

	if ( (exec) && (code == 0) && (size == all_size) ) {
		rc = download_file_ok_operation(file, filepath, size, dir);
	}  else {
		if ( (code) && (code != -EFAULT) )
			rc = download_file_failed_operation(filepath, file);
	}

	g_free(file);

	return rc;
}
int
download_overwrite_confirm_generic(const char *filepath){
  GtkWidget *dialog=create_ipmsgDownloadOverWrite ();
  GtkWidget *label;
  gint result;

  if (!filepath)
    return -EINVAL;

  g_assert(dialog);
  label=GTK_WIDGET(lookup_widget(dialog,"overwriteFileNameLabel"));
  gtk_label_set_text(GTK_LABEL(label),filepath);

  result=gtk_dialog_run (GTK_DIALOG (dialog));

  switch (result) {
  case GTK_RESPONSE_OK:
    dbg_out("Accept for overwrite\n");
    break;
  default:
    return -ENOENT;
    break;
  }

  err_out("Can not open file:%s (%d)\n",strerror(errno),errno);
  gtk_widget_destroy (dialog);

  return 0;  
}
int
do_download_regular_file(tcp_con_t *con, const char *filepath, off_t file_size, off_t *written, GtkTreeModel *model, GtkTreeIter *iter){
	int             rc = 0;
	int             fd = 0;
	char         *buff = NULL;
	int       wait_max = TCP_DOWNLOAD_RETRY;
	ssize_t   recv_len = 0;
	ssize_t  write_len = 0;
	off_t  total_write = 0;
	off_t file_remains = 0;


	if ( (con == NULL) || (filepath == NULL) || (written == NULL) ||
	    (model == NULL) || (iter == NULL) )
		return -EINVAL;

	buff = g_malloc(_MSG_BUF_SIZE);
	if (buff == NULL)
		return -ENOMEM;

	fd = open(filepath, O_RDWR|O_EXCL|O_CREAT, S_IRWXU);
	if (fd < 0) {
		if (errno == EEXIST) {
			GtkWidget *dialog = create_ipmsgDownloadOverWrite();
			GtkWidget *label = NULL;
			gint result = 0;

			g_assert(dialog != NULL);
      label = GTK_WIDGET(lookup_widget(dialog, "overwriteFileNameLabel"));
      gtk_label_set_text(GTK_LABEL(label), filepath);

      result = gtk_dialog_run (GTK_DIALOG(dialog));
      switch (result) {
      case GTK_RESPONSE_OK:
	dbg_out("Accept for overwrite\n");
	fd = open(filepath, O_RDWR|O_CREAT, S_IRWXU);
	if (fd < 0) {
	  err_out("Can not open file:%s %s (%d)\n", 
	      filepath, strerror(errno), errno);
	  gtk_widget_destroy (dialog);
	  rc = -errno;
	  goto no_close_out;
	}
	break;
      default:
	if (errno)
	  err_out("response:%d Can not open file:%s (%d)\n",
	      result, strerror(errno), errno);
	else
	  err_out("Can not open file %s fd: %d :%s (%d)\n",
	      filepath, fd, strerror(errno), errno);

	gtk_widget_destroy(dialog);
	rc = -ENOENT;
	goto no_close_out;
	break;
      }
      err_out("Can not open file:%s (%d)\n", strerror(errno), errno);
      gtk_widget_destroy (dialog);
    } else{
      err_out("Can not open file:%s (%d)\n", strerror(errno), errno);
      rc = -errno;
      goto no_close_out;
    }
  }
 retry_this_file:
  lseek(fd, 0, SEEK_SET);
  total_write = 0;
  file_remains = file_size;

  dbg_out("Try to read %lld byte total\n", file_remains);
  /* 
   * 同期のためのselect
   */
  wait_max = TCP_DOWNLOAD_RETRY;
 wait_peer:
  ipmsg_update_ui();
  rc = wait_socket(con->soc, WAIT_FOR_READ, TCP_SELECT_SEC);

  if (rc < 0) {
	  --wait_max;
	  if (wait_max)
		  goto wait_peer;
	  err_out("Can not wait for peer %s (%d)\n", strerror(-rc), -rc);
	  goto file_close_out;
  }  

  /*
   *読み取り
   */
  wait_max = TCP_DOWNLOAD_RETRY;  
 read_wait:/*  同期のためにウエイト付きで読む  */
  memset(buff, 0, _MSG_BUF_SIZE);
  recv_len = (file_remains > _MSG_BUF_SIZE) ? (_MSG_BUF_SIZE) : (file_remains);
  recv_len = recv(con->soc, buff, recv_len, (MSG_PEEK) ); /*  仮読み  */
  if ( (wait_max > 0) && 
       ( (recv_len == 0) || (errno == EINTR) || (errno == EAGAIN) ) ) {
    ipmsg_update_ui();
    --wait_max;
    goto read_wait;
  }
  if (recv_len < 0) {
    err_out("Can not peek message %s(errno:%d)\n", strerror(errno), errno);
    rc = -errno;
    goto file_close_out; /* err or end */
  }

  recv_len = recv(con->soc, buff, recv_len, 0); /* 読み込み */
  while(recv_len >= 0){
    write_len=write(fd,buff,recv_len);
    if (write_len<0) {
      err_out("Can not write file %s(errno:%d)\n", strerror(errno), errno);
      rc = -errno;
      goto file_close_out; /* err or end */
    }
    total_write  += write_len;
    file_remains -= recv_len;
    if ( !GTK_IS_LIST_STORE (model) ) {
      war_out("Download window is destroyed.\n");
      rc = -EFAULT; /*  仮にEFAULTを返す  */
      goto file_close_out; /* window destory */
    }
    gtk_list_store_set(GTK_LIST_STORE(model), iter, 
		       DOWNLOAD_RECVSIZE_NUM, (int64_t)total_write,
		       -1);
    ipmsg_update_ui();
    dbg_out("tcp write file :%d\n", total_write);
    if (file_remains == 0){
      dbg_out("OK :%d\n", total_write);
      rc = 0;
      break;
    }
  retry_recv:
    wait_max = TCP_DOWNLOAD_RETRY;  
    memset(buff, 0, _MSG_BUF_SIZE);
    recv_len = 
	    (file_remains > _MSG_BUF_SIZE) ? (_MSG_BUF_SIZE) : (file_remains);
    recv_len = recv(con->soc, buff, recv_len, MSG_PEEK );
    if (recv_len <= 0) {
      if ( (wait_max > 0) && 
	   ( (recv_len == 0) || (errno == EINTR) || (errno == EAGAIN) ) ) {
	      ipmsg_update_ui();
	      --wait_max;
	      goto retry_recv;
      }
      err_out("Can not recv message %s(errno:%d)\n", strerror(errno), errno);
      rc = -errno;
      goto file_close_out; /* err or end */
   }
    recv_len = recv(con->soc, buff, recv_len, 0); /* 読み込み  */
  }

  /* 0バイトファイル対策のためもう一度書き込む  */
  gtk_list_store_set(GTK_LIST_STORE(model), iter, 
      DOWNLOAD_RECVSIZE_NUM, (int64_t)total_write, -1);
  ipmsg_update_ui();

  rc = 0;

 file_close_out:
  close(fd);

 no_close_out:
  *written = total_write;

  if (buff)
    g_free(buff);

  return rc;
}
int
do_download_directory(tcp_con_t *con, const char *top_dir,GtkTreeModel *model,GtkTreeIter *iter){
  int rc;
  int fd;
  char *buff=NULL;
  ssize_t recv_len;
  ssize_t drop_len;
  off_t total_write=0;
  int wait_max=TCP_DOWNLOAD_RETRY;
  dir_request_t dir_info;
  char *current_dir;
  char *new_dir=NULL;
  gchar *regular_file;
  off_t written;
  gboolean is_cont=TRUE;

  if ( (!con) || (!top_dir) || (!model) || (!iter) )
    return -EINVAL;
  if (!g_path_is_absolute(top_dir))
    return -EINVAL;

  buff=g_malloc(_MSG_BUF_SIZE);
  if (!buff)
    return -ENOMEM;

  current_dir=g_strdup(top_dir);
  if (!current_dir) {
    g_free(buff);
    return -ENOMEM;
  }

  do{
    /* 
     * 同期のためのselect
     */
    wait_max=TCP_DOWNLOAD_RETRY;
  wait_peer:
    ipmsg_update_ui();
    rc=wait_socket(con->soc,WAIT_FOR_READ,TCP_SELECT_SEC);
    if (rc<0) {
      --wait_max;
      if (wait_max)
	goto wait_peer;
      err_out("Can not wait for peer %s (%d)\n",strerror(-rc),-rc);
      goto error_out;
    }

    /*
     *読み取り
     */
    wait_max=TCP_DOWNLOAD_RETRY;  
  read_wait:/*  同期のためにウエイト付きで読む  */
    memset(buff,0,_MSG_BUF_SIZE);
    recv_len=recv(con->soc,buff,_MSG_BUF_SIZE,(MSG_PEEK)); /*  仮読み  */
    if ( (wait_max) && 
	 ( (!recv_len) || (errno == EINTR) || (errno == EAGAIN) ) ) {
      ipmsg_update_ui();
      --wait_max;
      goto read_wait;
    }
    if (recv_len<0) {
      err_out("Can not peek message %s(errno:%d)\n",strerror(errno),errno);
      rc=-errno;
      goto error_out; /* err or end */
    }
    /*
     *一度で読み取れなければ再トライ
     */
    memset(&dir_info,0,sizeof(dir_request_t));
    rc=parse_dir_request(buff,&dir_info,recv_len);
    if (rc) {
      release_dir_info_contents(&dir_info);
      --wait_max;
      goto read_wait;
    }
    if (!dir_info.header_size) {
      release_dir_info_contents(&dir_info);
      break;
    }
    drop_len=dir_info.header_size;
    drop_len=recv(con->soc,buff,drop_len,0); /* ヘッダ読み捨て */  
    if (drop_len<0) {
      err_out("Can not drop message %s(errno:%d)\n",strerror(errno),errno);
      rc=-errno;
      goto error_out; /* err or end */
    }
    dbg_out("OK Drop:%d\n",drop_len);
    switch(download_base_cmd(dir_info.ipmsg_fattr)){
    case IPMSG_FILE_DIR:
      rc=update_file_display(dir_info.ipmsg_fattr, dir_info.filename, dir_info.file_size, model, iter);
      if (rc<0) {
	release_dir_info_contents(&dir_info);
	goto error_out;
      }

      rc=emulate_chdir(current_dir,dir_info.filename,&new_dir);
      if (rc<0) {
	release_dir_info_contents(&dir_info);
	goto error_out;
      }
      g_free(current_dir);
      current_dir=new_dir;
      dbg_out("Chdir to:%s\n",current_dir);

      if (dir_info.file_size > 0) {
	drop_len=(dir_info.file_size<_MSG_BUF_SIZE)?(dir_info.file_size):(_MSG_BUF_SIZE);
	drop_len=recv(con->soc,buff,drop_len,0); /* 継続読み捨て */
	if (drop_len<0) {
	  err_out("Can not drop message %s(errno:%d)\n",strerror(errno),errno);
	  rc=-errno;
	  release_dir_info_contents(&dir_info);
	  goto error_out; /* err or end */
	}
      }
      break;
    case IPMSG_FILE_RETPARENT:
      dbg_out("Return to parent:cur=%s\n",current_dir);
      rc=update_file_display(dir_info.ipmsg_fattr,dir_info.filename,dir_info.file_size,model,iter);
      if (rc<0) {
	release_dir_info_contents(&dir_info);
	goto error_out;
      }

      create_parent_dir(current_dir,&new_dir);
      if (rc<0) {
	release_dir_info_contents(&dir_info);
	goto error_out;
      }
      g_free(current_dir);
      current_dir=new_dir;
      dbg_out("Chdir to:%s\n",current_dir);
      rc=check_end_condition(top_dir,current_dir,&is_cont);
      if (rc<0){
	release_dir_info_contents(&dir_info);
	goto error_out;
      }
      break;
    case IPMSG_FILE_REGULAR:
      rc=update_file_display(dir_info.ipmsg_fattr,dir_info.filename,dir_info.file_size,model,iter);
      if (rc<0) {
	release_dir_info_contents(&dir_info);
	goto error_out;
      }
      dbg_out("Download : %s/%s but drop at this time\n",current_dir,dir_info.filename);
      regular_file=g_build_filename(current_dir,dir_info.filename,NULL);      
      if (!regular_file) {
	release_dir_info_contents(&dir_info);
	goto error_out;
      }
      rc=do_download_regular_file(con, regular_file, dir_info.file_size, &written, model,iter);
      g_free(regular_file);
      if ( (rc<0) || (written != dir_info.file_size) ){
	release_dir_info_contents(&dir_info);
	goto error_out;
      }
      break;
    default:
      break;
    }
  }while(is_cont);

 error_out:
  if (current_dir)
    g_free(current_dir);
  if (buff)
    g_free(buff);
  return rc;
}

int
send_download_request(tcp_con_t *con, const char *ipaddr, ipmsg_ftype_t ftype,long pkt_no,int fileid){
  int rc;
  char *buff=NULL;
  char *req_message=NULL;
  ssize_t len;
  ssize_t soc_remains;
  ssize_t write_len;
  int wait_max=TCP_DOWNLOAD_RETRY;
  struct sigaction saved_act;

  buff=g_malloc(_MSG_BUF_SIZE);
  if (!buff)
    return -ENOMEM;

  memset(buff,0,_MSG_BUF_SIZE);
  snprintf(buff,_MSG_BUF_SIZE-1,
	   (ftype == IPMSG_FILE_DIR)?("%lx:%x"):("%lx:%x:0:"),
	   (long)pkt_no,(unsigned int)fileid);

  buff[_MSG_BUF_SIZE-1]='\0';

  dbg_out("request:%s\n",buff);

  rc = ipmsg_construct_getfile_message(con, ipaddr, ftype, buff, 0, (size_t *)&len, &req_message);
  if (rc<0) {
    goto free_buff_out;
  }
  dbg_out("request message:%s\n",req_message);

  wait_max=TCP_DOWNLOAD_RETRY;
 wrtite_wait_start:
  ipmsg_update_ui();
  rc=wait_socket(con->soc,WAIT_FOR_WRITE,TCP_SELECT_SEC);
  if (rc<0) {
    --wait_max;
    if (wait_max)
      goto wrtite_wait_start;
    err_out("Can not write for socket %s (%d)\n",strerror(-rc),-rc);
    goto error_out;
  }  

  soc_remains=len;
  while(soc_remains > 0) {
    disable_pipe_signal(&saved_act);
    write_len=send(con->soc, req_message,len, 0);
    enable_pipe_signal(&saved_act);
    if (write_len < 0) {
      err_out("Can not write socket :%s (%d)\n",strerror(errno),errno);
      rc=-errno;
      goto error_out;
    }
    soc_remains -= write_len;
    dbg_out("Write remains:%d\n",soc_remains);
  }

  rc=0;

 error_out:
  g_free(req_message);
 free_buff_out:
  if (buff)
    g_free(buff);
  return rc;
}
gboolean 
is_supported_file_type(unsigned long ipmsg_fattr){

  switch(download_base_cmd(ipmsg_fattr))
    {
    case IPMSG_FILE_REGULAR:
    case IPMSG_FILE_DIR:
      return TRUE;
      break;
    default:
      break;
    }
  return FALSE;	
}
/*  recv attachments */
void
recv_attachments(gpointer data) {
	GtkTreeSelection                *sel = NULL;
	ipmsg_private_data_t           *priv = NULL;
	GtkWidget                      *view = NULL;
	GList                          *node = NULL;
	GtkWidget                     *okbtn = NULL;
	GtkTreeModel                  *model = NULL;
	GList                        *pathes = NULL; 
	GtkButton                    *button = NULL;
	GtkWidget                    *window = NULL;
	GtkWidget                    *dirbtn = NULL;
	GtkWidget                 *all_check = NULL;
	GtkWidget                 *dir_entry = NULL;
	ipmsg_recvmsg_private_t *sender_info = NULL;
	int                               rc = 0;
	gboolean           noneed_to_destory = FALSE;
	GtkTreeIter                     iter;


	dbg_out("here:\n");

	if (data == NULL)
		return;
	
	/*
	 * Widgets relevant initilization.
	 */

	button = GTK_BUTTON(data);

	window = lookup_widget(GTK_WIDGET(button), "downloadWindow");
	g_assert(window != NULL);

	dirbtn = lookup_widget(GTK_WIDGET(window), "DownLoadOpenBtn");
	g_assert(dirbtn != NULL);

	okbtn = lookup_widget(GTK_WIDGET(window),"DownLoadOKBtn");
	g_assert(okbtn != NULL);

	all_check = lookup_widget(GTK_WIDGET(window), "DownLoadAllCheckBtn");
	g_assert(all_check != NULL);

	dir_entry=lookup_widget(GTK_WIDGET(window), "DownLoadDirectoryEntry");
	g_assert(dir_entry != NULL);

	priv = 
		(ipmsg_private_data_t *)lookup_widget(GTK_WIDGET(window), 
		    "senderInfo");

	g_assert(priv != NULL);

	sender_info = priv->data; 
	g_assert(sender_info->ipaddr != NULL);

	gtk_widget_set_sensitive(dirbtn, FALSE);
	gtk_widget_set_sensitive(okbtn, FALSE);
	gtk_widget_set_sensitive(all_check, FALSE);
	gtk_widget_set_sensitive(dir_entry, FALSE);

	view = lookup_widget(GTK_WIDGET(window), "DownLoadFileTree");
	g_assert(view != NULL);

	/*
	 * Receive items
	 */
	sel = gtk_tree_view_get_selection( GTK_TREE_VIEW(view) );
	pathes = gtk_tree_selection_get_selected_rows(sel, &model);

	for(node = g_list_first(pathes); 
	    node != NULL; 
	    node = g_list_next(node) ) {

		/*
		 * Scroll window according to download progression.
		 */
		g_assert( (node != NULL) && (node->data != NULL) );
		gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(view),
		    node->data,
		    NULL,
		    FALSE,
		    0.0,
		    0.0);
		/*
		 * Get the position of current download item, and
		 * download it
		 */
		gtk_tree_model_get_iter(model, &iter, node->data);
		rc = do_download(model, node->data, &iter, window);
		
		/*
		 * Check whether the window alive.
		 */
		if ( (rc < 0) && ( !GTK_IS_WINDOW(window) ) ) {
			/*  window destroyed, free the list & it's contents */
			g_list_foreach (pathes, 
			    (GFunc)gtk_tree_path_free, NULL);

			g_list_free (pathes);

			return;
		}
	}

	/* free the list & it's contents */
	g_list_foreach (pathes, (GFunc)gtk_tree_path_free, NULL);
	g_list_free(pathes);

	/*
	 * remove items on the window
	 */
	for(sel = gtk_tree_view_get_selection( GTK_TREE_VIEW(view) );
	    gtk_tree_selection_count_selected_rows (sel) > 0; ) {
      
		pathes = gtk_tree_selection_get_selected_rows(sel, &model);
		node = g_list_first(pathes);

		g_assert( (node != NULL) && (node->data != NULL) );

		gtk_tree_model_get_iter(model, &iter, node->data);
		gtk_list_store_remove( GTK_LIST_STORE(model), &iter);    
		g_list_foreach (pathes, (GFunc)gtk_tree_path_free, NULL);
		g_list_free(pathes);
	}

	/*
	 * Update button status
	 */
	if (gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON(all_check) ) )  
		download_dir_open_operation(
			gtk_entry_get_text( GTK_ENTRY(dir_entry) ) );
  
	gtk_widget_set_sensitive(dirbtn, TRUE);
	gtk_widget_set_sensitive(okbtn, TRUE);
	gtk_widget_set_sensitive(all_check, TRUE);
	gtk_widget_set_sensitive(dir_entry, TRUE);

	/*
	 * Destroy the window if no items remain.
	 */
	noneed_to_destory = gtk_tree_model_get_iter_first (model, &iter);
	if (noneed_to_destory)
		dbg_out("some element remain\n");
	else
		gtk_widget_destroy (window);
}

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
 * @brief  コールバック関数群
 * @author Takeharu KATO
 */ 

/** lookup_widget用オブジェクトリファレンスを設定する
 *  @param[in]  component  トップレベルウィジェット
 *  @param[in]  widget     オブジェクトリファレンス設定対照ウィジェット
 *  @param[in]  name       検索キー文字列
 */
#define GLADE_HOOKUP_OBJECT(component, widget, name) \
  g_object_set_data_full (G_OBJECT (component), name, \
    gtk_widget_ref(widget), (GDestroyNotify) gtk_widget_unref)

/** ドラッグ＆ドロップ用ターゲットエントリ
 */
static GtkTargetEntry targetentries[] =
    {
      { "text/uri-list", 0, 0 },
    };

/** 設定ウィンドウのグループ名エントリボックス内のインデクス値を獲得する
 *  @param[in]  combobox   グループ名エントリボックス
 *  @param[in]  groupname  グループ名
 *  @param[out] index      インデクス値返却領域のアドレス
 *  @retval     0          正常終了
 *  @retval    -ESRCH      指定されたグループが見つからなかった.
 */
static int
get_group_index(GtkComboBox *combobox, const gchar *groupname, int *index) {
	GtkTreeIter    iter;
	GtkTreeModel *model = NULL;
	gchar     *str_data = NULL;
	gboolean      valid = FALSE;
	int           count = 0;
	int              rc = 0;

	if ( (combobox == NULL) || (groupname == NULL) || (index == NULL) )
		return -EINVAL;

	model = gtk_combo_box_get_model(combobox);
	g_assert(model != NULL);

	/*
	 * エントリボックスを探査し、指定されたグループのインデクスを獲得する.
	 */
	valid = gtk_tree_model_get_iter_first (model, &iter);
	while(valid) {

		gtk_tree_model_get (model, &iter, 0, &str_data, -1);
		dbg_out("group:%s\n", str_data);

		if (!strcmp(groupname, str_data)) { /* グループが見つかった */
			
			*index = count;
			rc = 0;

			dbg_out("find:%s(%d)\n", str_data, *index);
			g_free(str_data);

			goto func_out;
		}
		/* 次のエントリを探査する  */
		if (str_data != NULL)
			g_free(str_data);

		valid = gtk_tree_model_iter_next (model, &iter);
		++count;
	}

	rc = -ESRCH; /* 指定されたグループが見つからなかった  */

func_out:
	return rc;
}

/** 設定ウィンドウのグループ名エントリボックスアクティブインデクスを取得する
 *  @param[in]  combobox   グループ名エントリボックス
 *  @return     アクティブインデクス
 *  @retval    -ESRCH      指定されたグループが見つからなかった.
 */
static int
get_current_group_index(GtkComboBox *combobox) {
	int               rc = 0;
	int            index = 0;
	gchar *current_group = NULL;

	g_assert(combobox != NULL);

	current_group = (gchar *)hostinfo_refer_group_name();
	rc = get_group_index(combobox,current_group,&index);

	return index;
}

/** 設定ウィンドウのグループ名エントリボックスに設定値を追加する
 *  @param[in]  comboEntry   グループ名エントリボックス
 *  @retval    0             正常終了
 *  @retval   -EINVAL        引数異常
 */
static int
updateConfigWindowGroups(GtkWidget *comboEntry) {
	GtkTreeModel *model = NULL;
	GtkTreePath   *path = NULL;
	gchar  *active_text = NULL;
	int              rc = 0;
	int     group_index = 0;
	GtkTreeIter    iter;

	model = gtk_combo_box_get_model(GTK_COMBO_BOX(comboEntry));
	if (model == NULL)
		return -EINVAL;

	path = gtk_tree_path_new_first();
	g_assert(path != NULL);
  
	/*
	 * 現在の格納値をクリアする
	 */
	if (gtk_tree_model_get_iter(model,&iter,path)) {
		while(gtk_list_store_remove(GTK_LIST_STORE(model), &iter))
			dbg_out("Remove group\n");
	}
	gtk_tree_path_free(path);   /* データ領域を開放する  */

	/*
	 * ホストリストの内容をエントリボックスに反映する  
	 */
	userdb_update_group_list(GTK_COMBO_BOX(comboEntry));

	/*
	 * エントリへの入力値を獲得する
	 */
	active_text =
		gtk_combo_box_get_active_text( GTK_COMBO_BOX(comboEntry) );

	/*
	 * 設定されているグループ名を追加する
	 */
	if ( (active_text != NULL) && (strlen(active_text) > 0 ) ) {
		rc = get_group_index(GTK_COMBO_BOX(comboEntry), 
		    active_text, &group_index);
		
		/*
		 * 新規エントリの場合は, 追加し, 既存の場合はそのエントリを
		 * アクティブエントリに設定する.
		 */
		if (rc < 0)
			gtk_combo_box_append_text(GTK_COMBO_BOX(comboEntry), 
			    active_text);
		else
			gtk_combo_box_set_active(GTK_COMBO_BOX(comboEntry), 
			    group_index);
	}
	
	if (active_text != NULL)
		g_free(active_text);


	return 0;
}

/** 添付ファイルの拡張部文字列を生成する
 *  @param[in]      pktno  パケット番号
 *  @param[in]     editor 添付ファイルエディタのトップウィジェット
 *  @param[out]  ext_part 添付ファイルの拡張部文字列を指すポインタのアドレス
 */
static void
sendmessage_get_attachment_part(pktno_t pktno, GtkWidget *editor, const char **ext_part){
	attach_file_block_t *afcb = NULL;
	GtkWidget           *view = NULL;
	GtkTreeModel       *model = NULL;
	gchar             *string = NULL;
	gchar           *filepath = NULL;
	gboolean            valid = FALSE;
	int                 count = 0;
	GtkTreeIter          iter;
  
	dbg_out("here\n");

	g_assert(ext_part != NULL);
	g_assert(*ext_part == NULL);

	if (create_attach_file_block(&afcb))
		return;

	view = lookup_widget(editor,"attachedFilesView");
	g_assert(view != NULL);

	model = gtk_tree_view_get_model(GTK_TREE_VIEW(view));
	g_assert(model != NULL);

	count = 0;

	valid = gtk_tree_model_get_iter_first(model, &iter);

	while(valid) {
		gtk_tree_model_get (model, &iter, 0, &filepath, -1);

		dbg_out("filepath:%s\n", filepath);
		add_attach_file(afcb, filepath);
		g_free(filepath);
		++count;
		valid = gtk_tree_model_iter_next (model, &iter);
	}

	if ( (count > 0) && 
	    (get_attach_file_extention(afcb,(const gchar **)&string) == 0) ) {
		add_upload_queue(pktno, afcb);
		dbg_out("Attach file string:%s\n", string);
		*ext_part = string;  
	}
}

/** 電文を送信する
 *  @param[in]      model  ホストリストのモデル 
 *  @param[in]       path  ホストリスト中の選択部分
 *  @param[in]       iter  選択部分を探査するイテレータ
 *  @param[in]     info_p  送信者情報
 */
static void
do_send(GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, 
    gpointer info_p) {
	send_info_t            *info = NULL;
	gchar                *ipaddr = NULL;
	GtkWidget *attachment_editor = NULL;
	gchar              *ext_part = NULL;
	int                       rc = 0 ;
	int                   pkt_no = 0;
	int                   lflags = 0;

	info = (send_info_t *)info_p;
	gtk_tree_model_get (model, iter, USER_VIEW_IPADDR_ID, &ipaddr, -1);

	pkt_no = ipmsg_get_pkt_no();
	lflags = info->flags;

	dbg_out("Send to %s Flags[%x] from ui\n", ipaddr, lflags);

	if ( lflags & IPMSG_FILEATTACHOPT) {
		/*
		 *アタッチメント
		 */
		attachment_editor = info->attachment_editor;
		if (attachment_editor != NULL) {
			dbg_out("This message has attachment\n");
			ext_part = NULL;
			sendmessage_get_attachment_part(pkt_no,
			    GTK_WIDGET(attachment_editor), 
			    (const char **)&ext_part);

			if (ext_part == NULL) {
				lflags &= ~IPMSG_FILEATTACHOPT;
			}
		}
	}
	/* FIXME:IPアドレスからアドレスファミリを取得して, udp
	 * コネクションを獲得するように修正
	 * ipmsg_send_send_msgには, udp_conは本来不要
	 */
	if ( lflags & IPMSG_FILEATTACHOPT) {
		rc = ipmsg_send_send_msg(udp_con, ipaddr, lflags, pkt_no, 
		    info->msg, ext_part);
		if (rc == 0) {
			ref_attach_file_block(pkt_no, ipaddr);
		}
	} else {
		rc = ipmsg_send_send_msg(udp_con, ipaddr, lflags, pkt_no, 
		    info->msg, NULL);
	}

	if (rc != 0)
		ipmsg_err_dialog(_("Can not send message to %s pktno=%d"), 
		    ipaddr, pkt_no);

	if (ext_part != NULL)
		g_free(ext_part);
	g_free(ipaddr);
}

static int
has_dupulicated_string_in_cell(const char *string,GtkTreeView *treeview){
  GtkTreeModel *model;
  GtkTreeIter iter;
  gboolean valid;
  int rc;

  g_assert(treeview);
  if (!string)
    return -EINVAL;

  rc=-ESRCH;
  model = gtk_tree_view_get_model(treeview);
  for(valid = gtk_tree_model_get_iter_first (model, &iter);
      valid;
      valid = gtk_tree_model_iter_next (model, &iter)) {
	gchar *str_data;
	gtk_tree_model_get (model, &iter, 
                          0, &str_data,
                          -1);
	if (!strcmp(string,str_data)) {
	  dbg_out("Already exist:%s\n",str_data);
	  g_free(str_data);
	  rc=0;
	  break;
	}
	g_free(str_data);
  }
  return rc;
}
static void
onBroadcastSelectionChanged (GtkTreeSelection *sel, GtkListStore *liststore)
{
  GtkTreeIter  selected_row;
  GtkWidget    *view;
  GtkWidget    *remove_btn;
  
  view=GTK_WIDGET(gtk_tree_selection_get_tree_view(sel));
  g_assert(view);
  remove_btn=lookup_widget(GTK_WIDGET(view),"configRemoveBcastBtn");
  g_assert(remove_btn);
  g_assert(gtk_tree_selection_get_mode(sel) == GTK_SELECTION_SINGLE);

  if (gtk_tree_selection_get_selected(sel, NULL, &selected_row))
    {
      gtk_widget_set_sensitive(remove_btn, TRUE);
    }
  else
    {
      gtk_widget_set_sensitive(remove_btn, FALSE);
    }
}

void
on_app1_drag_data_received             (GtkWidget       *widget,
                                        GdkDragContext  *drag_context,
                                        gint             x,
                                        gint             y,
                                        GtkSelectionData *data,
                                        guint            info,
                                        guint            time,
                                        gpointer         user_data)
{
  dbg_out("here\n");
}


gboolean
on_app1_drag_drop                      (GtkWidget       *widget,
                                        GdkDragContext  *drag_context,
                                        gint             x,
                                        gint             y,
                                        guint            time,
                                        gpointer         user_data)
{
  dbg_out("here\n");
  return FALSE;
}


gboolean
on_app1_destroy_event                  (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
  dbg_out("here\n");
  return FALSE;
}


void
on_app1_destroy                        (GtkObject       *object,
                                        gpointer         user_data)
{
  GtkWidget *window;
  GtkWidget *attachment_editor;

  dbg_out("here\n");

  window=lookup_widget(GTK_WIDGET(object),"messageWindow");
  g_assert(window);

  attachment_editor = (GtkWidget*) g_object_get_data (G_OBJECT (object),
						      "attach_win");
  if (attachment_editor) 
    gtk_widget_destroy (attachment_editor);
  else
    dbg_out("No attachment editor\n");

  userdb_remove_waiter_window(window);
}


void
on_new1_activate                       (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  dbg_out("here\n");
}


/* notice: This function may be called from applt, so we should not use
 * menuitem argument.
 */
void
on_preferences1_activate               (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  GtkWidget *configWindow;
  GtkTreeView *broadcasts;
  GtkCellRenderer    *renderer;
  GtkTreeViewColumn  *col;
  GtkTreeSelection   *sel;
  GtkListStore       *liststore;
  GtkComboBox        *combobox;

  dbg_out("here\n");
  configWindow=create_configWindow1 ();
  if (!configWindow) 
    return;

  broadcasts=GTK_TREE_VIEW(lookup_widget(configWindow,"treeview4"));
  g_assert(broadcasts);

  renderer = gtk_cell_renderer_text_new();
  if (!renderer)
    goto free_window;

  col = gtk_tree_view_column_new();
  if (!col)
    goto free_renderer;

  liststore = gtk_list_store_new(1, G_TYPE_STRING);
  if (!liststore)
    goto free_col;

  gtk_tree_view_set_model(GTK_TREE_VIEW(broadcasts), GTK_TREE_MODEL(liststore));
  g_object_unref(liststore);
  gtk_tree_view_column_pack_start(col, renderer, TRUE);
  gtk_tree_view_column_add_attribute(col, renderer, "text", 0);
  gtk_tree_view_column_set_title(col, _("Broad cast addresses"));

  gtk_tree_view_append_column(GTK_TREE_VIEW(broadcasts), col);

  sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(broadcasts));
  gtk_tree_selection_set_mode(sel, GTK_SELECTION_SINGLE);
  g_signal_connect(sel, "changed", 
		   G_CALLBACK(onBroadcastSelectionChanged),
		   liststore);

  combobox = GTK_COMBO_BOX(lookup_widget(configWindow, "configEncodingComboBox"));
  g_assert(combobox != NULL);
  setup_encoding_combobox(combobox);

  gtk_widget_show (configWindow);

  return;

 free_window:
  gtk_widget_destroy(GTK_WIDGET(configWindow));
 free_col:
  gtk_object_destroy(GTK_OBJECT(col));
 free_renderer:
  gtk_object_destroy(GTK_OBJECT(renderer));
  return;
}

void
on_update2_activate                    (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  dbg_out("here\n");
  ipmsg_update_ui_user_list();
}


void
on_about1_activate                     (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  ipmsg_show_about_dialog();
}


void
on_sendBtn_released                    (GtkButton       *button,
                                        gpointer         user_data)
{
  dbg_out("here\n");
}


void
on_sendBtn_pressed                     (GtkButton       *button,
                                        gpointer         user_data)
{
  dbg_out("here\n");
}

void
on_configOpenCheckChkBtn_toggled       (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
  dbg_out("here\n");
}


void
on_configNonPopupCheckBtn_toggled      (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
  dbg_out("here\n");
}


void
on_configNoSoundCheckBtn_toggled       (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
  dbg_out("here\n");
}


void
on_configEncloseEnableCheckBtn_toggled (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
  dbg_out("here\n");
}


void
on_configCitationCheckBtn_toggled      (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
  dbg_out("here\n");
}


void
on_configLoggingEnableCheckBtn_toggled (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
  dbg_out("here\n");
}


void
on_configUserEnableCheckBtn_toggled    (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
  dbg_out("here\n");
}


void
on_logFileDialogBtn_pressed            (GtkButton       *button,
                                        gpointer         user_data)
{
  GtkWidget *parent_window;
  GtkWidget *dialog;
  GtkWidget *file_entry;
  dbg_out("here\n");

  parent_window=lookup_widget(GTK_WIDGET(button),"configWindow1");

  dialog = gtk_file_chooser_dialog_new (_("LogFile"),
					GTK_WINDOW(parent_window),
					GTK_FILE_CHOOSER_ACTION_SAVE,
					GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
					NULL);
  gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (dialog), TRUE);
  if (gtk_dialog_run(GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
  {
    char *filename;

    filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
    file_entry=lookup_widget(GTK_WIDGET(button),"entry1");
    gtk_entry_set_text(GTK_ENTRY(file_entry), filename); 
    g_free (filename);
  }
  gtk_widget_destroy (dialog);
}


void
on_logFileDialogBtn_released           (GtkButton       *button,
                                        gpointer         user_data)
{
  dbg_out("here\n");
}


void
on_logFileDialogBtn_clicked            (GtkButton       *button,
                                        gpointer         user_data)
{
  dbg_out("here\n");
}


void
on_entry1_editing_done                 (GtkCellEditable *celleditable,
                                        gpointer         user_data)
{
  dbg_out("here\n");
}


void
on_configAddBcastBtn_clicked           (GtkButton       *button,
                                        gpointer         user_data)
{
  GtkEntry *entry;
  GtkTreeView *treeview;
  const gchar *txt;

  dbg_out("here\n");

  entry = GTK_ENTRY(lookup_widget(GTK_WIDGET(button),"entry3"));
  treeview = GTK_TREE_VIEW(lookup_widget(GTK_WIDGET(button),"treeview4"));

  txt = gtk_entry_get_text(GTK_ENTRY(entry));
  if (txt && *txt) {
      GtkTreeModel *model;
      GtkTreeIter   newrow;

      if (has_dupulicated_string_in_cell(txt,treeview)){
	model = gtk_tree_view_get_model(GTK_TREE_VIEW(treeview));
      
	gtk_list_store_append(GTK_LIST_STORE(model), &newrow);
      
	gtk_list_store_set(GTK_LIST_STORE(model), &newrow, 0, txt, -1);
      
	gtk_entry_set_text(GTK_ENTRY(entry), ""); /* clear entry */
      }
    }
}


void
on_configRemoveBcastBtn_pressed        (GtkButton       *button,
                                        gpointer         user_data)
{
  dbg_out("here\n");
}


void
on_configRemoveBcastBtn_released       (GtkButton       *button,
                                        gpointer         user_data)
{
  dbg_out("here\n");
}


void
on_configRemoveBcastBtn_clicked        (GtkButton       *button,
                                        gpointer         user_data)
{
  GtkEntry *entry;
  GtkTreeView *treeview;
  GtkTreeSelection *sel;
  GtkTreeModel     *model;
  GtkTreeIter       selected_row;

  dbg_out("here\n");
  entry = GTK_ENTRY(lookup_widget(GTK_WIDGET(button),"entry3"));
  treeview = GTK_TREE_VIEW(lookup_widget(GTK_WIDGET(button),"treeview4"));

  sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));

  g_assert(gtk_tree_selection_get_mode(sel) == GTK_SELECTION_SINGLE);

  if (gtk_tree_selection_get_selected(sel, &model, &selected_row))
    {
      gchar *str_data;

      gtk_tree_model_get (model, &selected_row, 
                          0, &str_data,
                          -1);
      gtk_list_store_remove(GTK_LIST_STORE(model), &selected_row);
      dbg_out("Data:%s\n",str_data);
      gtk_entry_set_text(GTK_ENTRY(entry), str_data);
      g_free(str_data);
    }
  else
    {
      g_assert_not_reached();
    }

  
}


void
on_configApplySettingBtn_clicked       (GtkButton       *button,
                                        gpointer         user_data)
{
  GtkWidget *top;
  GtkWidget *nickname_entry;
  GtkWidget *groupEntry;
  GtkWidget *logfile_entry;
  GtkWidget *configOpenCheckChkBtn;
  GtkWidget *configNonPopupCheckBtn;
  GtkWidget *configNoSoundCheckBtn;
  GtkWidget *configEncloseEnableCheckBtn;
  GtkWidget *configCitationCheckBtn;
  GtkWidget *configIPV6CheckBtn;
  GtkWidget *configDialUpCheckBtn;
  GtkWidget *configIconifyCheckBtn;
  GtkWidget *enableLogToggle;
  GtkWidget *loginNameLoggingToggle;
  GtkWidget *logIPAddrToggle;
  GtkWidget *logLockToggle;
  GtkComboBox *codeset_combobox;
  GtkTreeView *treeview;
  GtkTreeModel *model;
  GtkTreeIter iter;
  GSList *addr_top,*addr_list;
  gchar *string;
  gboolean valid;

  dbg_out("here\n");

  top=lookup_widget(GTK_WIDGET(button),"configWindow1");
  g_assert(top);
  groupEntry=lookup_widget(GTK_WIDGET(button),"comboboxentry1");
  g_assert(groupEntry);

  nickname_entry=lookup_widget(GTK_WIDGET(button),"entry2");
  g_assert(nickname_entry);

  logfile_entry=lookup_widget(GTK_WIDGET(button),"entry1");
  g_assert(logfile_entry);

  treeview = GTK_TREE_VIEW(lookup_widget(GTK_WIDGET(button),"treeview4"));
  g_assert(treeview);

  configOpenCheckChkBtn=lookup_widget(GTK_WIDGET(button),"configOpenCheckChkBtn");
  g_assert(configOpenCheckChkBtn);
  configNonPopupCheckBtn=lookup_widget(GTK_WIDGET(button),"configNonPopupCheckBtn");
  g_assert(configNonPopupCheckBtn);
  configNoSoundCheckBtn=lookup_widget(GTK_WIDGET(button),"configNoSoundCheckBtn");
  g_assert(configNoSoundCheckBtn);
  configEncloseEnableCheckBtn=lookup_widget(GTK_WIDGET(button),"configEncloseEnableCheckBtn");
  g_assert(configEncloseEnableCheckBtn);
  configCitationCheckBtn=lookup_widget(GTK_WIDGET(button),"configCitationCheckBtn");
  g_assert(configCitationCheckBtn);
  configIPV6CheckBtn=lookup_widget(GTK_WIDGET(button),"configIPV6CheckBtn");
  g_assert(configIPV6CheckBtn);
  
  configDialUpCheckBtn=lookup_widget(GTK_WIDGET(button),"configDialUpCheckBtn");
  g_assert(configDialUpCheckBtn);

  configIconifyCheckBtn = lookup_widget(GTK_WIDGET(button), "configIconifyCheckBtn");
  g_assert(configIconifyCheckBtn != NULL);

  codeset_combobox = 
    GTK_COMBO_BOX(lookup_widget(GTK_WIDGET(button), "configEncodingComboBox"));
  g_assert(codeset_combobox != NULL);
  string = gtk_combo_box_get_active_text(GTK_COMBO_BOX(codeset_combobox));
  g_assert(string != NULL);
  hostinfo_set_encoding(string);
  g_free(string);

  enableLogToggle = lookup_widget(GTK_WIDGET(button),"enableLogToggle");
  g_assert(enableLogToggle);
  loginNameLoggingToggle = lookup_widget(GTK_WIDGET(button),"loginNameLoggingToggle");
  g_assert(loginNameLoggingToggle);
  logIPAddrToggle = lookup_widget(GTK_WIDGET(button),"logIPAddrToggle");
  g_assert(logIPAddrToggle);
  logLockToggle = lookup_widget(GTK_WIDGET(button), "logLockToggle");
  g_assert(logLockToggle);

  hostinfo_set_nick_name(gtk_entry_get_text (GTK_ENTRY(nickname_entry)));

  string = gtk_combo_box_get_active_text(GTK_COMBO_BOX(groupEntry));
  hostinfo_set_group_name(string);
  g_free(string);

  hostinfo_set_ipmsg_logfile(gtk_entry_get_text(GTK_ENTRY(logfile_entry)));

  hostinfo_set_ipmsg_default_confirm(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(configOpenCheckChkBtn)));

  hostinfo_set_ipmsg_default_popup(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(configNonPopupCheckBtn)));

  if (is_sound_system_available()) 
    hostinfo_set_ipmsg_default_sound(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(configNoSoundCheckBtn)));
  
  hostinfo_set_ipmsg_default_enclose(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(configEncloseEnableCheckBtn)));

  hostinfo_set_ipmsg_default_citation(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(configCitationCheckBtn)));

  hostinfo_set_ipmsg_ipv6_mode(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(configIPV6CheckBtn)));

  hostinfo_set_ipmsg_dialup_mode(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(configDialUpCheckBtn)));

  hostinfo_set_ipmsg_iconify_dialogs(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(configIconifyCheckBtn)));  

  hostinfo_set_ipmsg_enable_log(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(enableLogToggle)));

  hostinfo_set_ipmsg_logname_logging(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(loginNameLoggingToggle)));

  hostinfo_set_ipmsg_ipaddr_logging(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(logIPAddrToggle)));

  hostinfo_set_log_locked_message_handling(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(logLockToggle)));
 
  addr_top = NULL;
  model = gtk_tree_view_get_model(treeview);
  valid = gtk_tree_model_get_iter_first (model, &iter);
  while(valid) {
      gchar *str_data;

      gtk_tree_model_get (model, &iter, 
                          0, &str_data,
                          -1);
      dbg_out("Addr:%s\n",str_data);
      addr_top=g_slist_append(addr_top,str_data);
      valid = gtk_tree_model_iter_next (model, &iter);
  }

  if (addr_top) {
    hostinfo_set_ipmsg_broadcast_list(addr_top);
    addr_list=addr_top;
    while(addr_list) {
      g_free(addr_list->data);
      addr_list=g_slist_next(addr_list);
    }
    g_slist_free(addr_top);
    g_assert(g_slist_length(addr_top)==1);
  }
  ipmsg_send_br_absence(udp_con,0);
  gtk_widget_destroy(top);
}


void
on_configApplySettingBtn_pressed       (GtkButton       *button,
                                        gpointer         user_data)
{
  dbg_out("here\n");
}


void
on_configApplySettingBtn_released      (GtkButton       *button,
                                        gpointer         user_data)
{
  dbg_out("here\n");
}


void
on_configCancelBtn_pressed             (GtkButton       *button,
                                        gpointer         user_data)
{
  dbg_out("here\n");
}


void
on_configCancelBtn_released            (GtkButton       *button,
                                        gpointer         user_data)
{
  dbg_out("here\n");
}


void
on_configCancelBtn_clicked             (GtkButton       *button,
                                        gpointer         user_data)
{
  GtkWidget *top;

  dbg_out("here\n");
  top=lookup_widget(GTK_WIDGET(button),"configWindow1");
  g_assert(top);
  gtk_widget_destroy(top);

}

GtkWidget *
internal_create_messageWindow(void){
  GtkWidget *messageWindow;
  GtkWidget *userview;
  GtkWidget *textview;
  GtkWidget *usersEntry;
  GtkWidget *usersVPane;
  gint width,height;

  messageWindow = create_messageWindow ();
  g_assert(messageWindow);

  userview = lookup_widget(messageWindow,"messageUserTree");
  g_assert(userview);

  textview = lookup_widget(messageWindow,"textview1");
  g_assert(textview);

  usersVPane = lookup_widget(messageWindow,"vpaned1");
  g_assert(usersVPane);
  
  setup_message_tree_view(userview);

  gtk_drag_dest_set(userview, GTK_DEST_DEFAULT_ALL, targetentries, 1,
		    GDK_ACTION_COPY|GDK_ACTION_MOVE|GDK_ACTION_LINK);

  gtk_drag_dest_set(usersVPane, GTK_DEST_DEFAULT_ALL, targetentries, 1,
		    GDK_ACTION_COPY|GDK_ACTION_MOVE|GDK_ACTION_LINK);
  gtk_tree_view_set_column_drag_function(GTK_TREE_VIEW(userview),NULL,NULL,NULL);
  gtk_drag_dest_unset(textview);
  gtk_drag_dest_set(textview, GTK_DEST_DEFAULT_ALL, targetentries, 1,
		    GDK_ACTION_COPY|GDK_ACTION_MOVE|GDK_ACTION_LINK);

  if (!hostinfo_get_ipmsg_message_window_size(&width,&height)){
    dbg_out("Resize:(%d,%d)\n",width,height);
    gtk_window_resize (GTK_WINDOW(messageWindow),width,height);
  }
  gtk_widget_set_events (GTK_WIDGET(messageWindow),GDK_KEY_RELEASE);
  gtk_signal_connect (GTK_OBJECT (messageWindow), "key_release_event",
		      GTK_SIGNAL_FUNC (on_messageWindow_key_release_event), NULL);
  /*
   *  表示を中央寄せに設定
   */
  usersEntry=lookup_widget(GTK_WIDGET(messageWindow),"messageWinUsersEntry");
  g_assert(usersEntry);
  gtk_entry_set_alignment(GTK_ENTRY(usersEntry),0.5);

  userdb_add_waiter_window(messageWindow);

  return messageWindow;
}
void
on_startBtn_clicked                    (GtkButton       *button,
                                        gpointer         user_data)
{
  GtkWidget *messageWindow;

  /* We can not use button widget because this is called from applet/systray */
  dbg_out("here\n");
  if (recv_windows_are_stored()) {
    show_stored_windows();
  } else {
    messageWindow = internal_create_messageWindow();
    g_assert(messageWindow);

    gtk_widget_show (messageWindow);
    update_users_on_message_window(messageWindow,FALSE); 
  }
}
static void
onAttachFileSelectionChanged (GtkTreeSelection *sel, GtkListStore *liststore)
{
	GtkTreeIter  selected_row;
	GtkWidget    *view;
	GtkWidget    *remove_btn;

	view=GTK_WIDGET(gtk_tree_selection_get_tree_view(sel));
	g_assert(view);
	remove_btn=lookup_widget(GTK_WIDGET(view),"AttachFileRemoveBtn");
	g_assert(remove_btn);
	g_assert(gtk_tree_selection_get_mode(sel) == GTK_SELECTION_SINGLE);

	if (gtk_tree_selection_get_selected(sel, NULL, &selected_row))
	{
		gtk_widget_set_sensitive(remove_btn, TRUE);
	}
	else
	{
		gtk_widget_set_sensitive(remove_btn, FALSE);
	}
}

static GtkWidget *
setup_attachment_editor(GtkWidget *mainwindow){
  GtkWidget           *attachment_editor;
  GtkWidget           *mainframe;
  GtkWidget           *entry;
  GtkTreeView         *view;
  GtkListStore        *liststore;
  GtkTreeViewColumn   *col;
  GtkCellRenderer     *renderer;
  GtkTreeSelection    *sel;
  gint                 width,height;

  dbg_out("here\n");
  attachment_editor = (GtkWidget*) g_object_get_data (G_OBJECT (mainwindow),
						      "attach_win");
  if (attachment_editor) {
    gtk_widget_grab_focus (attachment_editor);
    gtk_window_present (GTK_WINDOW(attachment_editor)); /* 前面移動  */
    return attachment_editor;
  }
  dbg_out("Create new editor\n");

  attachment_editor=create_attachFileEditor ();
  g_assert(attachment_editor);
  view=GTK_TREE_VIEW(lookup_widget(attachment_editor,"attachedFilesView"));  
  g_assert(view);

  liststore = gtk_list_store_new(3, 
				 G_TYPE_STRING,
				 G_TYPE_STRING,
				 G_TYPE_STRING);
  g_assert(liststore);

  gtk_tree_view_set_model(GTK_TREE_VIEW(view), GTK_TREE_MODEL(liststore));
  g_object_unref(liststore);

  /*
   * タイトル設定
   */
  /* --- Column #1 --- */

  col = gtk_tree_view_column_new();

  gtk_tree_view_column_set_title(col, _("FilePath"));

  /* pack tree view column into tree view */
  gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);

  renderer = gtk_cell_renderer_text_new();

  /* pack cell renderer into tree view column */
  gtk_tree_view_column_pack_start(col, renderer, TRUE);

  /* connect 'text' property of the cell renderer to
   *  model column that contains the first name */
  gtk_tree_view_column_add_attribute(col, renderer, "text", 0);

  gtk_tree_view_column_set_resizable (col,TRUE);

  /* --- Column #2 --- */

  col = gtk_tree_view_column_new();

  gtk_tree_view_column_set_title(col, _("FileSize"));

  /* pack tree view column into tree view */
  gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);

  renderer = gtk_cell_renderer_text_new();

  /* pack cell renderer into tree view column */
  gtk_tree_view_column_pack_start(col, renderer, TRUE);

  /* connect 'text' property of the cell renderer to
   *  model column that contains the first name */
  gtk_tree_view_column_add_attribute(col, renderer, "text", 1);

  gtk_tree_view_column_set_resizable (col,TRUE);

  /* --- Column #3 --- */

  col = gtk_tree_view_column_new();

  gtk_tree_view_column_set_title(col, _("FileType"));

  /* pack tree view column into tree view */
  gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);

  renderer = gtk_cell_renderer_text_new();

  /* pack cell renderer into tree view column */
  gtk_tree_view_column_pack_start(col, renderer, TRUE);

  /* connect 'text' property of the cell renderer to
   *  model column that contains the first name */
  gtk_tree_view_column_add_attribute(col, renderer, "text", 2);

  gtk_tree_view_column_set_resizable (col,TRUE);

  GLADE_HOOKUP_OBJECT (mainwindow, attachment_editor, "attach_win");
  GLADE_HOOKUP_OBJECT (attachment_editor,mainwindow,  "main_win");

  sel=gtk_tree_view_get_selection(GTK_TREE_VIEW(view));
  gtk_tree_selection_set_mode(sel, GTK_SELECTION_SINGLE);
  g_signal_connect(sel, "changed", 
		   G_CALLBACK(onAttachFileSelectionChanged),
		   liststore);
  GTK_WIDGET_SET_FLAGS(attachment_editor,GTK_CAN_FOCUS);
  /*
   * Drop target setting
   */
  gtk_drag_dest_set(GTK_WIDGET(view), GTK_DEST_DEFAULT_ALL, targetentries, 1,
		    GDK_ACTION_COPY|GDK_ACTION_MOVE|GDK_ACTION_LINK);

  /*mainframe*/
  mainframe=lookup_widget(attachment_editor,"attachFileEditorMainFrame");
  g_assert(mainframe);
  gtk_drag_dest_set(mainframe, GTK_DEST_DEFAULT_ALL, targetentries, 1,
		    GDK_ACTION_COPY|GDK_ACTION_MOVE|GDK_ACTION_LINK);

  /*entry*/
  entry=lookup_widget(attachment_editor,"AttachFilePathEntry");
  g_assert(entry);
  gtk_drag_dest_set(entry, GTK_DEST_DEFAULT_ALL, targetentries, 1,
		    GDK_ACTION_COPY|GDK_ACTION_MOVE|GDK_ACTION_LINK);

  if (!hostinfo_get_ipmsg_attach_editor_size(&width,&height)){
    dbg_out("Resize:(%d,%d)\n",width,height);
    gtk_window_resize (GTK_WINDOW(attachment_editor),width,height);
  }

  return attachment_editor;
}

static void
on_add_new_file(const gchar *filename,GtkWidget *parent_window,gboolean is_clear) {
  int rc;
  GtkWidget *file_entry;
  GtkTreeView *treeview;
  GtkTreeModel *model;
  GtkTreeIter   newrow;
  off_t num_size;
  time_t num_mtime;
  ipmsg_ftype_t  num_ftype;
  gchar str_size[256];

  dbg_out("here\n");

  file_entry=lookup_widget(parent_window,"AttachFilePathEntry");
  g_assert(file_entry);
  treeview = GTK_TREE_VIEW(lookup_widget(parent_window,"attachedFilesView"));
  g_assert(treeview);

  rc=has_dupulicated_string_in_cell(filename,treeview);
  if (!rc){
    dbg_out("Ignore duplicated entry %s\n",filename);
    return;
  }

  rc = get_file_info(filename, &num_size, &num_mtime, &num_ftype);
  if (rc<0) {
    dbg_out("Can not obtain file info %s %d\n",filename,num_ftype);
    return;
  }
  snprintf(str_size, 256,"%lld",num_size);
  str_size[254]=0;
  if (is_supported_file_type(num_ftype)) {
    model = gtk_tree_view_get_model(treeview);
    gtk_list_store_append(GTK_LIST_STORE(model), &newrow);
    gtk_list_store_set(GTK_LIST_STORE(model), &newrow, 
		       0, filename,
		       1, str_size,
		       2, get_file_type_name(num_ftype),
		       -1);
  }
  if (is_clear)
    gtk_entry_set_text(GTK_ENTRY(file_entry),"");  /* clear entry */
}

void
on_attachment1_activate                (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  GtkWidget *window;
  GtkWidget *attachment_editor;

  window=lookup_widget(GTK_WIDGET(menuitem),"messageWindow");
  g_assert(window);
  attachment_editor=setup_attachment_editor(window);
  g_assert(attachment_editor);

  gtk_widget_show(attachment_editor);
}


void
on_quit1_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  dbg_out("here\n");
  cleanup_message_watcher();
  gtk_main_quit();
}


void
on_entry2_activate                     (GtkEntry        *entry,
                                        gpointer         user_data)
{
  dbg_out("here\n");
}


gboolean
on_downloadWindow_destroy_event        (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
  dbg_out("here\n");
  return FALSE;
}


void
on_clist4_click_column                 (GtkCList        *clist,
                                        gint             column,
                                        gpointer         user_data)
{
  dbg_out("here\n");
}


void
on_downloadSaveBtn_clicked             (GtkButton       *button,
                                        gpointer         user_data)
{
  dbg_out("here\n");
}


void
on_close_button1_clicked               (GtkButton       *button,
                                        gpointer         user_data)
{
  dbg_out("here\n");
}


void
on_configWindow_destroy                (GtkObject       *object,
                                        gpointer         user_data)
{

}


void
on_initialWindow_destroy               (GtkObject       *object,
                                        gpointer         user_data)
{

  dbg_out("here\n");
  on_quit1_activate((GtkMenuItem *)NULL,user_data);
}


void
on_downloadWindow_destroy              (GtkObject       *object,
                                        gpointer         user_data)
{
  GtkWidget *window;
  ipmsg_recvmsg_private_t *sender_info;
  ipmsg_private_data_t *priv;

  dbg_out("here\n");

  window=lookup_widget(GTK_WIDGET(object),"downloadWindow");
  g_assert(window);

  priv=(ipmsg_private_data_t *)lookup_widget(GTK_WIDGET(window),"senderInfo");
  g_assert(priv);
  sender_info=priv->data; 
  g_assert(sender_info->ipaddr);
  /* FIX ME */
  ipmsg_send_release_files(udp_con,sender_info->ipaddr,sender_info->pktno);
  remove_window_from_displaylist(window);
  /* privateは自動破棄される  */
}



void
on_messageWindow_drag_data_get         (GtkWidget       *widget,
                                        GdkDragContext  *drag_context,
                                        GtkSelectionData *data,
                                        guint            info,
                                        guint            time,
                                        gpointer         user_data)
{

}


void
on_messageWindow_drag_end              (GtkWidget       *widget,
                                        GdkDragContext  *drag_context,
                                        gpointer         user_data)
{

}


void
on_downloadfilechooserdialog_destroy   (GtkObject       *object,
                                        gpointer         user_data)
{

}


GtkFileChooserConfirmation
on_downloadfilechooserdialog_confirm_overwrite
                                        (GtkFileChooser  *filechooser,
                                        gpointer         user_data)
{
  return TRUE;
}


void
on_downloadfilechooserdialog_current_folder_changed
                                        (GtkFileChooser  *filechooser,
                                        gpointer         user_data)
{

}


void
on_downloadfilechooserdialog_file_activated
                                        (GtkFileChooser  *filechooser,
                                        gpointer         user_data)
{

}


void
on_downloadfilechooserdialog_update_preview
                                        (GtkFileChooser  *filechooser,
                                        gpointer         user_data)
{

}


void
on_downloadfilechooserdialog_close     (GtkDialog       *dialog,
                                        gpointer         user_data)
{

}


void
on_downloadfilechooserdialog_response  (GtkDialog       *dialog,
                                        gint             response_id,
                                        gpointer         user_data)
{

}


void
on_logfileChooserdialog_destroy        (GtkObject       *object,
                                        gpointer         user_data)
{

}


void
on_logfileChooserdialog_close          (GtkDialog       *dialog,
                                        gpointer         user_data)
{

}


void
on_logfileChooserdialog_response       (GtkDialog       *dialog,
                                        gint             response_id,
                                        gpointer         user_data)
{

}


void
on_aboutdialog_destroy                 (GtkObject       *object,
                                        gpointer         user_data)
{
  dbg_out("here\n");
}


void
on_aboutdialog_close                   (GtkDialog       *dialog,
                                        gpointer         user_data)
{

}


void
on_aboutdialog_response                (GtkDialog       *dialog,
                                        gint             response_id,
                                        gpointer         user_data)
{
  gtk_widget_destroy(GTK_WIDGET(dialog));
}


void
on_readNotifyDialog_close              (GtkDialog       *dialog,
                                        gpointer         user_data)
{

}


void
on_readNotifyDialog_response           (GtkDialog       *dialog,
                                        gint             response_id,
                                        gpointer         user_data)
{

}


void
on_readNotifyDialog_destroy            (GtkObject       *object,
                                        gpointer         user_data)
{

}


void
on_readNotifyDialogOKBtn_pressed       (GtkButton       *button,
                                        gpointer         user_data)
{
  GtkWidget *top;

  top=lookup_widget(GTK_WIDGET(button),"readNotifyDialog");
  g_assert(top);
  gtk_widget_destroy(top);

}

void
on_messageWindow_show                  (GtkWidget       *widget,
                                        gpointer         user_data)
{
  GtkWidget *view = NULL;
  GtkToggleButton *enclose_toggle = NULL;
  GtkToggleButton    *lock_toggle = NULL;
  GtkWidget *window = NULL;
  GtkWidget *textview = NULL;

  dbg_out("here\n");

  window = 
	  lookup_widget(widget,"messageWindow");
  g_assert(window != NULL);

  enclose_toggle = 
	  GTK_TOGGLE_BUTTON(lookup_widget(widget,"encloseCheckBtn"));
  g_assert(enclose_toggle != NULL);

  lock_toggle = 
	  GTK_TOGGLE_BUTTON(lookup_widget(widget, "lockChkBtn"));
  g_assert(lock_toggle != NULL);

  view = lookup_widget(widget, "messageUserTree");
  g_assert(view != NULL);

  textview = lookup_widget(widget, "textview1");
  g_assert(textview != NULL);

  if (hostinfo_refer_ipmsg_default_enclose()) 
	  gtk_toggle_button_set_active(enclose_toggle, TRUE);
  else
	  gtk_toggle_button_set_active(enclose_toggle, FALSE);

  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(enclose_toggle)))
	  gtk_widget_set_sensitive(GTK_WIDGET(lock_toggle), TRUE);
  else
	  gtk_widget_set_sensitive(GTK_WIDGET(lock_toggle), FALSE);
}


void
on_viewWindow_destroy                  (GtkObject       *object,
                                        gpointer         user_data)
{
  GtkWidget *window;

  dbg_out("here\n");

  window=lookup_widget(GTK_WIDGET(object),"viewWindow");
  g_assert(window);
  remove_window_from_openlist(window);
  remove_window_from_displaylist(window);
}


void
on_viewWindow_show                     (GtkWidget       *widget,
                                        gpointer         user_data)
{

}


void
on_initialWindow_show                  (GtkWidget       *widget,
                                        gpointer         user_data)
{
  GtkWidget *startBtn;

  startBtn=lookup_widget(widget,"startBtn");
  gtk_signal_connect (GTK_OBJECT (startBtn), "button_press_event",
		      GTK_SIGNAL_FUNC (on_init_win_event_button_press_event),
		      NULL);
}


void
on_viewwindowCloseBtn_clicked          (GtkButton       *button,
                                        gpointer         user_data)
{
  GtkWidget *top;

  top=lookup_widget(GTK_WIDGET(button),"viewWindow");
  g_assert(top);
  gtk_widget_destroy(top);
}

void
select_user_list_by_addr(GtkTreeView *treeview,const char *ipaddr) {
  GtkTreeSelection *sel;
  GtkTreeModel *model;
  GtkTreeIter iter;
  gboolean valid;

  g_assert((treeview) && (ipaddr));

  sel=gtk_tree_view_get_selection(treeview);

  model = gtk_tree_view_get_model(treeview);
  valid = gtk_tree_model_get_iter_first (model, &iter);
  while(valid) {
      gchar *tree_ipaddr;

      gtk_tree_model_get (model, &iter, 
                          USER_VIEW_IPADDR_ID, &tree_ipaddr,
                          -1);
      dbg_out("Addr:%s\n",tree_ipaddr);
      if (!strcmp(tree_ipaddr,ipaddr)) {
	dbg_out("Select : %s\n",tree_ipaddr);
	gtk_tree_selection_select_iter(sel,&iter);
      }
      g_free(tree_ipaddr);
      valid = gtk_tree_model_iter_next (model, &iter);
  }
  
}
void
on_viewwindowReplyBtn_clicked          (GtkButton       *button,
                                        gpointer         user_data)
{
  GtkWidget *view;
  GtkWidget *messageWindow;
  GtkTextView *text_view;
  GtkTextIter start_iter,end_iter;
  GtkTextBuffer *buffer;
  GtkTextView *new_view;
  GtkTextBuffer *new_buffer;
  GtkWidget *cite_toggle;
  gint text_line;
  gchar *string;
  ipmsg_private_data_t *priv=NULL;
  ipmsg_recvmsg_private_t *sender_info;
  GtkWidget *messageUserTree;
  int i;

  dbg_out("here\n");

  messageWindow = internal_create_messageWindow();
  g_assert(messageWindow);
  messageUserTree=lookup_widget(messageWindow,"messageUserTree");
  g_assert(messageUserTree);

  view=lookup_widget(GTK_WIDGET(button),"viewWindow");
  text_view=GTK_TEXT_VIEW(lookup_widget(GTK_WIDGET(button),"viewwindowTextView"));
  cite_toggle=lookup_widget(GTK_WIDGET(button),"viewwindowCitationCheck");
  priv=(ipmsg_private_data_t *)lookup_widget(GTK_WIDGET(button),"senderInfo");

  g_assert(cite_toggle);
  g_assert(text_view);
  g_assert(view);
  g_assert(priv);

  IPMSG_ASSERT_PRIVATE(priv,IPMSG_PRIVATE_RECVMSG);
  sender_info=priv->data;
  dbg_out("sender_info:%p\n",sender_info);
  if (sender_info->ipaddr) {
    update_users_on_message_window(messageWindow,FALSE);
    select_user_list_by_addr(GTK_TREE_VIEW(messageUserTree),sender_info->ipaddr);    
  }

  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(cite_toggle))) {
    new_view=GTK_TEXT_VIEW(lookup_widget(messageWindow,"textview1"));
    new_buffer=gtk_text_view_get_buffer(new_view);
    
    g_assert(new_view);
    g_assert(new_buffer);

    /* following codes assume gtk_text_view_get_buffer(text_view) does not
     * allocate new reference
     * as gtk manual says.
     */
    buffer=gtk_text_view_get_buffer(text_view);
    text_line=gtk_text_buffer_get_line_count(buffer);
    dbg_out("lines:%d\n",text_line);
    for(i=0;i<text_line;++i) {
      
      gtk_text_buffer_get_iter_at_line(buffer,&start_iter,i);
      if (i<(text_line-1))
	gtk_text_buffer_get_iter_at_line(buffer,&end_iter,i+1);
      else
	gtk_text_buffer_get_end_iter(buffer,&end_iter);
      string=gtk_text_buffer_get_text(buffer,
				      &start_iter,
				      &end_iter,
				      FALSE);
      g_assert(string);
      gtk_text_buffer_insert_at_cursor(new_buffer,hostinfo_refer_ipmsg_cite_string(),-1);
      gtk_text_buffer_insert_at_cursor(new_buffer," ",-1);
      gtk_text_buffer_insert_at_cursor(new_buffer,string,-1);
      dbg_out("string:%s\n",string);
      g_free(string);
    }
  }

  gtk_widget_show (messageWindow);
  return;
}

void
on_comboboxentry1_editing_done         (GtkCellEditable *celleditable,
                                        gpointer         user_data)
{
  dbg_out("here\n");
}


void
on_comboboxentry1_set_focus_child      (GtkContainer    *container,
                                        GtkWidget       *widget,
                                        gpointer         user_data)
{

}


void
on_entry1_activate                     (GtkEntry        *entry,
                                        gpointer         user_data)
{

}


void
on_configWindow_show                   (GtkWidget       *widget,
                                        gpointer         user_data)
{
  GtkWidget *nickname_entry;
  GtkWidget *groupEntry;
  GtkWidget *logfile_entry;
  GtkWidget *configOpenCheckChkBtn;
  GtkWidget *configNonPopupCheckBtn;
  GtkWidget *configNoSoundCheckBtn;
  GtkWidget *configEncloseEnableCheckBtn;
  GtkWidget *configCitationCheckBtn;
  GtkWidget *configIPV6CheckBtn;
  GtkWidget *configDialUpCheckBtn;
  GtkWidget *configIconifyCheckBtn;
  GtkWidget *enableLogToggle;
  GtkWidget *loginNameLoggingToggle;
  GtkWidget *logIPAddrToggle;
  GtkWidget *logLockToggle;
  GtkTreeView *treeview;
  GtkWidget *addr_entry;
  GtkWidget *configAddBcastBtn;
  GtkTreeIter  selected_row;
  GtkWidget *configRemoveBcastBtn;
  GtkTreeSelection *sel;
  gchar  *entry_string;
  GSList *addr_list,*addr_top;
  int group_index;
  int err_count;

  groupEntry=lookup_widget(widget,"comboboxentry1");
  g_assert(groupEntry);

  nickname_entry=lookup_widget(widget,"entry2");
  g_assert(nickname_entry);

  logfile_entry=lookup_widget(widget,"entry1");
  g_assert(logfile_entry);

  configOpenCheckChkBtn=lookup_widget(widget,"configOpenCheckChkBtn");
  g_assert(configOpenCheckChkBtn);
  configNonPopupCheckBtn=lookup_widget(widget,"configNonPopupCheckBtn");
  g_assert(configNonPopupCheckBtn);
  configNoSoundCheckBtn=lookup_widget(widget,"configNoSoundCheckBtn");
  g_assert(configNoSoundCheckBtn);
  configEncloseEnableCheckBtn=lookup_widget(widget,"configEncloseEnableCheckBtn");
  g_assert(configEncloseEnableCheckBtn);
  configCitationCheckBtn=lookup_widget(widget,"configCitationCheckBtn");
  g_assert(configCitationCheckBtn);
  configIPV6CheckBtn=lookup_widget(widget,"configIPV6CheckBtn");
  g_assert(configIPV6CheckBtn);
  configDialUpCheckBtn=lookup_widget(widget,"configDialUpCheckBtn");
  g_assert(configDialUpCheckBtn);
  configIconifyCheckBtn = lookup_widget(widget, "configIconifyCheckBtn");
  g_assert(configIconifyCheckBtn != NULL);

  enableLogToggle=lookup_widget(widget,"enableLogToggle");
  g_assert(enableLogToggle);
  loginNameLoggingToggle=lookup_widget(widget,"loginNameLoggingToggle");
  g_assert(loginNameLoggingToggle);
  logIPAddrToggle=lookup_widget(widget,"logIPAddrToggle");
  g_assert(logIPAddrToggle);
  logLockToggle = lookup_widget(widget, "logLockToggle");
  g_assert(logLockToggle);

  treeview = GTK_TREE_VIEW(lookup_widget(widget,"treeview4"));
  g_assert(treeview);

  gtk_entry_set_text(GTK_ENTRY(nickname_entry), hostinfo_refer_nick_name());
  for(err_count=0,group_index=-1;
      ( (err_count<IPMSG_COMMON_MAX_RETRY) && (group_index<0) );
      ++err_count) {
    updateConfigWindowGroups(groupEntry);
    group_index=get_current_group_index(GTK_COMBO_BOX(groupEntry));
  }
  g_assert(group_index>=0);
  gtk_combo_box_set_active(GTK_COMBO_BOX(groupEntry),group_index);

  gtk_entry_set_text(GTK_ENTRY(logfile_entry), hostinfo_refer_ipmsg_logfile());

  if (hostinfo_refer_ipmsg_default_confirm())
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(configOpenCheckChkBtn),TRUE);
  else
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(configOpenCheckChkBtn),FALSE);

  if (hostinfo_refer_ipmsg_default_popup())
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(configNonPopupCheckBtn),TRUE);
  else
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(configNonPopupCheckBtn),FALSE);

  if (is_sound_system_available()) {
    if (hostinfo_refer_ipmsg_default_sound())
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(configNoSoundCheckBtn),TRUE);
    else
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(configNoSoundCheckBtn),FALSE);
  }else{
    gtk_widget_set_sensitive(configNoSoundCheckBtn, FALSE);
  }
  if (hostinfo_refer_ipmsg_default_enclose()) 
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(configEncloseEnableCheckBtn),TRUE);
  else
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(configEncloseEnableCheckBtn),FALSE);

  if (hostinfo_refer_ipmsg_default_citation())
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(configCitationCheckBtn),TRUE);
  else
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(configCitationCheckBtn),FALSE);

  if (hostinfo_refer_ipmsg_ipv6_mode())
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(configIPV6CheckBtn),TRUE);
  else
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(configIPV6CheckBtn),FALSE);

  if (hostinfo_refer_ipmsg_dialup_mode())
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(configDialUpCheckBtn),TRUE);
  else
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(configDialUpCheckBtn),FALSE);

  if (hostinfo_refer_ipmsg_iconify_dialogs())
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(configIconifyCheckBtn),TRUE);
  else
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(configIconifyCheckBtn),FALSE);

  if (hostinfo_refer_ipmsg_enable_log())
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(enableLogToggle),TRUE);
  else
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(enableLogToggle),FALSE);

  if ( (hostinfo_refer_ipmsg_logname_logging()) && (hostinfo_refer_ipmsg_enable_log()) )
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(loginNameLoggingToggle),TRUE);
  else
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(loginNameLoggingToggle),FALSE);

  if ( (hostinfo_refer_ipmsg_ipaddr_logging()) && (hostinfo_refer_ipmsg_enable_log()) )
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(logIPAddrToggle),TRUE);
  else
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(logIPAddrToggle),FALSE);

  if ( (hostinfo_refer_ipmsg_log_locked_message_handling()) && (hostinfo_refer_ipmsg_enable_log()) )
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(logLockToggle),TRUE);
  else
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(logLockToggle),FALSE);


  addr_top=addr_list=hostinfo_get_ipmsg_broadcast_list();
  while(addr_list != NULL) {
      GtkTreeModel *model;
      GtkTreeIter   newrow;
      gchar *element = addr_list->data;

      model = gtk_tree_view_get_model(treeview);
      gtk_list_store_append(GTK_LIST_STORE(model), &newrow);
      gtk_list_store_set(GTK_LIST_STORE(model), &newrow, 0, element, -1);
      g_free(element);
      addr_list=g_slist_next(addr_list);
  }
  if (addr_top)
    g_slist_free(addr_top);

  /*
   * button state
   */
  configAddBcastBtn=lookup_widget(widget,"configAddBcastBtn");
  g_assert(configAddBcastBtn);
  addr_entry=lookup_widget(widget,"entry3");
  g_assert(addr_entry);

  entry_string=(char *)gtk_entry_get_text(GTK_ENTRY(addr_entry));
  if ( (entry_string) && (*entry_string) ) 
    gtk_widget_set_sensitive(configAddBcastBtn,TRUE);
  else
    gtk_widget_set_sensitive(configAddBcastBtn,FALSE);


  sel=gtk_tree_view_get_selection(treeview);
  configRemoveBcastBtn=lookup_widget(widget,"configRemoveBcastBtn");
  g_assert(configRemoveBcastBtn);
  g_assert(gtk_tree_selection_get_mode(sel) == GTK_SELECTION_SINGLE);

  if (gtk_tree_selection_get_selected(sel, NULL, &selected_row))
    {
      gtk_widget_set_sensitive(configRemoveBcastBtn, TRUE);
    }
  else
    {
      gtk_widget_set_sensitive(configRemoveBcastBtn, FALSE);
    }

}

void
on_configWindow_clicked                (GtkButton       *button,
                                        gpointer         user_data)
{

}

void
update_one_group_entry(gpointer data,GtkComboBox *user_data) {
  userdb_t *current_user;
  GtkWidget *comboEntry=GTK_WIDGET(user_data);
  GtkTreeModel *model;
  GtkTreeIter iter;
  gboolean valid;

  if ( (!data) || (!user_data) )
    return;

  current_user=(userdb_t *)data;

  dbg_out("NickName: %s\n",current_user->nickname);
  dbg_out("Group: %s\n",current_user->group);
  dbg_out("User: %s\n",current_user->user);
  dbg_out("Host: %s\n",current_user->host);
  dbg_out("IP Addr: %s\n",current_user->ipaddr);

  model=gtk_combo_box_get_model(GTK_COMBO_BOX(comboEntry));
  if (!model)
    return;

  valid = gtk_tree_model_get_iter_first (model, &iter);

  while (valid) {
      gchar *str_data;
      /* Make sure you terminate calls to gtk_tree_model_get()
       * with a '-1' value
       */
      gtk_tree_model_get (model, &iter, 
                          0, &str_data,
                          -1);
	dbg_out("check: %s\n",str_data);
      if (!strcmp(current_user->group,str_data)) {
	dbg_out("found: %s\n",current_user->group);
	g_free(str_data);
	return; /* Already appended */
      }
      g_free(str_data);
      valid = gtk_tree_model_iter_next (model, &iter);
    }

  gtk_combo_box_append_text(GTK_COMBO_BOX(comboEntry),current_user->group);

  return;
}
void
on_configWindowAddGroupBtn_clicked     (GtkButton       *button,
                                        gpointer         user_data)
{
  GtkWidget *comboEntry;

  ipmsg_send_br_absence(udp_con,0);
  comboEntry=lookup_widget(GTK_WIDGET(button),"comboboxentry1");
  g_assert(comboEntry);
  updateConfigWindowGroups(comboEntry);
}


void
on_sendFailDialog_show                 (GtkWidget       *widget,
                                        gpointer         user_data)
{

}


void
on_sendFailDialog_destroy              (GtkObject       *object,
                                        gpointer         user_data)
{

}


void
on_sendFailDialog_response             (GtkDialog       *dialog,
                                        gint             response_id,
                                        gpointer         user_data)
{
  dbg_out("Response:%d\n",response_id);
}


void
on_sendFailDialog_close                (GtkDialog       *dialog,
                                        gpointer         user_data)
{

}


void
on_cancelbutton1_clicked               (GtkButton       *button,
                                        gpointer         user_data)
{

}


void
on_okbutton1_clicked                   (GtkButton       *button,
                                        gpointer         user_data)
{
  GtkWidget *dialog;
  dialog=lookup_widget(GTK_WIDGET(button),"sendFailDialog");
  gtk_dialog_response(GTK_DIALOG(dialog),GTK_RESPONSE_OK);
}

static int
handle_attachment_drag_data(GtkSelectionData *data,GtkWidget *attachment_editor){
  GList *files,*list;
  gchar *path;
  if ( (!data) || (!(data->data)) || (!attachment_editor) )
    return -EINVAL;

  files= gnome_vfs_uri_list_parse((gchar *)data->data);
  for(list = files; list != NULL; list = g_list_next(list) ) {
    path=gnome_vfs_unescape_string(gnome_vfs_uri_get_path(list->data),NULL);
    dbg_out("draged file: %s\n",path);
    on_add_new_file(path,attachment_editor,FALSE);
    g_free(path);
  }
  gnome_vfs_uri_list_free(files);

  return 0;
}
static int 
on_message_window_drag_data_received(GtkWidget *widget,GtkSelectionData *data){
  GtkWidget *window;
  GtkWidget *attachment_editor;
  int rc;

  if (!data)
    return -EINVAL;

  dbg_out("here:data %s\n",(char *)data->data);  

  window=lookup_widget(widget,"messageWindow");
  g_assert(window);
  attachment_editor=setup_attachment_editor(window);
  g_assert(attachment_editor);

  gtk_widget_show(attachment_editor);

  rc=handle_attachment_drag_data(data,attachment_editor);

  if (rc<0)
    ipmsg_err_dialog(_("Can not handle drag data %s (%d)"),strerror(-rc),-rc);

  return rc;
}

void
on_close1_activate                     (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  GtkWidget *top;

  dbg_out("here\n");
  top=lookup_widget(GTK_WIDGET(menuitem),"messageWindow");
  g_assert(top);
  gtk_widget_destroy(top);
}


void
on_attachFileEditor_destroy            (GtkObject       *object,
                                        gpointer         user_data)
{
  GtkWidget *mainwindow;
  GtkWidget *attachment_editor;

  dbg_out("here\n");
  mainwindow = (GtkWidget*) g_object_get_data (G_OBJECT (object),
						      "main_win");
  g_assert(mainwindow);
  dbg_out("Remove Assosiation\n");
  attachment_editor =  (GtkWidget*) g_object_get_data (G_OBJECT (mainwindow),
						      "attach_win");
  g_assert(attachment_editor);
  g_object_set_data (G_OBJECT (attachment_editor),
		     "main_win",
		     NULL);
  g_object_set_data (G_OBJECT (mainwindow),
		     "attach_win",
		     NULL);
  attachment_editor =  (GtkWidget*) g_object_get_data (G_OBJECT (mainwindow),
						      "attach_win");
  g_assert(!attachment_editor);
}


void
on_attachFileEditor_show               (GtkWidget       *widget,
                                        gpointer         user_data)
{
  GtkWidget *attachment_editor;
  GtkWidget *add_btn;
  GtkWidget *remove_btn;
  GtkWidget *view;
  GtkWidget *entry;
  GtkTreeSelection *sel;
  GtkTreeModel     *model;
  GtkTreeIter  selected_row;
  off_t size;
  time_t mtime;
  ipmsg_ftype_t type;
  int rc;
  gchar *filepath;

  dbg_out("here\n");

  /*
   * setup button/entry
   */
  attachment_editor=lookup_widget(widget,"attachFileEditor");
  g_assert(attachment_editor);

  add_btn=lookup_widget(GTK_WIDGET(attachment_editor),"AttachFIleAddBtn");
  g_assert(add_btn);
  remove_btn=lookup_widget(GTK_WIDGET(attachment_editor),"AttachFileRemoveBtn");
  g_assert(remove_btn);
  entry=lookup_widget(GTK_WIDGET(attachment_editor),"AttachFilePathEntry");
  g_assert(entry);

  filepath=(char *)gtk_entry_get_text(GTK_ENTRY(entry));

  rc=get_file_info(filepath, &size, &mtime, &type);
  if ( (rc) || (!is_supported_file_type(type)) )
    gtk_widget_set_sensitive(add_btn,FALSE);
  else
    gtk_widget_set_sensitive(add_btn,TRUE);

  view=lookup_widget(GTK_WIDGET(attachment_editor),"attachedFilesView");
  g_assert(GTK_TREE_VIEW(view));

  sel=gtk_tree_view_get_selection(GTK_TREE_VIEW(view));
  g_assert(gtk_tree_selection_get_mode(sel) == GTK_SELECTION_SINGLE);

  if (gtk_tree_selection_get_selected(sel, &model, &selected_row))
    gtk_widget_set_sensitive(remove_btn,TRUE);
  else
    gtk_widget_set_sensitive(remove_btn,FALSE);
}



void
on_AttachFIleAddBtn_clicked            (GtkButton       *button,
                                        gpointer         user_data)
{
  GtkWidget *parent_window;
  GtkWidget *file_entry;
  char *filename;

  dbg_out("here\n");

  parent_window=lookup_widget(GTK_WIDGET(button),"attachFileEditor");
  file_entry=lookup_widget(GTK_WIDGET(button),"AttachFilePathEntry");
  filename=(char *)gtk_entry_get_text(GTK_ENTRY(file_entry)); 

  on_add_new_file(filename,parent_window,TRUE);
}


void
on_AttachFileRemoveBtn_clicked         (GtkButton       *button,
                                        gpointer         user_data)
{
  GtkTreeIter  selected_row;
  GtkWidget    *view;
  GtkTreeSelection *sel;
  GtkTreeModel *model;
  GtkWidget    *remove_btn;
  GtkWidget    *file_entry;

  dbg_out("here\n");

  view=lookup_widget(GTK_WIDGET(button),"attachedFilesView");
  g_assert(view);
  remove_btn=lookup_widget(GTK_WIDGET(button),"AttachFileRemoveBtn");
  g_assert(remove_btn);  
  sel=gtk_tree_view_get_selection(GTK_TREE_VIEW(view));
  g_assert(sel);  
  file_entry=lookup_widget(GTK_WIDGET(button),"AttachFilePathEntry");
  g_assert(file_entry);
  g_assert(gtk_tree_selection_get_mode(sel) == GTK_SELECTION_SINGLE);

  if (gtk_tree_selection_get_selected(sel, &model, &selected_row))
    {
      gchar *tree_path;

      gtk_tree_model_get (model, &selected_row, 
                          0, &tree_path,
                          -1);
      dbg_out("Selected: %s\n",tree_path);
      gtk_entry_set_text(GTK_ENTRY(file_entry), tree_path);       
      g_free(tree_path);
      gtk_list_store_remove(GTK_LIST_STORE(model), &selected_row);
      gtk_widget_set_sensitive(remove_btn, FALSE);
    }
}

static int 
on_attach_window_drag_data_received(GtkWidget *widget,GtkSelectionData *data){
  GtkWidget *attachment_editor;
  int rc;

  if (!data)
    return -EINVAL;

  dbg_out("here:data %s\n",(char *)data->data);  


  attachment_editor=lookup_widget(widget,"attachFileEditor");
  g_assert(attachment_editor);

  gtk_widget_show(attachment_editor);

  rc=handle_attachment_drag_data(data,attachment_editor);
  if (rc<0)
    ipmsg_err_dialog(_("Can not handle drag data %s (%d)"),strerror(-rc),-rc);

  return rc;
}

void
on_attachedFilesView_drag_data_received
                                        (GtkWidget       *widget,
                                        GdkDragContext  *drag_context,
                                        gint             x,
                                        gint             y,
                                        GtkSelectionData *data,
                                        guint            info,
                                        guint            time,
                                        gpointer         user_data)
{
  int rc;

  dbg_out("here\n");

  if (!data)
    return;

  rc=on_attach_window_drag_data_received(widget,data);

  if (rc<0)
    ipmsg_err_dialog(_("Can not handle drag data %s (%d)"),strerror(-rc),-rc);

  return;
}
static int
elapsed_time(struct timeval *old,struct timeval *new,struct timeval *elaps){
  time_t sec;
  suseconds_t usec;

  if ( (!old) || (!new) || (!elaps) )
    return -EINVAL;

  if  ( (!(old->tv_sec)) && (!(old->tv_usec)) )
    return -ENOENT;

  sec=new->tv_sec;
  if (old->tv_usec > new->tv_usec) {
    --sec;
    usec=1000*1000 +(new->tv_usec - old->tv_usec);
  }else{
    usec= new->tv_usec - old->tv_usec ;
  }
  sec -= old->tv_sec;

  elaps->tv_sec=sec;
  elaps->tv_usec=usec;

  return 0;
}

void
on_DownLoadOKBtn_clicked               (GtkButton       *button,
                                        gpointer         user_data)
{
  dbg_out("Recv start");
  recv_attachments(button);
}


void
on_DownLoadCancelBtn_clicked           (GtkButton       *button,
                                        gpointer         user_data)
{
  GtkWidget *window;

  dbg_out("here:\n");
  window=lookup_widget(GTK_WIDGET(button),"downloadWindow");
  g_assert(window);
  gtk_widget_destroy (window);  
}


void
on_DownLoadOpenBtn_clicked             (GtkButton       *button,
                                        gpointer         user_data)
{
  int rc;
  GtkWidget *parent_window;
  GtkWidget *dialog;
  GtkWidget *file_entry;
  struct stat stat_buf;

  dbg_out("here\n");

  parent_window=lookup_widget(GTK_WIDGET(button),"downloadWindow");
  file_entry=lookup_widget(GTK_WIDGET(button),"DownLoadDirectoryEntry");
  /* 
   * なんで, GTK_FILE_CHOOSER_ACTION_SELECT_FOLDERが
   *  アンドキュメンテッドなの?
   */
  dialog = gtk_file_chooser_dialog_new ("Download Directory",
					GTK_WINDOW(parent_window),
					GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
					GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
					NULL);

  if (gtk_dialog_run(GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
  {
    char *filename;

    filename = gtk_file_chooser_get_current_folder (GTK_FILE_CHOOSER (dialog));
    rc=stat(filename,&stat_buf);
    if (rc<0) 
      goto free_file_out;
    if (!(S_ISDIR(stat_buf.st_mode)))
      goto free_file_out;
    gtk_entry_set_text(GTK_ENTRY(file_entry), filename);
  free_file_out:
    g_free (filename);
  }
  gtk_widget_destroy (dialog);
}

void
on_attachFileEditorMainFrame_drag_data_received
                                        (GtkWidget       *widget,
                                        GdkDragContext  *drag_context,
                                        gint             x,
                                        gint             y,
                                        GtkSelectionData *data,
                                        guint            info,
                                        guint            time,
                                        gpointer         user_data)
{
  int rc;

  dbg_out("here\n");

  if (!data)
    return;

  rc=on_attach_window_drag_data_received(widget,data);

  if (rc<0)
    ipmsg_err_dialog(_("Can not handle drag data %s (%d)"),strerror(-rc),-rc);

  return;
}

void
on_entry3_activate                     (GtkEntry        *entry,
                                        gpointer         user_data)
{
  GtkTreeView *treeview;
  const gchar *txt;

  dbg_out("here\n");

  g_assert(entry);
  treeview = GTK_TREE_VIEW(lookup_widget(GTK_WIDGET(entry),"treeview4"));

  txt = gtk_entry_get_text(GTK_ENTRY(entry));
  if (txt && *txt) {
      GtkTreeModel *model;
      GtkTreeIter   newrow;

      if (has_dupulicated_string_in_cell(txt,treeview)){
	model = gtk_tree_view_get_model(GTK_TREE_VIEW(treeview));
      
	gtk_list_store_append(GTK_LIST_STORE(model), &newrow);
      
	gtk_list_store_set(GTK_LIST_STORE(model), &newrow, 0, txt, -1);
      
	gtk_entry_set_text(GTK_ENTRY(entry), ""); /* clear entry */
      }
    }
}


void
on_entry3_changed                      (GtkEditable     *editable,
                                        gpointer         user_data)
{
  GtkWidget *addr_entry;
  GtkWidget *configAddBcastBtn;
  gchar *addr;

  configAddBcastBtn=lookup_widget(GTK_WIDGET(editable),"configAddBcastBtn");
  g_assert(configAddBcastBtn);
  addr_entry=lookup_widget(GTK_WIDGET(editable),"entry3");
  g_assert(addr_entry);

  addr=(char *)gtk_entry_get_text(GTK_ENTRY(addr_entry));
  if ( (addr) && (*addr) ) 
    gtk_widget_set_sensitive(configAddBcastBtn,TRUE);
  else
    gtk_widget_set_sensitive(configAddBcastBtn,FALSE);

}
void
on_downloadMonitor_destroy             (GtkObject       *object,
                                        gpointer         user_data)
{
  GtkWidget *window;

  dbg_out("here\n");

  window=lookup_widget(GTK_WIDGET(object),"downloadMonitor");
  download_monitor_remove_waiter_window(window);
}


void
on_downloadMonitor_show                (GtkWidget       *widget,
                                        gpointer         user_data)
{
  GtkWidget *window;  

  download_monitor_update_state();
}


void
on_deleteBtn_clicked                   (GtkButton       *button,
                                        gpointer         user_data)
{
  download_monitor_delete_btn_action(button,user_data);  
}


void
on_updateBtn_clicked                   (GtkButton       *button,
                                        gpointer         user_data)
{
  download_monitor_update_state();
}


void
on_closeBtn_clicked                    (GtkButton       *button,
                                        gpointer         user_data)
{
  GtkWidget *window;

  window=lookup_widget(GTK_WIDGET(button),"downloadMonitor");
  g_assert(window);
  gtk_widget_destroy(GTK_WIDGET(window));

}


void
on_initialWindow_check_resize          (GtkContainer    *container,
                                        gpointer         user_data)
{
  dbg_out("here\n");
}


void
on_downloadWindow_check_resize         (GtkContainer    *container,
                                        gpointer         user_data)
{
  dbg_out("here\n");
}


void
on_aboutdialog_check_resize            (GtkContainer    *container,
                                        gpointer         user_data)
{
  dbg_out("here\n");
}


void
on_viewWindow_check_resize             (GtkContainer    *container,
                                        gpointer         user_data)
{
  dbg_out("here\n");
}


void
on_attachFileEditor_check_resize       (GtkContainer    *container,
                                        gpointer         user_data)
{
  dbg_out("here\n");
}


void
on_downloadMonitor_check_resize        (GtkContainer    *container,
                                        gpointer         user_data)
{
  dbg_out("here\n");
}


void
on_initialWindow_size_allocate         (GtkWidget       *widget,
                                        GdkRectangle    *allocation,
                                        gpointer         user_data)
{
  g_assert(allocation);
  dbg_out("here:(%d,%d)\n",allocation->width,allocation->height);

}


void
on_downloadWindow_size_allocate        (GtkWidget       *widget,
                                        GdkRectangle    *allocation,
                                        gpointer         user_data)
{
  g_assert(allocation);
  dbg_out("here:(%d,%d)\n",allocation->width,allocation->height);
}


void
on_aboutdialog_size_allocate           (GtkWidget       *widget,
                                        GdkRectangle    *allocation,
                                        gpointer         user_data)
{
  g_assert(allocation);
  dbg_out("here:(%d,%d)\n",allocation->width,allocation->height);

}


void
on_viewWindow_size_allocate            (GtkWidget       *widget,
                                        GdkRectangle    *allocation,
                                        gpointer         user_data)
{
  g_assert(allocation);
  dbg_out("here:(%d,%d)\n",allocation->width,allocation->height);
  hostinfo_set_ipmsg_recv_window_size(allocation->width,
				      allocation->height);
}


void
on_attachFileEditor_size_allocate      (GtkWidget       *widget,
                                        GdkRectangle    *allocation,
                                        gpointer         user_data)
{
  g_assert(allocation);
  dbg_out("here:(%d,%d)\n",allocation->width,allocation->height);
  hostinfo_set_ipmsg_attach_editor_size(allocation->width,
					 allocation->height);

}


void
on_downloadMonitor_size_allocate       (GtkWidget       *widget,
                                        GdkRectangle    *allocation,
                                        gpointer         user_data)
{
  g_assert(allocation);
  dbg_out("here:(%d,%d)\n",allocation->width,allocation->height);

}


void
on_messageWindow_size_allocate         (GtkWidget       *widget,
                                        GdkRectangle    *allocation,
                                        gpointer         user_data)
{
  g_assert(allocation);
  dbg_out("here:(%d,%d)\n",allocation->width,allocation->height);
  hostinfo_set_ipmsg_message_window_size(allocation->width,
					 allocation->height);
}

gboolean
on_viewwindowTextView_button_press_event
                                        (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
  GtkTextView *text_view;
  GtkTextIter start_iter,end_iter;
  GtkTextBuffer *buffer;
  gchar *string;
  gchar *url;
  int rc;
  /* FixMe: Strictly speaking, this should set for each window */
  static struct timeval old_tv; 
  struct timeval new_tv;
  struct timeval elapsed_tv;
  GnomeVFSResult result;
  
  dbg_out("here\n");

  switch (event->button) {
  case 1:

#if GTK_CHECK_VERSION(2,10,0)
    gettimeofday(&new_tv, NULL);
    rc=elapsed_time(&old_tv,&new_tv, &elapsed_tv);

    if (rc == -EINVAL)
      goto update_out;

    if ( (rc != -ENOENT) && (!(elapsed_tv.tv_sec)) &&
	 (elapsed_tv.tv_usec < IPMSG_KEY_BTN_CHATTER_MS*1000) )
      return FALSE;

    text_view=GTK_TEXT_VIEW(lookup_widget(widget,"viewwindowTextView"));
    g_assert(text_view);

    buffer=gtk_text_view_get_buffer(text_view);
    if (!(gtk_text_buffer_get_has_selection(buffer)))
      return FALSE;

    dbg_out("selected\n");

      gtk_text_buffer_get_selection_bounds(buffer,
					   &start_iter,
					   &end_iter);
      string=gtk_text_buffer_get_text(buffer,
				      &start_iter,
				      &end_iter,
				      FALSE);
      dbg_out("String:%s\n",string);
      result = gnome_vfs_url_show (string);
      /*
       * 操作ミスなどで無効パラメタは容易に発生するため, ダイアログの
       * 表示対称外とした.
       */
      if ( (result != GNOME_VFS_OK) && 
	   (result != GNOME_VFS_ERROR_BAD_PARAMETERS) ) {
	ipmsg_err_dialog("%s URL=%s, reason: %s(%d)\n%s\n",
			 _("Can not open URL"), string, 
			 gnome_vfs_result_to_string(result), result, 
			 _("Please check your gnome configuration(url-handler setting)"));
      }
      g_free(string);

  update_out:
      memcpy(&old_tv, &new_tv, sizeof(struct timeval));
    break;
#endif  /*  GTK_CHECK_VERSION(2,10,0)  */
  default:					

    break;
  }

  return FALSE;
}


void
on_absenceEditor_destroy               (GtkObject       *object,
                                        gpointer         user_data)
{

}


void
on_absenceEditor_show                  (GtkWidget       *widget,
                                        gpointer         user_data)
{

}

void
on_applyAndAbsentBtn_clicked           (GtkButton       *button,
                                        gpointer         user_data)
{
  GtkWidget *view;
  GtkWidget *window;
  
  window=GTK_WIDGET(lookup_widget(GTK_WIDGET(button),"absenceEditor"));
  g_assert(window);
  view=GTK_WIDGET(lookup_widget(GTK_WIDGET(button),"absenseTitles"));
  g_assert(view);

  update_fuzai_config(view,TRUE);
  gtk_widget_destroy(GTK_WIDGET(window));
}


void
on_applyOnlyBtn_clicked                (GtkButton       *button,
                                        gpointer         user_data)
{
  GtkWidget *view;
  GtkWidget *window;
  
  window=GTK_WIDGET(lookup_widget(GTK_WIDGET(button),"absenceEditor"));
  g_assert(window);
  view=GTK_WIDGET(lookup_widget(GTK_WIDGET(button),"absenseTitles"));
  g_assert(view);

  update_fuzai_config(view,FALSE);
}


void
on_configIPV6CheckBtn_toggled          (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{

}

void
on_DownloadConfirmDialog_show          (GtkWidget       *widget,
                                        gpointer         user_data)
{

}


void
on_DownloadConfirmDialog_destroy       (GtkObject       *object,
                                        gpointer         user_data)
{

}


void
on_DownLoadConfirmClose_clicked        (GtkButton       *button,
                                        gpointer         user_data)
{

}


void
on_DownLoadConfirmExec_clicked         (GtkButton       *button,
                                        gpointer         user_data)
{

}

gboolean
on_messageWindow_key_release_event     (GtkWidget       *widget,
                                        GdkEventKey     *event,
                                        gpointer         user_data)
{
  int rc;
  /* FixMe: Strictly speaking, this should set for each window */
  static struct timeval old_tv; 
  struct timeval new_tv;
  struct timeval elapsed_tv;
  GtkWidget *view;

  view=lookup_widget(widget,"messageUserTree");
  if (!view) {
    dbg_out("No message window\n");
    return FALSE;
  }

  gettimeofday(&new_tv, NULL);

  if (elapsed_time(&old_tv,&new_tv, &elapsed_tv))
    goto update_out;

  if ( (!(elapsed_tv.tv_sec)) &&
       (elapsed_tv.tv_usec < IPMSG_KEY_BTN_CHATTER_MS*1000) )
    goto no_update_out;

  dbg_out("Here: key code: %d(%x) key: %s\n", event->keyval, event->keyval, event->string);  

  if (event->keyval == GDK_F5) {
    userdb_invalidate_userdb();
    ipmsg_send_br_entry(udp_con,0);
  }
 update_out:
  memcpy(&old_tv, &new_tv, sizeof(struct timeval));
  return TRUE;
 no_update_out:
  return FALSE;
}

void
on_textview1_drag_data_received        (GtkWidget       *widget,
                                        GdkDragContext  *drag_context,
                                        gint             x,
                                        gint             y,
                                        GtkSelectionData *data,
                                        guint            info,
                                        guint            time,
                                        gpointer         user_data)
{
  g_assert(widget);
  g_assert(data);

  dbg_out("here:data %s\n",(char *)data->data);  
  on_message_window_drag_data_received(widget,data);
}


gboolean
on_textview1_button_press_event        (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{

  return FALSE;
}


void
on_absenceEditorCloseBtn_clicked       (GtkButton       *button,
                                        gpointer         user_data)
{
  GtkWidget *window;
  
  window=GTK_WIDGET(lookup_widget(GTK_WIDGET(button),"absenceEditor"));
  g_assert(window);
  gtk_widget_destroy(GTK_WIDGET(window));
}


void
on_messageUserTree_drag_data_received  (GtkWidget       *widget,
                                        GdkDragContext  *drag_context,
                                        gint             x,
                                        gint             y,
                                        GtkSelectionData *data,
                                        guint            info,
                                        guint            time,
                                        gpointer         user_data)
{
  g_assert(widget);
  g_assert(data);

  dbg_out("here:data %s\n",(char *)data->data);  
  on_message_window_drag_data_received(widget,data);
}


void
on_messageWinUpdateBtn_clicked         (GtkButton       *button,
                                        gpointer         user_data)
{
  ipmsg_update_ui_user_list();
}


void
on_sendBtn_clicked                     (GtkButton       *button,
                                        gpointer         user_data)
{
  GtkWidget                  *top = NULL;
  GtkTextView          *text_view = NULL;
  GtkTextIter        start_iter,end_iter;
  GtkTextBuffer           *buffer = NULL;
  GtkToggleButton *enclose_toggle = NULL;
  GtkToggleButton    *lock_toggle = NULL;
  GtkTreeView               *view = NULL;
  GtkTreeSelection           *sel = NULL;
  GtkWidget    *attachment_editor = NULL;
  gint                  text_line = 0;
  gchar                   *string = NULL;
  gchar                *to_string = NULL;
  int                       flags = 0;
  send_info_t                info;
  GList                *path_list = NULL;
  GtkTreeModel             *model = NULL;
  gint               sender_count = 0;

  dbg_out("here\n");

  top = lookup_widget( GTK_WIDGET(button), "messageWindow");
  g_assert(top != NULL);

  text_view = GTK_TEXT_VIEW( lookup_widget( GTK_WIDGET(button), "textview1"));
  g_assert(text_view != NULL);

  enclose_toggle = GTK_TOGGLE_BUTTON( lookup_widget( GTK_WIDGET(button), "encloseCheckBtn"));
  g_assert(enclose_toggle != NULL);

  lock_toggle = GTK_TOGGLE_BUTTON( lookup_widget( GTK_WIDGET(button), "lockChkBtn"));
  g_assert(lock_toggle != NULL);

  view = GTK_TREE_VIEW(lookup_widget( GTK_WIDGET(button), "messageUserTree") );
  g_assert(view != NULL);

  attachment_editor = (GtkWidget*) g_object_get_data (G_OBJECT (top), "attach_win");

  sel = gtk_tree_view_get_selection(view);

  sender_count=gtk_tree_selection_count_selected_rows(sel);
  if (!sender_count) {
    dbg_out("Not selected\n");
    return;
  }


  /* following codes assume gtk_text_view_get_buffer(text_view) does not
   * allocate new reference
   * as gtk manual says.
   */
  buffer=gtk_text_view_get_buffer(text_view);
  text_line=gtk_text_buffer_get_line_count(buffer);
  dbg_out("lines:%d\n",text_line);
  gtk_text_buffer_get_start_iter(buffer,&start_iter);
  gtk_text_buffer_get_end_iter(buffer,&end_iter);
  string=gtk_text_buffer_get_text(buffer,
				  &start_iter,
				  &end_iter,
				  FALSE);
  g_assert(string);
  dbg_out("string:%s\n",string);
  /*
   * 送信属性取得
   */
  flags=hostinfo_get_normal_send_flags();
  path_list=gtk_tree_selection_get_selected_rows(sel,&model);
  if (g_list_length(path_list)>1)
    flags |= IPMSG_MULTICASTOPT;

  g_list_foreach (path_list, (GFunc)gtk_tree_path_free, NULL);
  g_list_free (path_list);

  /*
   * 封書
   */
  if (gtk_toggle_button_get_active(enclose_toggle))
    flags |= IPMSG_SECRETOPT;
  else
    flags &= ~IPMSG_SECRETOPT;

  /*
   * 錠前
   */
  if (gtk_toggle_button_get_active(lock_toggle))
    flags |= IPMSG_PASSWORDOPT;
  else
    flags &= ~IPMSG_PASSWORDOPT;

  /*
   * 添付
   */
  info.flags=flags;
  info.msg=string;
  if (attachment_editor) {
    dbg_out("This message has attachment\n");
    info.attachment_editor=attachment_editor;
    info.flags |= IPMSG_FILEATTACHOPT;
  }

  gtk_tree_selection_selected_foreach(sel,do_send,&info);

  if (hostinfo_is_ipmsg_absent()) { /* 不在モード強制解除 */
    hostinfo_set_ipmsg_absent(FALSE);
    ipmsg_send_br_absence(udp_con,0);
  }
  
 free_out:
  g_free(string);
  gtk_widget_destroy(top);

  return;
}



void
on_vpaned1_drag_data_received          (GtkWidget       *widget,
                                        GdkDragContext  *drag_context,
                                        gint             x,
                                        gint             y,
                                        GtkSelectionData *data,
                                        guint            info,
                                        guint            time,
                                        gpointer         user_data)
{
  g_assert(widget);
  g_assert(data);

  dbg_out("here:data %s\n",(char *)data->data);  
  on_message_window_drag_data_received(widget,data);
}


void
on_AttachFileFileChooseBtn_clicked     (GtkButton       *button,
                                        gpointer         user_data)
{
  GtkWidget *parent_window;
  GtkWidget *dialog;
  GtkWidget *file_entry;
  dbg_out("here\n");

  parent_window=lookup_widget(GTK_WIDGET(button),"attachFileEditor");

  dialog = gtk_file_chooser_dialog_new (_("AttachmentFile"),
					GTK_WINDOW(parent_window),
					GTK_FILE_CHOOSER_ACTION_OPEN,
					GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
					NULL);
  gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (dialog), TRUE);
  if (gtk_dialog_run(GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
  {
    char *filename;

    filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
    file_entry=lookup_widget(GTK_WIDGET(button),"AttachFilePathEntry");
    gtk_entry_set_text(GTK_ENTRY(file_entry), filename); 
    g_free (filename);
  }
  gtk_widget_destroy (dialog);
}


void
on_AttachFilePathEntry_activate        (GtkEntry        *entry,
                                        gpointer         user_data)
{
  GtkWidget *parent_window;
  char *filename;

  dbg_out("here\n");

  parent_window=lookup_widget(GTK_WIDGET(entry),"attachFileEditor");
  filename=(char *)gtk_entry_get_text(entry); 
}


void
on_AttachFilePathEntry_changed         (GtkEditable     *editable,
                                        gpointer         user_data)
{
  GtkWidget *file_entry;
  GtkWidget *btn;
  gchar *filepath;
  off_t size;
  time_t mtime;
  ipmsg_ftype_t type;
  int rc;

  btn=lookup_widget(GTK_WIDGET(editable),"AttachFIleAddBtn");
  file_entry=lookup_widget(GTK_WIDGET(editable),"AttachFilePathEntry");
  filepath=(char *)gtk_entry_get_text(GTK_ENTRY(file_entry));
  rc = get_file_info(filepath, &size, &mtime, &type);
  if ( (rc) || (!is_supported_file_type(type)) )
    gtk_widget_set_sensitive(btn,FALSE);
  else
    gtk_widget_set_sensitive(btn,TRUE);
}


void
on_AttachFilePathEntry_drag_data_received
                                        (GtkWidget       *widget,
                                        GdkDragContext  *drag_context,
                                        gint             x,
                                        gint             y,
                                        GtkSelectionData *data,
                                        guint            info,
                                        guint            time,
                                        gpointer         user_data)
{
  GList *files,*node;
  GtkWidget *entry;
  gchar *path;

  dbg_out("here\n");

  if ( (!widget) || (!data) || (!(data->data)) )
    return;

  entry=lookup_widget(widget,"AttachFilePathEntry");
  g_assert(entry);

  files= gnome_vfs_uri_list_parse((gchar *)data->data);
  if (g_list_length(files) == 1) {
    /*  単一ファイルの場合は, ファイルパスをエントリに格納  */
    node=g_list_first(files);
    g_assert(node);
    path=gnome_vfs_unescape_string(gnome_vfs_uri_get_path(files->data),NULL);
    dbg_out("draged file: %s\n",path);
    gtk_entry_set_text(GTK_ENTRY(entry),path);
    g_free(path);
  }else{
    dbg_out("droped multiple files.\n");
  }
  gnome_vfs_uri_list_free(files);
}

void
on_absenceEditor_size_allocate         (GtkWidget       *widget,
                                        GdkRectangle    *allocation,
                                        gpointer         user_data)
{
  g_assert(allocation);
  dbg_out("here:(%d,%d)\n",allocation->width,allocation->height);
  hostinfo_set_ipmsg_absence_editor_size(allocation->width,
				      allocation->height);

}


void
on_messageWinCloseBtn_clicked          (GtkButton       *button,
                                        gpointer         user_data)
{
  GtkWidget *window;

  dbg_out("here\n");

  window=lookup_widget(GTK_WIDGET(button),"messageWindow");
  g_assert(window);
  
  gtk_widget_destroy (window);
}

void
on_configApplyBtn_clicked              (GtkButton       *button,
                                        gpointer         user_data)
{
  GtkRadioAction *action;
  GtkWidget *window=NULL;
  GtkWidget *viewConfigGroupChkBtn=NULL;
  GtkWidget *viewConfigHostChkBtn=NULL;
  GtkWidget *viewConfigIPAddrChkBtn=NULL;
  GtkWidget *viewConfigLogonChkBtn=NULL;
  GtkWidget *viewConfigPriorityChkBtn=NULL;
  GtkWidget *viewConfigGridChkBtn=NULL;
  GtkWidget *viewConfigGroupSortChkBtn=NULL;
  GtkWidget *viewConfigGroupInverseSortBtn=NULL;
  GtkWidget *viewConfigSecondKeyUserRBtn=NULL;
  GtkWidget *viewConfigSecondKeyMachineRBtn=NULL;
  GtkWidget *viewConfigSecondKeyIPAddrRBtn=NULL;
  GtkWidget *viewConfigOtherSortInverseBtn=NULL;
  uint state=0;
  int sub_sort_id=0;

  dbg_out("here\n");


  window=lookup_widget(GTK_WIDGET(button),"viewConfigWindow");

  viewConfigGroupChkBtn=lookup_widget(window,"viewConfigGroupChkBtn");
  viewConfigHostChkBtn=lookup_widget(window, "viewConfigHostChkBtn");
  viewConfigIPAddrChkBtn=lookup_widget(window,"viewConfigIPAddrChkBtn");
  viewConfigLogonChkBtn=lookup_widget(window,"viewConfigLogonChkBtn");
  viewConfigPriorityChkBtn=lookup_widget(window,"viewConfigPriorityChkBtn");
  viewConfigGridChkBtn=lookup_widget(window,"viewConfigGridChkBtn");

  viewConfigGroupSortChkBtn=lookup_widget(window,"groupSortCheckBtn");
  viewConfigGroupInverseSortBtn=lookup_widget(window,"groupInverseBtn");

  viewConfigSecondKeyUserRBtn=lookup_widget(window,"secondKeyUserRBtn");
  viewConfigSecondKeyMachineRBtn=lookup_widget(window,"secondKeyMachineRBtn");
  viewConfigSecondKeyIPAddrRBtn=lookup_widget(window,"secondKeyIPAddrRBtn");
  viewConfigOtherSortInverseBtn=lookup_widget(window,"secondKeyInverseBtn");

  g_assert(viewConfigGroupChkBtn);
  g_assert(viewConfigHostChkBtn);
  g_assert(viewConfigIPAddrChkBtn);
  g_assert(viewConfigLogonChkBtn);
  g_assert(viewConfigPriorityChkBtn);
  g_assert(viewConfigGridChkBtn);

  g_assert(viewConfigGroupSortChkBtn);
  g_assert(viewConfigGroupInverseSortBtn);

  g_assert(viewConfigSecondKeyUserRBtn);
  g_assert(viewConfigSecondKeyMachineRBtn);
  g_assert(viewConfigSecondKeyIPAddrRBtn);
  g_assert(viewConfigOtherSortInverseBtn);


  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(viewConfigGroupChkBtn)))
    state |= HEADER_VISUAL_GROUP_ID;

  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(viewConfigHostChkBtn)))
    state |= HEADER_VISUAL_HOST_ID;

  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(viewConfigIPAddrChkBtn)))
    state |= HEADER_VISUAL_IPADDR_ID;

  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(viewConfigLogonChkBtn)))
    state |= HEADER_VISUAL_LOGON_ID;

  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(viewConfigPriorityChkBtn)))
    state |= HEADER_VISUAL_PRIO_ID;

  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(viewConfigGridChkBtn)))
    state |= HEADER_VISUAL_GRID_ID;

  /* group */
  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(viewConfigGroupSortChkBtn)))
    hostinfo_set_ipmsg_sort_with_group(TRUE);
  else
    hostinfo_set_ipmsg_sort_with_group(FALSE);

  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(viewConfigGroupInverseSortBtn)))
    hostinfo_set_ipmsg_group_sort_order(FALSE);
  else
    hostinfo_set_ipmsg_group_sort_order(TRUE);
  
  /*
   * sub sort
   */

 if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(viewConfigSecondKeyIPAddrRBtn)))
   hostinfo_set_ipmsg_sub_sort_id(SORT_TYPE_IPADDR);
 else 
   if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(viewConfigSecondKeyMachineRBtn)))
     hostinfo_set_ipmsg_sub_sort_id(SORT_TYPE_MACHINE);
   else
     hostinfo_set_ipmsg_sub_sort_id(SORT_TYPE_USER);


 if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(viewConfigOtherSortInverseBtn)))
   hostinfo_set_ipmsg_sub_sort_order(FALSE);
 else
   hostinfo_set_ipmsg_sub_sort_order(TRUE);

  dbg_out("New state:%x (%d)\n",state,state);
  hostinfo_set_header_state(state);
  gtk_widget_destroy(GTK_WIDGET(window));  
  
}
static int
update_prioriy_for_selection(GtkMenuItem *menuitem,int prio){
  GtkWidget *window;
  GtkTreeView *view;
  GtkTreeSelection *sel;
  GtkTreeModel *model;
  GtkTreeIter iter;
  GList *pathes,*node;

  dbg_out("here:%x prio:%d\n",menuitem,prio);

  if ( (!menuitem) || (!( prio_is_valid(prio))) )
    return -EINVAL;

  window=lookup_widget(GTK_WIDGET(menuitem),"messageWindow");
  g_assert(window);
  view=GTK_TREE_VIEW(lookup_widget(GTK_WIDGET(window),"messageUserTree"));
  g_assert(view);
    
  sel = gtk_tree_view_get_selection(view);

  pathes=gtk_tree_selection_get_selected_rows(sel,&model);
  for(node=g_list_first(pathes);node;node=g_list_next(node)) {
    g_assert( (node) && (node->data));

    model=gtk_tree_view_get_model(view);
    gtk_tree_model_get_iter(model,&iter,node->data);
    userview_set_view_priority(model,node->data,&iter,prio);
  }
  g_list_foreach (pathes, (GFunc)gtk_tree_path_free, NULL);
  g_list_free (pathes);
}

static void
do_get_client_info(GtkTreeModel *model,
	       GtkTreePath *path,
	       GtkTreeIter *iter,
	       gpointer data){
  gchar *ipaddr;
  int rc;
  int lflags;
  unsigned long cmd_no;

  cmd_no=(unsigned long)data;

  gtk_tree_model_get (model, iter, 
		      USER_VIEW_IPADDR_ID, &ipaddr,
		      -1);

  lflags=hostinfo_get_normal_entry_flags();

  dbg_out("Send %s request to %s from ui\n",
	  (cmd_no == IPMSG_GETINFO)?("get version"):("get absence info"),
	  ipaddr);

  rc=ipmsg_send_get_info_msg(udp_con,ipaddr,cmd_no);
  if (rc)
    ipmsg_err_dialog(_("Can not require %s  to %s"),
	      (cmd_no == IPMSG_GETINFO)?("get version"):("get absence info"),
	      ipaddr);

  g_free(ipaddr);
}

void
on_set_priority1_activate              (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  dbg_out("here\n");
  update_prioriy_for_selection(menuitem,1);
}


void
on_set_priority_as_2_activate          (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  dbg_out("here\n");
  update_prioriy_for_selection(menuitem,2);
}


void
on_set_priority_as_3_activate          (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  dbg_out("here\n");
  update_prioriy_for_selection(menuitem,3);
}


void
on_set_priority_as_4_activate          (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  dbg_out("here\n");
  update_prioriy_for_selection(menuitem,4);
}


void
on_set_as_default_activate             (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  dbg_out("here\n");
  update_prioriy_for_selection(menuitem,0);
}


void
on_set_invisible_item_activate         (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  dbg_out("here\n");
  update_prioriy_for_selection(menuitem,-1);
}


void
on_show_invisible_items1_activate      (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  GtkWidget *window;

  if  (!menuitem) 
    return;

  window=lookup_widget(GTK_WIDGET(menuitem),"messageWindow");
  g_assert(window);

    
  update_users_on_message_window(window, TRUE);
}


void
on_set_all_as_default_activate         (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  GtkWidget *window;
  GtkTreeView *view;
  GtkTreeSelection *sel;
  GtkTreeModel *model;
  GtkTreeIter iter;
  gboolean valid;


  if  (!menuitem) 
    return;

  window=lookup_widget(GTK_WIDGET(menuitem),"messageWindow");
  g_assert(window);
  view=GTK_TREE_VIEW(lookup_widget(GTK_WIDGET(window),"messageUserTree"));
  g_assert(view);
    
  model=gtk_tree_view_get_model(view);
  valid=gtk_tree_model_get_iter_first (model,&iter);
  while(valid){
    userview_set_view_priority_without_update(model, NULL, &iter, USERLIST_DEFAULT_PRIO);
    valid = gtk_tree_model_iter_next (model, &iter);
  }
  update_all_user_list_view();
}


void
on_groupselection_activate             (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{

}


void
on_attachmentfile_activate             (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  GtkWidget *window;
  GtkWidget *attachment_editor;

  window=lookup_widget(GTK_WIDGET(menuitem),"messageWindow");
  g_assert(window);

  attachment_editor=setup_attachment_editor(window);
  g_assert(attachment_editor);
  gtk_widget_show(attachment_editor);
}


void
on_set_user_list_view_config_activate  (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  GtkWidget *window;

  dbg_out("here\n");

  window=internal_create_viewConfigWindow ();
  gtk_widget_show(window);
}


void
on_groupInverseBtn_toggled             (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{

}


void
on_secondKeyInverseBtn_toggled         (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{

}


void
on_save_list_headers_state1_activate   (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  GtkWidget *window;
  GtkWidget *view;
  GList *cols,*node;
  int count;
  dbg_out("here\n");

  window=lookup_widget(GTK_WIDGET(menuitem),"messageWindow");

  remind_headers_state(window);
}

void
on_viewConfigCloseBtn_clicked          (GtkButton       *button,
                                        gpointer         user_data)
{
  GtkWidget *window;

  dbg_out("here\n");

  window=lookup_widget(GTK_WIDGET(button),"viewConfigWindow");
  
  gtk_widget_destroy(GTK_WIDGET(window));
}


void
on_get_version1_activate               (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  GtkWidget *top;
  GtkTreeView *view;
  GtkTreeSelection *sel;

  dbg_out("here\n");

  top=lookup_widget(GTK_WIDGET(menuitem),"messageWindow");
  view=GTK_TREE_VIEW(lookup_widget(GTK_WIDGET(menuitem),"messageUserTree"));

  g_assert(view);
  g_assert(top);
    
  sel = gtk_tree_view_get_selection(view);
  gtk_tree_selection_selected_foreach(sel,do_get_client_info,(gpointer)IPMSG_GETINFO);
}


void
on_absence_info1_activate              (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  dbg_out("here\n");

  GtkWidget *top;
  GtkTreeView *view;
  GtkTreeSelection *sel;

  dbg_out("here\n");

  top=lookup_widget(GTK_WIDGET(menuitem),"messageWindow");
  view=GTK_TREE_VIEW(lookup_widget(GTK_WIDGET(menuitem),"messageUserTree"));

  g_assert(view);
  g_assert(top);
    
  sel = gtk_tree_view_get_selection(view);
  gtk_tree_selection_selected_foreach(sel,do_get_client_info,(gpointer)IPMSG_GETABSENCEINFO);
}


void
on_clientInfoOKBtn_clicked             (GtkButton       *button,
                                        gpointer         user_data)
{
  GtkWidget *window;

  window=lookup_widget(GTK_WIDGET(button),"clientInfoWindow");
  g_assert(window);
  gtk_widget_destroy(GTK_WIDGET(window));  
}


void
on_sort_filter1_activate               (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  dbg_out("here\n");
}


void
on_menu_set_priority_as_1_activate     (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  on_set_priority1_activate(menuitem,user_data);
}


void
on_menu_set_priority_as_2_activate     (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  on_set_priority_as_2_activate(menuitem,user_data);
}


void
on_menu_set_priority_as_3_activate     (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  on_set_priority_as_3_activate(menuitem,user_data);
}


void
on_menu_set_priority_as_4_activate     (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  on_set_priority_as_4_activate(menuitem,user_data);
}


void
on_menu_set_them_as_default_activate   (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  on_set_as_default_activate(menuitem,user_data);
}


void
on_menu_set_them_invisible_item_activate
                                        (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  on_set_invisible_item_activate(menuitem,user_data);
}


void
on_menu_show_invisible_items_activate  (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  on_show_invisible_items1_activate(menuitem,user_data);
}


void
on_menu_set_all_as_default_activate    (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  on_set_all_as_default_activate(menuitem,user_data);
}


void
on_menu_user_list_view_config_activate (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  on_set_user_list_view_config_activate(menuitem,user_data);
}


void
on_menu_save_list_headers_state_activate
                                        (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  on_save_list_headers_state1_activate(menuitem,user_data);
}


void
on_pubkeyPWDBtn_clicked                (GtkButton       *button,
                                        gpointer         user_data)
{
  password_setting_window(HOSTINFO_PASSWD_TYPE_ENCKEY);
}


void
on_lockPWDBtn_clicked                  (GtkButton       *button,
                                        gpointer         user_data)
{
  password_setting_window(HOSTINFO_PASSWD_TYPE_LOCK);
}


void
on_configSecurityOKBtn_clicked         (GtkButton       *button,
                                        gpointer         user_data)
{
  GtkWidget *window;

  window=lookup_widget(GTK_WIDGET(button),"securityConfig");
  g_assert(window);

  apply_crypt_config_window(GTK_WINDOW(window));

  gtk_widget_destroy(GTK_WIDGET(window));    
}


void
on_configSecurityCancelBtn_clicked     (GtkButton       *button,
                                        gpointer         user_data)
{
  GtkWidget *window;

  window=lookup_widget(GTK_WIDGET(button),"securityConfig");
  g_assert(window);
  gtk_widget_destroy(GTK_WIDGET(window));
}


void
on_passwordEntry_activate              (GtkEntry        *entry,
                                        gpointer         user_data)
{

}


void
on_passWordDialogCancelBtn_clicked     (GtkButton       *button,
                                        gpointer         user_data)
{
}


void
on_passWordDialogCOKBtn_clicked        (GtkButton       *button,
                                        gpointer         user_data)
{
}

void
on_decodeFailDialogOKButton_clicked    (GtkButton       *button,
                                        gpointer         user_data)
{
  GtkWidget *top;

  dbg_out("here\n");
  top=lookup_widget(GTK_WIDGET(button),"decodeFaildialog");
  g_assert(top);
  gtk_widget_destroy(top);
}


void
on_errorDialogCloseBtn_pressed         (GtkButton       *button,
                                        gpointer         user_data)
{
  GtkWidget *top;

  dbg_out("here\n");
  top=lookup_widget(GTK_WIDGET(button), "errorDialog");
  g_assert(top);
  gtk_widget_destroy(top);
}


void
on_menu_security_configuration_activate(GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  GtkWidget *configWindow = NULL;

  configWindow = internal_create_crypt_config_window();
  gtk_widget_show(configWindow);
}


void
on_passWordOKBtn_clicked               (GtkButton       *button,
                                        gpointer         user_data)
{
	GtkWidget                          *top = NULL;
	GtkEntry                    *new_passwd = NULL;
	const char           *new_passwd_string = NULL;
	ipmsg_private_data_t              *priv = NULL;
	char                          *enc_pass = NULL;
	int                                type = 0;
	int                                  rc = 0;

	dbg_out("here\n");

	top = lookup_widget(GTK_WIDGET(button), "passwdWindow");
	g_assert(top);

	new_passwd = GTK_ENTRY(lookup_widget(top, "passwdEntry1"));
	g_assert(new_passwd != NULL);

	new_passwd_string = 
		(char *)gtk_entry_get_text(GTK_ENTRY(new_passwd));
	g_assert(new_passwd_string != NULL);

	priv = 
		(ipmsg_private_data_t *)lookup_widget(top, "passWordType");
	g_assert(priv != NULL);

	dbg_out("Private type=%d\n", (int)priv->data);
	IPMSG_ASSERT_PRIVATE(priv, IPMSG_PRIVATE_PASSWD);

	/*
	 * パスワードをエンコードする
	 */
	if (new_passwd_string[0] != '\0') {
		rc = pbkdf2_encrypt(new_passwd_string, NULL, &enc_pass);
		if (rc != 0)
			goto error_out;  /* Open SSLを使用しない場合等でありうる  */
	} else {
		/* 空文の場合は, 設定解除用に空文を設定する  */
		enc_pass = g_strdup("");
		if (enc_pass == NULL)
			goto error_out;  /* Open SSLを使用しない場合等でありうる  */
	}

	/*
	 * エンコードしたパスワードを登録する
	 */
	type = (int)priv->data;

	switch(type) {
	case HOSTINFO_PASSWD_TYPE_ENCKEY:
		dbg_out("Register encrypt key password:%s\n", enc_pass);
		hostinfo_set_encryption_key_password((const char *)enc_pass);
		break;
	case HOSTINFO_PASSWD_TYPE_LOCK:
		dbg_out("Register lock password:%s\n", enc_pass);
		hostinfo_set_lock_password((const char *)enc_pass);
		break;
	default:
		g_assert_not_reached();
		break;
	}
	
 error_out:
  if (enc_pass != NULL)
    g_free(enc_pass);

  gtk_widget_destroy(top);  /*  private情報は, 自動的に破棄される  */
}


void
on_PassWordCancelBtn_clicked           (GtkButton       *button,
                                        gpointer         user_data)
{
  GtkWidget                *top = NULL;

  dbg_out("here\n");

  top = lookup_widget(GTK_WIDGET(button), "passwdWindow");
  g_assert(top);


  gtk_widget_destroy(top);
}

static int
password_window_passwd_changed(GtkEditable     *editable) {
	GtkWindow                     *window = NULL;
	GtkButton                  *ok_button = NULL;
	GtkEntry              *current_passwd = NULL;
	GtkEntry                  *new_passwd = NULL;
	GtkEntry            *confirmed_passwd = NULL;
	char           *current_passwd_string = NULL;
	char               *new_passwd_string = NULL;
	char           *confirm_passwd_string = NULL;
	const char *configured_encoded_passwd = NULL;
	ipmsg_private_data_t            *priv = NULL;
	int                           type = 0;
	int                             rc = 0;

	dbg_out("here\n");

	g_assert(editable != NULL);

	window = 
		GTK_WINDOW(lookup_widget(GTK_WIDGET(editable), "passwdWindow"));
	g_assert(window != NULL);

	/*
	 * ウィジェット獲得
	 */
	current_passwd = 
		GTK_ENTRY(lookup_widget(GTK_WIDGET(window), 
			"currentPassWordEntry"));
	g_assert(current_passwd != NULL);

	new_passwd = 
		GTK_ENTRY(lookup_widget(GTK_WIDGET(window), 
			"passwdEntry1"));
	g_assert(new_passwd != NULL);

	confirmed_passwd = 
		GTK_ENTRY(lookup_widget(GTK_WIDGET(window), 
			"passwdEntry2"));
	g_assert(confirmed_passwd != NULL);

	ok_button = 
		GTK_BUTTON(lookup_widget(GTK_WIDGET(window), 
			"passWordOKBtn"));

	current_passwd_string =
		(char *)gtk_entry_get_text(GTK_ENTRY(current_passwd));
	g_assert(current_passwd_string != NULL);

	new_passwd_string = 
		(char *)gtk_entry_get_text(GTK_ENTRY(new_passwd));
	g_assert(new_passwd_string != NULL);

	confirm_passwd_string =
		(char *)gtk_entry_get_text(GTK_ENTRY(confirmed_passwd));
	g_assert(confirm_passwd_string != NULL);

	priv = 
		(ipmsg_private_data_t *)lookup_widget(GTK_WIDGET(window),"passWordType");
	g_assert(priv != NULL);

	dbg_out("Private type=%d\n", (int)priv->data);
	IPMSG_ASSERT_PRIVATE(priv, IPMSG_PRIVATE_PASSWD);

	/*
	 * エンコードされたパスワードを取得する
	 */
	type = (int)priv->data;

	switch(type) {
	case HOSTINFO_PASSWD_TYPE_ENCKEY:
		configured_encoded_passwd = hostinfo_refer_encryption_key_password();
		break;
	case HOSTINFO_PASSWD_TYPE_LOCK:
		configured_encoded_passwd = hostinfo_refer_lock_password();
		break;
	default:
		g_assert_not_reached();
		break;
	}

	rc = -EPERM;
	/* 
	 * 入力したパスワードが一致していて, かつ, 現在パスワードが未設定かまたは,
	 * 現在のパスワードを正確に入力していた場合は,
	 * OKボタンを有効にする.
	 * セキュリティ上は, あまり好ましくない実装だが, 
	 * 初心者にはこのほうがよいと判断した.
	 */
	if ( (g_utf8_collate(new_passwd_string, confirm_passwd_string) == 0) &&
	    ( (pbkdf2_encoded_passwd_configured(configured_encoded_passwd)) || 
	       (pbkdf2_verify(current_passwd_string, configured_encoded_passwd) == 0) ) )
		rc = 0;

	if (rc == 0)
		gtk_widget_set_sensitive(GTK_WIDGET(ok_button), TRUE);
	else	  
		gtk_widget_set_sensitive(GTK_WIDGET(ok_button), FALSE);

	return rc;
}
void
on_passwdEntry1_changed                (GtkEditable     *editable,
                                        gpointer         user_data) {
	dbg_out("here\n");
	password_window_passwd_changed(editable);
}


void
on_passwdEntry2_changed                (GtkEditable     *editable,
                                        gpointer         user_data) {
	dbg_out("here\n");
	password_window_passwd_changed(editable);
}


void
on_currentPassWordEntry_changed        (GtkEditable     *editable,
                                        gpointer         user_data)
{
	dbg_out("here\n");
	password_window_passwd_changed(editable);
}

void
on_RSAKeyEncryptionCBtn_toggled        (GtkToggleButton *togglebutton,
                                        gpointer         user_data){
	update_rsa_encryption_button_state(togglebutton);
}


void
on_useLockCBtn_toggled                 (GtkToggleButton *togglebutton,
                                        gpointer         user_data){
	update_lockkey_button_state(togglebutton);
}


void
on_lockChkBtn_toggled                  (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{

}


void
on_encloseCheckBtn_toggled             (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
  GtkToggleButton *enclose_toggle = NULL;
  GtkToggleButton    *lock_toggle = NULL;

  dbg_out("here\n");

  enclose_toggle = 
	  GTK_TOGGLE_BUTTON(lookup_widget(GTK_WIDGET(togglebutton),"encloseCheckBtn"));
  g_assert(enclose_toggle != NULL);

  lock_toggle = 
	  GTK_TOGGLE_BUTTON(lookup_widget(GTK_WIDGET(togglebutton), "lockChkBtn"));
  g_assert(lock_toggle != NULL);

  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(enclose_toggle))) {
	  gtk_widget_set_sensitive(GTK_WIDGET(lock_toggle), TRUE);
  } else {
	  gtk_widget_set_sensitive(GTK_WIDGET(lock_toggle), FALSE);
	  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(lock_toggle), FALSE);
 }

}


gboolean
on_readNotifyDialog_window_state_event (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
  dbg_out("Here\n");
  return FALSE;
}



void
on_enableLogToggle_toggled             (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{

}


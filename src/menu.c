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

#define GLADE_HOOKUP_OBJECT_NO_REF(component,widget,name) \
  g_object_set_data (G_OBJECT (component), name, widget)

#define GLADE_HOOKUP_OBJECT(component,widget,name) \
  g_object_set_data_full (G_OBJECT (component), name, \
    gtk_widget_ref (widget), (GDestroyNotify) gtk_widget_unref)

static GList *monitor_wins=NULL;
GStaticMutex monitor_win_mutex = G_STATIC_MUTEX_INIT;

static void 
light_weight_flush (void)
{
  GdkDisplay *display = gdk_display_get_default ();

  XFlush (GDK_DISPLAY_XDISPLAY (display));
}

GtkWidget *
create_menu_item(const char *name,const char *label,gpointer data,
		 void (*menu_item_callback_function)
		 (gpointer user_data)){
  GtkWidget *new_item;
  int index=(int)data;

  dbg_out("Create item with:%d\n",index);
  new_item=gtk_menu_item_new_with_label(label);
  if (menu_item_callback_function){
    dbg_out("connect:data %x\n",(unsigned int)data);
    if (data)
      gtk_signal_connect_object( (gpointer)new_item, "activate",
				 GTK_SIGNAL_FUNC(menu_item_callback_function), 
				 data);
    else
      gtk_signal_connect_object( (gpointer)new_item, "activate",
				 GTK_SIGNAL_FUNC(menu_item_callback_function), 
				 (gpointer )new_item);
  }
  gtk_widget_show(new_item);

  return new_item;
}
static GtkWidget *
create_stock_menu_item(const char *name,const gchar *stock_id,gpointer data,
		 void (*menu_item_callback_function)
		 (gpointer user_data)){
  GtkWidget *new_item;

  new_item=gtk_image_menu_item_new_from_stock(stock_id,NULL);
  if (menu_item_callback_function) {
    if (data)
      gtk_signal_connect_object( GTK_OBJECT(new_item), "activate",
				 GTK_SIGNAL_FUNC(menu_item_callback_function), 
				 data);
    else
      gtk_signal_connect_object( GTK_OBJECT(new_item), "activate",
				 GTK_SIGNAL_FUNC(menu_item_callback_function), 
				 new_item);
  }
  gtk_widget_show(new_item);

  return new_item;
}
static GtkWidget *
create_menu_separator(const char *name){
  GtkWidget *new_item;

  new_item=gtk_separator_menu_item_new ();
  gtk_widget_show(new_item);

  return new_item;
}
static void
onDownloadMonitorSelectionChanged (GtkTreeSelection *sel, GtkListStore *liststore) {
  update_download_monitor_button(sel);
}
void
update_download_monitor_button(GtkTreeSelection *sel) {
  GtkTreeIter  selected_row;
  GtkWidget    *view;
  GtkWidget    *remove_btn;

  view=GTK_WIDGET(gtk_tree_selection_get_tree_view(sel));
  g_assert(view);
  remove_btn=lookup_widget(GTK_WIDGET(view),"deleteBtn");
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
setup_download_window(GtkWidget *window){
  GtkWidget *view;
  GtkListStore        *liststore;
  GtkTreeViewColumn   *col;
  GtkCellRenderer     *renderer;
  GtkTreeSelection    *sel;

  dbg_out("here\n");
  g_assert(window);
  view=lookup_widget(GTK_WIDGET(window),"treeview5");
  g_assert(view);
  dbg_out("Create new monitor\n");

  liststore = gtk_list_store_new(4, 
				 G_TYPE_STRING,
				 G_TYPE_UINT,
				 G_TYPE_STRING,
				 G_TYPE_INT);
  g_assert(liststore);
  gtk_tree_view_set_model(GTK_TREE_VIEW(view), GTK_TREE_MODEL(liststore));
  g_object_unref(liststore);

  /*
   * タイトル設定
   */
  /* --- Column #1 --- */

  col = gtk_tree_view_column_new();

  gtk_tree_view_column_set_title(col, _("FileNames"));

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

  gtk_tree_view_column_set_title(col, _("Remains"));

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

  gtk_tree_view_column_set_title(col, _("Receivers"));

  /* pack tree view column into tree view */
  gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);

  renderer = gtk_cell_renderer_text_new();

  /* pack cell renderer into tree view column */
  gtk_tree_view_column_pack_start(col, renderer, TRUE);

  /* connect 'text' property of the cell renderer to
   *  model column that contains the first name */
  gtk_tree_view_column_add_attribute(col, renderer, "text", 2);

  gtk_tree_view_column_set_resizable (col,TRUE);

  sel=gtk_tree_view_get_selection(GTK_TREE_VIEW(view));
  gtk_tree_selection_set_mode(sel, GTK_SELECTION_SINGLE);
  g_signal_connect(sel, "changed", 
		   G_CALLBACK(onDownloadMonitorSelectionChanged),
		   liststore);
}
void
on_create_download_monitor(void) {
  GtkWidget *window;

  dbg_out("here\n");

  g_static_mutex_lock(&monitor_win_mutex);
  if (g_list_length(monitor_wins)>0) {
    GList *node; /* FIXME */

    node=g_list_first (monitor_wins);
    g_assert(node->data);
    gtk_window_present (GTK_WINDOW(node->data));
    g_static_mutex_unlock(&monitor_win_mutex);
    return; 
  }
    
  window=create_downloadMonitor();
  g_assert(window);
  setup_download_window(window);

  g_static_mutex_unlock(&monitor_win_mutex);
  download_monitor_add_waiter_window(window);

  gtk_widget_show(window);
}
void
on_mainmenu_new_message_item(gpointer menuitem){
  dbg_out("here\n");
  on_startBtn_clicked (NULL,NULL);
}
void
on_mainmenu_download_item(gpointer menuitem){
  on_create_download_monitor();
}
void
on_mainmenu_preferences_item(gpointer menuitem){
  dbg_out("here\n");
  on_preferences1_activate(menuitem,NULL);
}
void
on_mainmenu_config_security_item(gpointer menuitem){
  dbg_out("here\n");
  on_menu_security_configuration_activate(menuitem,NULL);
}
void
on_mainmenu_about_item(gpointer menuitem){
  dbg_out("here\n");

  ipmsg_show_about_dialog();
}
void
on_mainmenu_log_item(gpointer menuitem){
  dbg_out("here\n");
  show_ipmsg_log();
}
void
on_mainmenu_absent_item(gpointer menuitem){
  dbg_out("here\n");
}
void
on_mainmenu_attend_item(gpointer menuitem){
  dbg_out("here\n");
  hostinfo_set_ipmsg_absent(FALSE);
  ipmsg_send_br_absence(udp_con,0);
}
void
on_mainmenu_foreground_win_item(gpointer menuitem){
  dbg_out("here\n");
  present_all_displayed_windows();
}
void
on_mainmenu_remove_win_item(gpointer menuitem){
  dbg_out("here\n");
  destroy_all_opened_windows();
}
void
on_mainmenu_quit_item (gpointer menuitem){
  dbg_out("here\n");
  on_initialWindow_destroy(GTK_OBJECT(menuitem),NULL);
}
void
on_fuzai_item_activate (gpointer user_data){
  int max_index;
  int index;

  index=(int)user_data - 1;
  dbg_out("here:%x\n",index);
  if (hostinfo_refer_absent_length(&max_index)) {
    g_assert_not_reached();
  }
  dbg_out("Max index:%d\n",max_index);
  if ( (index >= max_index) || (index < 0) )
    return;
  hostinfo_set_absent_id(index);
  hostinfo_set_ipmsg_absent(TRUE);
  ipmsg_send_br_absence(udp_con,0);
  return;
}
void
on_fuzai_config_activate (gpointer menuitem){
  GtkWidget *window;
  dbg_out("here:%x\n",(unsigned int)menuitem);
  window=internal_create_fuzai_editor();
  gtk_widget_show(window);
}

static GtkWidget *
create_fuzai_menu(void){
  GtkWidget *menu;
  GtkWidget *new_item;
  GtkWidget *separator4_item;
  GtkWidget *config_item;
  gchar name[16];
  gchar *title;
  int i,max_index=0;

  menu=gtk_menu_new(); 
  if (hostinfo_refer_absent_length(&max_index)) {
    g_assert_not_reached();
  }
  dbg_out("Max index:%d\n",max_index);
  for(i=0;i<max_index;++i) {
    snprintf(name,15,"fuzai%d",i);
    name[15]='\0';
    title=NULL;
    hostinfo_get_absent_title(i,(const char **)&title);
    /*  埋め込みのindexは, NULLと区別するため, +1する */
    if (title) {
      new_item=create_menu_item(name,title,(gpointer)(i+1),on_fuzai_item_activate);
      g_free(title);
    }else
      new_item=create_menu_item(name,name,(gpointer)(i+1),on_fuzai_item_activate);
    gtk_menu_append( GTK_MENU(menu), new_item);
    GLADE_HOOKUP_OBJECT(menu,new_item,name);
  }
  separator4_item=create_menu_separator("separator4");

  config_item=create_stock_menu_item("fuzai_config_item",GTK_STOCK_PREFERENCES,NULL,on_fuzai_config_activate);

  gtk_menu_append( GTK_MENU(menu), separator4_item);
  gtk_menu_append( GTK_MENU(menu), config_item);

  GLADE_HOOKUP_OBJECT(menu,separator4_item,"separator4_item");
  GLADE_HOOKUP_OBJECT(menu,config_item,"fuzai_config_item");

  return menu;
}
GtkWidget *
create_main_menu(void){
  GtkWidget *menu;
  GtkWidget *new_message_item;
  GtkWidget *separator0_item;
  GtkWidget *download_item;
  GtkWidget *separator1_item;
  GtkWidget *remove_win_item;
  GtkWidget *foreground_win_item;
  GtkWidget *separator2_item;
  GtkWidget *preferences_item;
  GtkWidget *config_security_item;
  GtkWidget *about_item;
  GtkWidget *log_item;
  GtkWidget *separator3_item;
  GtkWidget *absent_item;
  GtkWidget *attend_item;
  GtkWidget *separator4_item;
  GtkWidget *quit_item;

  menu = gtk_menu_new(); 
  new_message_item = 
    create_menu_item("new_message_item", _("_New Message"), NULL, 
		     on_mainmenu_new_message_item);
  separator0_item = create_menu_separator("separator0");
  download_item = create_menu_item("download_item", _("DownLoadMonitor"), NULL,
				   on_mainmenu_download_item);

  separator1_item = create_menu_separator("separator1");
  foreground_win_item = create_menu_item("foreground_win_item", 
					 _("Foreground all message windows"), 
					 NULL, on_mainmenu_foreground_win_item);
  remove_win_item = create_menu_item("remove_win_item",
				     _("Remove opend windows"), NULL, 
				     on_mainmenu_remove_win_item);

  separator2_item = create_menu_separator("separator2");

  preferences_item = create_stock_menu_item("preferences_item", 
					    GTK_STOCK_PREFERENCES, NULL, 
					    on_mainmenu_preferences_item);

  config_security_item = create_menu_item("config_security_item", 
					  _("security configuration"), NULL, 
					  on_mainmenu_config_security_item);

  about_item = create_stock_menu_item("about_item", 
				      GTK_STOCK_ABOUT, NULL, 
				      on_mainmenu_about_item);

  log_item = create_menu_item("log_item", _("Show Log"), NULL, 
			      on_mainmenu_log_item);

  separator3_item = create_menu_separator("separator3");

  absent_item = create_menu_item("absent_item", _("Absence"), 
				 NULL, on_mainmenu_absent_item);

  gtk_menu_item_set_submenu(GTK_MENU_ITEM (absent_item), create_fuzai_menu());

  if (hostinfo_is_ipmsg_absent())
    attend_item = create_menu_item("attend_item", _("Attend"), NULL, 
				   on_mainmenu_attend_item);

  separator4_item = create_menu_separator("separator4");
  quit_item = create_stock_menu_item("quit_item", GTK_STOCK_QUIT, 
				     NULL, on_mainmenu_quit_item);

  gtk_menu_append( GTK_MENU(menu), new_message_item);
  gtk_menu_append( GTK_MENU(menu), separator0_item);
  gtk_menu_append( GTK_MENU(menu), download_item);
  gtk_menu_append( GTK_MENU(menu), separator1_item);
  gtk_menu_append( GTK_MENU(menu), remove_win_item);
  gtk_menu_append( GTK_MENU(menu), foreground_win_item);
  gtk_menu_append( GTK_MENU(menu), separator2_item);
  gtk_menu_append( GTK_MENU(menu), preferences_item);
  gtk_menu_append( GTK_MENU(menu), config_security_item);
  gtk_menu_append( GTK_MENU(menu), about_item);
  gtk_menu_append( GTK_MENU(menu), log_item);
  gtk_menu_append( GTK_MENU(menu), separator3_item);

  gtk_menu_append( GTK_MENU(menu), absent_item);
  if (hostinfo_is_ipmsg_absent())
    gtk_menu_append( GTK_MENU(menu), attend_item);

  gtk_menu_append( GTK_MENU(menu), separator4_item);

  gtk_menu_append( GTK_MENU(menu), quit_item);

  GLADE_HOOKUP_OBJECT(menu, new_message_item, "new_message_item");
  GLADE_HOOKUP_OBJECT(menu, separator0_item, "separator0_item");
  GLADE_HOOKUP_OBJECT(menu, download_item, "download_item");
  GLADE_HOOKUP_OBJECT(menu, separator1_item, "separator1_item");
  GLADE_HOOKUP_OBJECT(menu, remove_win_item, "remove_win_item");
  GLADE_HOOKUP_OBJECT(menu, foreground_win_item, "foreground_win_item");
  GLADE_HOOKUP_OBJECT(menu, separator1_item, "separator2_item");
  GLADE_HOOKUP_OBJECT(menu, preferences_item, "preferences_item");
  GLADE_HOOKUP_OBJECT(menu, config_security_item,  "config_security_item");
  GLADE_HOOKUP_OBJECT(menu, about_item, "about_item");
  GLADE_HOOKUP_OBJECT(menu, log_item, "log_item");
  GLADE_HOOKUP_OBJECT(menu, separator3_item, "separator3_item");
  
  GLADE_HOOKUP_OBJECT(menu, absent_item, "absent_item");

  if (hostinfo_is_ipmsg_absent())
    GLADE_HOOKUP_OBJECT(menu, attend_item, "attend_item");

  GLADE_HOOKUP_OBJECT(menu, separator4_item, " separator4_item");

  GLADE_HOOKUP_OBJECT(menu, quit_item, "quit_item");

  return menu;
}
gboolean
on_init_win_event_button_press_event (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
	GtkWidget	*fuzaiMenu;

	dbg_out("buttonWin: button press: %d\n", event->button);
	switch (event->button) {
	case 3:
	  gtk_menu_popup(GTK_MENU(create_main_menu()), NULL, NULL, NULL, NULL,
			       event->button, event->time);

	default:					/* main menu */
	  return FALSE;
	  break;
	}
	return TRUE;
}

void
ipmsg_show_about_dialog(void){
  GdkPixbuf *aboutDialog_icon_pixbuf;
  GtkWidget *aboutDialog;
  const gchar *authors[]={
    G2IPMSG_AUTHOR,
    NULL
  };

  dbg_out("here\n");

  aboutDialog=create_aboutdialog ();
  g_assert(aboutDialog);

  gtk_about_dialog_set_authors(GTK_ABOUT_DIALOG(aboutDialog),(const gchar **)authors);
  gtk_about_dialog_set_translator_credits (GTK_ABOUT_DIALOG(aboutDialog),G2IPMSG_TRANSLATOR);
  gtk_about_dialog_set_copyright (GTK_ABOUT_DIALOG(aboutDialog),_("Takeharu KATO"));
  aboutDialog_icon_pixbuf = create_pixbuf ("g2ipmsg/ipmsg.xpm");
  if (aboutDialog_icon_pixbuf)
    {
      gtk_window_set_icon (GTK_WINDOW (aboutDialog), aboutDialog_icon_pixbuf);
      gdk_pixbuf_unref (aboutDialog_icon_pixbuf);
    }

  gtk_widget_show (aboutDialog);

}
static void
do_update_monitor_win(gpointer data,gpointer user_data) {
  GtkWidget *window;
  GtkWidget *view;
  GtkTreeSelection *sel;

  if (!data)
    return;

  window=GTK_WIDGET(data);
 
  view=lookup_widget(GTK_WIDGET(window),"treeview5");
  g_assert(view);

  update_download_view(window);  
  sel=gtk_tree_view_get_selection(GTK_TREE_VIEW(view));
  update_download_monitor_button(sel);

  return;
}
int 
download_monitor_delete_btn_action(GtkButton       *button,
				   gpointer         user_data){
  GtkWidget *window;  
  GtkWidget *view;
  GtkTreeSelection *sel;
  GtkTreeIter  selected_row;
  GtkTreeModel *model;
  pktno_t pkt_no;

  dbg_out("here\n");

  g_static_mutex_lock(&monitor_win_mutex);

  window=lookup_widget(GTK_WIDGET(button),"downloadMonitor");
  view=lookup_widget(GTK_WIDGET(window),"treeview5");
  g_assert(view);
  sel=gtk_tree_view_get_selection(GTK_TREE_VIEW(view));

  if (gtk_tree_selection_get_selected(sel, &model, &selected_row)) {
    gtk_tree_model_get(model, &selected_row, 
		       3, &pkt_no,
		       -1);
    dbg_out("Relase:%ld\n",pkt_no);
    release_attach_file_block(pkt_no,TRUE);
    gtk_list_store_remove(GTK_LIST_STORE(model), &selected_row);
  }
  download_monitor_update_state();

  sel=gtk_tree_view_get_selection(GTK_TREE_VIEW(view));

  update_download_monitor_button(sel);

  g_static_mutex_unlock(&monitor_win_mutex);
}
int 
download_monitor_update_state(void){

  g_static_mutex_lock(&monitor_win_mutex);
  g_list_foreach(monitor_wins,
		 do_update_monitor_win,
		 NULL);  
  g_static_mutex_unlock(&monitor_win_mutex);

}
/*
 *スレッド経由でのリリースファイル
 */
void
download_monitor_release_file(const pktno_t pktno,int fileid){

  g_static_mutex_lock(&monitor_win_mutex);

  release_attach_file(pktno,fileid);

  gdk_threads_enter();
    g_list_foreach(monitor_wins,
		 do_update_monitor_win,
		 NULL);  
  gdk_threads_leave();

  g_static_mutex_unlock(&monitor_win_mutex);
}
/*
 *スレッド経由でのリリースファイル
 */
int
download_monitor_refer_file(const  pktno,int fileid,unsigned long *ipmsg_fattr,const char **path, off_t *size){
  int rc;

  g_static_mutex_lock(&monitor_win_mutex);

  if (g_list_length(monitor_wins)>0) {
    rc=refer_attach_file(pktno,fileid,ipmsg_fattr,path,size);
  }

  g_static_mutex_unlock(&monitor_win_mutex);

  return rc;
}

int
download_monitor_add_waiter_window(GtkWidget *window){
  if (!window)
    return -EINVAL;

  dbg_out("here %x\n",(unsigned int)window);

  g_static_mutex_lock(&monitor_win_mutex);
  monitor_wins=g_list_append(monitor_wins,(gpointer)window);
  g_static_mutex_unlock(&monitor_win_mutex);

  return 0;
}
int
download_monitor_remove_waiter_window(GtkWidget *window){
  if (!window)
    return -EINVAL;

  dbg_out("here %x\n",(unsigned int)window);

  g_static_mutex_lock(&monitor_win_mutex);
  monitor_wins=g_list_remove(monitor_wins,(gpointer)window);
  g_static_mutex_unlock(&monitor_win_mutex);

  return 0;
}
int 
show_ipmsg_log(void){
  int rc;
  gchar *logfile;
  GnomeVFSMimeApplication *app;
  GList params;
  gchar *url;

  dbg_out("here\n");

  rc=0;
  if (!hostinfo_refer_ipmsg_enable_log())
    goto error_out;

  rc=-ENOMEM;
  if (g_path_is_absolute ((gchar *)hostinfo_refer_ipmsg_logfile()))
    logfile=g_strdup((gchar *)hostinfo_refer_ipmsg_logfile());
  else {
    gchar *current_dir;
   current_dir=g_get_current_dir();
   if (!current_dir)
     goto error_out;
   logfile=g_build_filename (current_dir,hostinfo_refer_ipmsg_logfile(),NULL);
   g_free(current_dir);
   dbg_out("Absolute path:%s\n",logfile);
  }
  if (!logfile)
    goto error_out;

  url=gnome_vfs_get_uri_from_local_path(logfile);
  if (!url)
    goto free_log_file;

  params.data = (char *) url;
  params.prev = NULL;
  params.next = NULL;

  rc=-ENOENT;
  app=gnome_vfs_mime_get_default_application("text/plain");
  if (!app)
    goto free_url;

  gnome_vfs_mime_application_launch(app, &params);
  gnome_vfs_mime_application_free (app);

  rc=0;

 free_url:
  g_free(url);
 free_log_file:
  g_free(logfile);
    
 error_out:
  return rc;
}

GtkWidget*
internal_create_viewConfigWindow (void){
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
  uint state;


  state=hostinfo_refer_header_state();

  window=create_viewConfigWindow();

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

  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(viewConfigGroupChkBtn),(state&HEADER_VISUAL_GROUP_ID));
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(viewConfigHostChkBtn),(state&HEADER_VISUAL_HOST_ID));
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(viewConfigIPAddrChkBtn),(state&HEADER_VISUAL_IPADDR_ID));
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(viewConfigLogonChkBtn),(state&HEADER_VISUAL_LOGON_ID));
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(viewConfigPriorityChkBtn),(state&HEADER_VISUAL_PRIO_ID));
#if GTK_CHECK_VERSION(2,10,6)
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(viewConfigGridChkBtn),(state&HEADER_VISUAL_GRID_ID));
#else
  gtk_widget_set_sensitive(viewConfigGridChkBtn,FALSE);
#endif

  /*
   * Group sort
   */
  if (hostinfo_refer_ipmsg_is_sort_with_group())
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(viewConfigGroupSortChkBtn),TRUE);
  else
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(viewConfigGroupSortChkBtn),FALSE);

  if (hostinfo_refer_ipmsg_group_sort_order())
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(viewConfigGroupInverseSortBtn),FALSE);
  else
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(viewConfigGroupInverseSortBtn),TRUE);

  /*
   * Sub sort
   */
  switch(hostinfo_refer_ipmsg_sub_sort_id()) {
  default:
  case SORT_TYPE_USER:
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(viewConfigSecondKeyUserRBtn),TRUE);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(viewConfigSecondKeyIPAddrRBtn),FALSE);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(viewConfigSecondKeyMachineRBtn),FALSE);
      break;
    case SORT_TYPE_MACHINE:
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(viewConfigSecondKeyUserRBtn),FALSE);
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(viewConfigSecondKeyIPAddrRBtn),FALSE);
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(viewConfigSecondKeyMachineRBtn),TRUE);
      break;
    case SORT_TYPE_IPADDR:
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(viewConfigSecondKeyUserRBtn),FALSE);
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(viewConfigSecondKeyIPAddrRBtn),TRUE);
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(viewConfigSecondKeyMachineRBtn),FALSE);
      break;

  }

  if (hostinfo_refer_ipmsg_sub_sort_order())
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(viewConfigOtherSortInverseBtn),FALSE);
  else
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(viewConfigOtherSortInverseBtn),TRUE);
  
  return window;
}

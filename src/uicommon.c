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

#define HOOKUP_ID(component,id) \
  g_object_set_data (G_OBJECT (component), "ID", (gpointer)id)
#define GET_ID(component)       \
  ((int)g_object_get_data (G_OBJECT (component),"ID"))
#define GLADE_HOOKUP_OBJECT_NO_REF(component,widget,name) \
  g_object_set_data (G_OBJECT (component), name, widget)
#define GLADE_HOOKUP_OBJECT(component,widget,name) \
  g_object_set_data_full (G_OBJECT (component), name, \
    gtk_widget_ref (widget), (GDestroyNotify) gtk_widget_unref)

static int
release_user_entry(GtkTreeView *view) {
  GtkTreePath *path;
  GtkTreeIter iter;
  GtkTreeModel *model;

  model = gtk_tree_view_get_model(view);
  
  if (!model)
    return -EINVAL;

  path=gtk_tree_path_new_first ();
  g_assert(path);

  if (gtk_tree_model_get_iter(model,&iter,path)) {
    while(gtk_list_store_remove(GTK_LIST_STORE(model), &iter))
      dbg_out("Remove OK\n");
  }
  gtk_tree_path_free (path);

  return 0;
}
static void
on_messageUserTree_user_clicked(GtkTreeViewColumn *treeviewcolumn,
				gpointer user_data) {
  dbg_out("Here");

  g_assert(treeviewcolumn);
  gtk_tree_view_column_set_sort_column_id(treeviewcolumn,0);
}
static void
on_messageUserTree_group_clicked(GtkTreeViewColumn *treeviewcolumn,
				 gpointer user_data) {
  dbg_out("Here");

  g_assert(treeviewcolumn);
  gtk_tree_view_column_set_sort_column_id(treeviewcolumn,1);
}
static void
on_messageUserTree_host_clicked(GtkTreeViewColumn *treeviewcolumn,
				gpointer user_data) {
  dbg_out("Here");

  g_assert(treeviewcolumn);
  gtk_tree_view_column_set_sort_column_id(treeviewcolumn,2);
}
static void
on_messageUserTree_ipaddr_clicked(GtkTreeViewColumn *treeviewcolumn,
				  gpointer user_data) {
  dbg_out("Here");

  g_assert(treeviewcolumn);
  gtk_tree_view_column_set_sort_column_id(treeviewcolumn,3);
}
static void
on_messageUserTree_logon_clicked(GtkTreeViewColumn *treeviewcolumn,
				 gpointer user_data) {
  dbg_out("Here");

  g_assert(treeviewcolumn);
  gtk_tree_view_column_set_sort_column_id(treeviewcolumn,4);
}
static void
on_messageUserTree_priority_clicked(GtkTreeViewColumn *treeviewcolumn,
				 gpointer user_data) {
  dbg_out("Here");

  g_assert(treeviewcolumn);
  gtk_tree_view_column_set_sort_column_id(treeviewcolumn,5);
}
static GtkWidget *
create_group_menu(GtkWidget *window) {
  GtkWidget *menu;
  GtkWidget *new_item;
  GList *groups,*node;


  menu=gtk_menu_new(); 

  groups=get_group_list();

  for(node=g_list_first(groups);node;node=g_list_next (node)) {

    if (node->data){
      dbg_out("Group:%s(%x)\n",node->data,node->data);
      new_item=create_menu_item(node->data,node->data,NULL,on_usermenu_group_item);

      gtk_menu_append( GTK_MENU(menu), new_item);
      GLADE_HOOKUP_OBJECT(new_item,window,"messageWindow");
      g_free(node->data);
    }
  }

  g_list_free(groups);

  return menu;
}
void
on_usermenu_group_item (gpointer data){
  GtkMenuItem *menuitem=(GtkMenuItem *)data;
  GtkWidget *child;

  dbg_out("Here:0x%x\n",menuitem);
  if (menuitem) {
    child=GTK_BIN (menuitem)->child;
    if (GTK_IS_LABEL (child)) {
        gchar *text;
	GtkWidget *window;
	GtkWidget *view;
	GtkTreeSelection *sel;
	GtkTreeModel *model;
	GtkTreeIter iter;
	gboolean valid;
	gchar *groupname;

        text=(gchar *)gtk_label_get_text (GTK_LABEL (child));
	window=lookup_widget(GTK_WIDGET(menuitem),"messageWindow");
        g_assert(window);
	view=lookup_widget(window,"messageUserTree");
        g_assert(view);

	sel=gtk_tree_view_get_selection(GTK_TREE_VIEW(view));
	gtk_tree_selection_unselect_all(sel);

	model=gtk_tree_view_get_model(GTK_TREE_VIEW(view));
	valid=gtk_tree_model_get_iter_first (model,&iter);
	while(valid){
	  gtk_tree_model_get (model, &iter, 
			      USER_VIEW_GROUP_ID, &groupname,
			      -1);
	  if (!strcmp(groupname,text)) {
	    dbg_out("Select:%s\n",groupname);
	    sel=gtk_tree_view_get_selection(GTK_TREE_VIEW(view));
	    gtk_tree_selection_select_iter(sel,&iter);
	  }
	  g_free(groupname);
	  valid = gtk_tree_model_iter_next (model, &iter);
	}
      }
  }
}

static GtkWidget *
internal_create_view_config_menu(GtkWidget *view){
  GtkWidget *menu;
  GtkWidget *window;
  GtkWidget *selectGroupMenu=NULL;

  window=lookup_widget(GTK_WIDGET(view),"messageWindow");
  g_assert(window);

  menu=create_userlistPopUpMenu ();
  selectGroupMenu=lookup_widget(menu,"groupselection");
  g_assert(selectGroupMenu);
  gtk_menu_item_set_submenu (GTK_MENU_ITEM (selectGroupMenu),create_group_menu(window));  
  GLADE_HOOKUP_OBJECT_NO_REF (menu, window, "messageWindow");

  return menu;
}
static gboolean
on_message_view_event_button_press_event (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
  dbg_out("messageWin: button press: %d\n", event->button);
  switch (event->button) {
  case 3:
    gtk_menu_popup(GTK_MENU(internal_create_view_config_menu(widget)), NULL, NULL, NULL, NULL,
		   event->button, event->time);
    
  default:					/* main menu */
    return FALSE;
    break;
  }
  return TRUE;
}

static GtkTreeViewColumn   *
create_user_view_column(int id,const char *title, GCallback callback){
  GtkTreeViewColumn   *col;
  GtkCellRenderer     *renderer;

  if ( (id<0) || (id>=MAX_VIEW_ID) || (!title) )
    return NULL;
  /* --- Column #1 --- */

  col = gtk_tree_view_column_new();

  gtk_tree_view_column_set_title(col, title);

  renderer = gtk_cell_renderer_text_new();

  /* pack cell renderer into tree view column */
  gtk_tree_view_column_pack_start(col, renderer, TRUE);

  /* connect 'text' property of the cell renderer to
   *  model column that contains the first name */
  gtk_tree_view_column_add_attribute(col, renderer, "text", id);

  gtk_tree_view_column_set_resizable (col,TRUE);
  gtk_tree_view_column_set_clickable(col,TRUE);
  gtk_tree_view_column_set_reorderable(col, TRUE);

  g_signal_connect ((gpointer) col, "clicked",
                    G_CALLBACK (callback),
		    NULL);
  HOOKUP_ID(col,id);
  return col;
}
static void
userview_set_new_view_priority(GtkTreeModel *model,
			       GtkTreeIter *iter,
			       int prio,
			       gboolean need_notify){
  int rc;
  gchar *ipaddr=NULL;

  if ( (prio < USERLIST_PRIO_MIN) || 
       (prio > USERLIST_PRIO_MAX) ) 
    return;

  dbg_out("Here: prio %d\n",prio);
  
  gtk_tree_model_get(model, iter, 
		     USER_VIEW_IPADDR_ID, &ipaddr,
		     -1);

  g_assert(ipaddr);
  userdb_replace_prio_by_addr(ipaddr,prio,need_notify);
  g_free(ipaddr);

  return;
}

void
setup_message_tree_view(GtkWidget *view) {
  GtkTreeViewColumn   *col;
  GtkCellRenderer     *renderer;
  guint state;
  GList *cols,*node;
  GtkTreeViewColumn  *view_cols[MAX_VIEW_ID];
  int col_id;
  int i;

  state=hostinfo_refer_header_state();
  /*
   * タイトル設定   
   */
  /* --- Column #1 --- */
  col=create_user_view_column(USER_VIEW_NICKNAME_ID, _("User"), G_CALLBACK(on_messageUserTree_user_clicked));
  view_cols[USER_VIEW_NICKNAME_ID]=col;
  g_assert(GET_ID(col)==USER_VIEW_NICKNAME_ID);
  
  /* --- Column #2 --- */
  col=create_user_view_column(USER_VIEW_GROUP_ID, _("Group"), G_CALLBACK(on_messageUserTree_group_clicked));
  view_cols[USER_VIEW_GROUP_ID]=col;
  g_assert(GET_ID(col)==USER_VIEW_GROUP_ID);
  if (state & HEADER_VISUAL_GROUP_ID)
    gtk_tree_view_column_set_visible(col,TRUE);
  else
    gtk_tree_view_column_set_visible(col,FALSE);

  /* --- Column #3 --- */
  col=create_user_view_column(USER_VIEW_HOST_ID, _("Host"), G_CALLBACK(on_messageUserTree_host_clicked));
  view_cols[USER_VIEW_HOST_ID]=col;
  g_assert(GET_ID(col)==USER_VIEW_HOST_ID);
  if (state & HEADER_VISUAL_HOST_ID)
    gtk_tree_view_column_set_visible(col,TRUE);
  else
    gtk_tree_view_column_set_visible(col,FALSE);

  /* --- Column #4 --- */
  col=create_user_view_column(USER_VIEW_IPADDR_ID, _("IP Address"), G_CALLBACK(on_messageUserTree_ipaddr_clicked));
  view_cols[USER_VIEW_IPADDR_ID]=col;
  g_assert(GET_ID(col)==USER_VIEW_IPADDR_ID);
  if (state & HEADER_VISUAL_IPADDR_ID)
    gtk_tree_view_column_set_visible(col,TRUE);
  else
    gtk_tree_view_column_set_visible(col,FALSE);

  /* --- Column #5 --- */
  col=create_user_view_column(USER_VIEW_LOGON_ID, _("LogOn"), G_CALLBACK(on_messageUserTree_logon_clicked));
  view_cols[USER_VIEW_LOGON_ID]=col;
  g_assert(GET_ID(col)==USER_VIEW_LOGON_ID);
  if (state & HEADER_VISUAL_LOGON_ID)
    gtk_tree_view_column_set_visible(col,TRUE);
  else
    gtk_tree_view_column_set_visible(col,FALSE);

  /* --- Column #6 --- */
  col=create_user_view_column(USER_VIEW_PRIO_ID, _("Priority"), G_CALLBACK(on_messageUserTree_priority_clicked));
  view_cols[USER_VIEW_PRIO_ID]=col;
  g_assert(GET_ID(col)==USER_VIEW_PRIO_ID);
  if (state & HEADER_VISUAL_PRIO_ID)
    gtk_tree_view_column_set_visible(col,TRUE);
  else
    gtk_tree_view_column_set_visible(col,FALSE);

  /* Append all columns  */
  for(i=0;i<MAX_VIEW_ID;++i) {
    if (!hostinfo_get_header_order(i,&col_id)) {
      dbg_out("append:%d->%d\n",i,col_id);
      gtk_tree_view_append_column(GTK_TREE_VIEW(view), view_cols[col_id]);
    }
  }

  gtk_signal_connect (GTK_OBJECT (view), "button_press_event",
		      GTK_SIGNAL_FUNC (on_message_view_event_button_press_event),
		      NULL);

}

void
update_user_entry(GList *top,GtkWidget *view,gboolean is_forced) {
  GtkTreeIter              toplevel;
  GtkListStore           *liststore = NULL;
  GtkWidget             *usersEntry = NULL;
  GtkTreeViewColumn            *col = NULL;
  userdb_t            *current_user = NULL;
  int                             i = 0;
  int                   users_count = 0;
  int                          prio = 0;
  guint                       state = 0;
  int                            id = 0;
  gchar              *nick_name_ref = NULL;
  gchar                     num_str[32];

  if ( (!top) || (!view) )
    return;

  state=hostinfo_refer_header_state();

  release_user_entry(GTK_TREE_VIEW(view));
  liststore = gtk_list_store_new(MAX_VIEW_ID,
                                 G_TYPE_STRING,
                                 G_TYPE_STRING,
                                 G_TYPE_STRING,
                                 G_TYPE_STRING,
                                 G_TYPE_STRING,
				 G_TYPE_STRING);

  for(i=0;(current_user=g_list_nth_data(top,i));++i) {
    g_assert(current_user);

    nick_name_ref = current_user->nickname;
    if ( (current_user->nickname == NULL) || ( current_user->nickname[0] == '\0' ) ) {
      if ( (current_user->user != NULL) && (current_user->user[0] != '\0') )
	nick_name_ref = current_user->user;
      else
	nick_name_ref = _("Unknown");
    }

    dbg_out("Update\n");
    dbg_out("NickName: %s\n", current_user->nickname);
    dbg_out("Group: %s\n",current_user->group);
    dbg_out("User: %s\n",current_user->user);
    dbg_out("Host: %s\n",current_user->host);
    dbg_out("IP Addr: %s\n",current_user->ipaddr);
    dbg_out("Priority: %d\n",current_user->prio);

    prio=current_user->prio;
    memset(num_str,0,32);
    if (prio>0)
      snprintf(num_str,31,"%d",prio);
    else
      snprintf(num_str,31,"-");
    num_str[31]='\0';

    if ( (prio>=0) || (is_forced) ){
      /* Append a top level row and leave it empty */
      gtk_list_store_append(liststore, &toplevel);
      gtk_list_store_set(liststore, &toplevel,
			 USER_VIEW_NICKNAME_ID, nick_name_ref,
			 USER_VIEW_GROUP_ID,current_user->group,
			 USER_VIEW_HOST_ID,current_user->host,
			 USER_VIEW_IPADDR_ID,current_user->ipaddr,
			 USER_VIEW_LOGON_ID,current_user->user,
			 USER_VIEW_PRIO_ID,num_str,
			 -1);
    }
    ++users_count;
    fflush(stdout);
  }
  gtk_tree_view_set_model(GTK_TREE_VIEW(view), GTK_TREE_MODEL(liststore));

  g_object_unref(liststore); /* destroy model automatically with view */

  gtk_tree_selection_set_mode(gtk_tree_view_get_selection(GTK_TREE_VIEW(view)),
                             GTK_SELECTION_MULTIPLE );
  /*
   * ユーザ数更新
   */
  usersEntry=lookup_widget(GTK_WIDGET(view),"messageWinUsersEntry");
  g_assert(usersEntry);
  snprintf(num_str,31,"%d",users_count);
  gtk_entry_set_text(GTK_ENTRY(usersEntry), num_str);

#if GTK_CHECK_VERSION(2,10,0)
  gtk_tree_view_set_rubber_banding (GTK_TREE_VIEW(view),TRUE);
  if (GTK_WIDGET_REALIZED(view)) {
#if GTK_CHECK_VERSION(2,10,6)
    gtk_tree_view_set_enable_tree_lines(GTK_TREE_VIEW(view),TRUE);
    if (state & HEADER_VISUAL_GRID_ID)
      gtk_tree_view_set_grid_lines(GTK_TREE_VIEW(view),GTK_TREE_VIEW_GRID_LINES_BOTH);
#endif
  }
#endif

  return;
}

void
ipmsg_update_ui(void){
  while (gtk_events_pending ())
    gtk_main_iteration ();
}

void
ipmsg_update_ui_user_list(void){

  userdb_invalidate_userdb();
  ipmsg_send_br_entry(udp_con,0);
}

void 
ipmsg_wait_ms(const int wait_ms){
  GTimer *wait_timer = NULL;
  gulong     elapsed = 0;
  gint    remains_ms = 0;

  wait_timer = g_timer_new();  
  g_assert(wait_timer != NULL);

  for(remains_ms = wait_ms;remains_ms > 0; --remains_ms) {

    g_timer_start(wait_timer);

    for(elapsed = 0; elapsed < 1000UL;g_timer_elapsed(wait_timer, &elapsed)) {
      ipmsg_update_ui(); /* 待ちループ */
    }
  }

  g_timer_destroy(wait_timer);

  return;
}

void
userview_set_view_priority(GtkTreeModel *model,
	       GtkTreePath *path,
	       GtkTreeIter *iter,
	       int prio){
  userview_set_new_view_priority(model, iter, prio, TRUE);

  return;
}
void
userview_set_view_priority_without_update(GtkTreeModel *model,
					  GtkTreePath *path, 
					  GtkTreeIter *iter,
					  int prio){
  /* path はNULLになりうることに注意(on_set_all_as_default_activate 参照) */
  userview_set_new_view_priority(model, iter, prio, FALSE);

  return;
}

int
remind_headers_state(GtkWidget *window){
  GtkWidget *view;
  GList *cols,*node;
  int count;
  GtkTreeViewColumn *col;

  dbg_out("here\n");

  g_assert(window);

  view=lookup_widget(GTK_WIDGET(window),"messageUserTree");

  cols=gtk_tree_view_get_columns(GTK_TREE_VIEW(view));
  
  for(node=g_list_first(cols),count=0;node;node=g_list_next (node),++count) {
    g_assert(node->data);
    col=node->data;
    dbg_out("%d : %d\n",count,GET_ID(col));
    hostinfo_set_ipmsg_header_order(count,GET_ID(col));
  }
  return 0;
}

void 
update_rsa_encryption_button_state(GtkToggleButton *togglebutton) {
	GtkWindow                   *window = NULL;
	GtkCheckButton  *encryptionCheckBtn = NULL;
	GtkButton             *pubkeyPWDBtn = NULL;

	dbg_out("here\n");

	window = 
		GTK_WINDOW(lookup_widget(GTK_WIDGET(togglebutton), "securityConfig"));
	g_assert(window != NULL);

	encryptionCheckBtn = 
		GTK_CHECK_BUTTON(lookup_widget(GTK_WIDGET(window), "RSAKeyEncryptionCBtn"));
	g_assert(encryptionCheckBtn != NULL);

	pubkeyPWDBtn =
		GTK_BUTTON(lookup_widget(GTK_WIDGET(window), "pubkeyPWDBtn"));
	g_assert(pubkeyPWDBtn != NULL);

	/*
	 * パスワードを使用しない場合は, パスワード設定ボタンを有効化しない.
	 */
#if defined(USE_OPENSSL)
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(encryptionCheckBtn)))
		gtk_widget_set_sensitive(GTK_WIDGET(pubkeyPWDBtn), TRUE);		
	else
		gtk_widget_set_sensitive(GTK_WIDGET(pubkeyPWDBtn), FALSE);
#else
	gtk_widget_set_sensitive(GTK_WIDGET(pubkeyPWDBtn), FALSE);
#endif  /*  USE_OPENSSL  */
	return;
}

void 
update_lockkey_button_state(GtkToggleButton *togglebutton) {
	GtkWindow                *window = NULL;
	GtkCheckButton  *uselockCheckBtn = NULL;
	GtkButton            *lockPWDBtn = NULL;

	dbg_out("here\n");

	window = 
		GTK_WINDOW(lookup_widget(GTK_WIDGET(togglebutton), "securityConfig"));
	g_assert(window != NULL);

	uselockCheckBtn = 
		GTK_CHECK_BUTTON(lookup_widget(GTK_WIDGET(window), "useLockCBtn"));
	g_assert(uselockCheckBtn != NULL);

	lockPWDBtn =
		GTK_BUTTON(lookup_widget(GTK_WIDGET(window), "lockPWDBtn"));
	g_assert(lockPWDBtn != NULL);

	/*
	 * 錠を使用しない場合は, パスワード設定ボタンを有効化しない.
	 */
#if defined(USE_OPENSSL)
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(uselockCheckBtn)))
		gtk_widget_set_sensitive(GTK_WIDGET(lockPWDBtn), TRUE);
	else 
		gtk_widget_set_sensitive(GTK_WIDGET(lockPWDBtn), FALSE);
#else
	gtk_widget_set_sensitive(GTK_WIDGET(lockPWDBtn), FALSE);
#endif  /* USE_OPENSSL  */
	return;	
}


GtkWidget *
internal_create_crypt_config_window(void){
  int                          rc = 0;
  GtkWidget         *configWindow = NULL;
  GtkWidget   *sendHostListChkBtn = NULL;
  GtkWidget *obtainHostlistChkBtn = NULL;
  GtkWidget    *configRC2Bit40Btn = NULL;
  GtkWidget   *configRC2Bit128Btn = NULL;
  GtkWidget   *configRC2Bit256Btn = NULL;
  GtkWidget    *configBFBit128Btn = NULL;
  GtkWidget    *configBFBit256Btn = NULL;
  GtkWidget    *configAESBit128Btn = NULL;
  GtkWidget   *configAESBit192Btn = NULL;
  GtkWidget   *configAESBit256Btn = NULL;
  GtkWidget   *configRSABit512Btn = NULL;
  GtkWidget  *configRSABit1024Btn = NULL;
  GtkWidget  *configRSABit2048Btn = NULL;
  GtkWidget         *configMD5Btn = NULL;
  GtkWidget        *configSHA1Btn = NULL;
  GtkWidget    *keySelectAlgoCBtn = NULL;
  GtkWidget *RSAKeyEncryptionCBtn = NULL;
  GtkWidget          *useLockCBtn = NULL;
  unsigned long             state = 0;
  
  configWindow = create_securityConfig();
  g_assert(configWindow != NULL);

  sendHostListChkBtn = lookup_widget(configWindow, "sendHostListChkBtn");
  obtainHostlistChkBtn = lookup_widget(configWindow, "obtainHostlistChkBtn");
  configRC2Bit40Btn = lookup_widget(configWindow, "configRC2Bit40Btn");
  configRC2Bit128Btn = lookup_widget(configWindow, "configRC2Bit128Btn");
  configRC2Bit256Btn = lookup_widget(configWindow, "configRC2Bit256Btn");
  configBFBit128Btn = lookup_widget(configWindow, "configBFBit128Btn");
  configBFBit256Btn = lookup_widget(configWindow, "configBFBit256Btn");
  configAESBit128Btn = lookup_widget(configWindow, "configAESBit128Btn");
  configAESBit192Btn = lookup_widget(configWindow, "configAESBit192Btn");
  configAESBit256Btn = lookup_widget(configWindow, "configAESBit256Btn");
  configRSABit512Btn = lookup_widget(configWindow, "configRSABit512Btn");
  configRSABit1024Btn = lookup_widget(configWindow, "configRSABit1024Btn");
  configRSABit2048Btn = lookup_widget(configWindow, "configRSABit2048Btn");
  configMD5Btn = lookup_widget(configWindow, "configMD5Btn");
  configSHA1Btn = lookup_widget(configWindow, "configSHA1Btn");
  keySelectAlgoCBtn = lookup_widget(configWindow, "keySelectAlgoCBtn");
  RSAKeyEncryptionCBtn = lookup_widget(configWindow, "RSAKeyEncryptionCBtn");
  useLockCBtn = lookup_widget(configWindow, "useLockCBtn");

  rc = hostinfo_refer_ipmsg_cipher(&state);
  if (rc != 0)
    state = 0;

  /*
   *ホストリスト制御
   */
  if (hostinfo_refer_ipmsg_is_allow_hlist())
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(sendHostListChkBtn),TRUE);
  else
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(sendHostListChkBtn),FALSE);

  if (hostinfo_refer_ipmsg_is_get_hlist())
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(obtainHostlistChkBtn),TRUE);
  else
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(obtainHostlistChkBtn),FALSE);

  /*
   *暗号選択
   */

#if defined(USE_OPENSSL)

  if (state & IPMSG_RC2_40)
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(configRC2Bit40Btn),TRUE);
  else
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(configRC2Bit40Btn),FALSE);

  if (state & IPMSG_RC2_128)
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(configRC2Bit128Btn),TRUE);
  else
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(configRC2Bit128Btn),FALSE);

  if (state & IPMSG_RC2_256)
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(configRC2Bit256Btn),TRUE);
  else
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(configRC2Bit256Btn),FALSE);

  if (state & IPMSG_BLOWFISH_128)
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(configBFBit128Btn),TRUE);
  else
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(configBFBit128Btn),FALSE);

  if (state & IPMSG_BLOWFISH_256)
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(configBFBit256Btn),TRUE);
  else
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(configBFBit256Btn),FALSE);

  if (state & IPMSG_AES_128)
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(configAESBit128Btn),TRUE);
  else
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(configAESBit128Btn),FALSE);

  if (state & IPMSG_AES_192)
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(configAESBit192Btn),TRUE);
  else
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(configAESBit192Btn),FALSE);

  if (state & IPMSG_AES_256)
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(configAESBit256Btn),TRUE);
  else
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(configAESBit256Btn),FALSE);

  if (state & IPMSG_RSA_512)
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(configRSABit512Btn),TRUE);
  else
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(configRSABit512Btn),FALSE);

  if (state & IPMSG_RSA_1024)
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(configRSABit1024Btn),TRUE);
  else
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(configRSABit1024Btn),FALSE);

  if (state & IPMSG_RSA_2048)
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(configRSABit2048Btn),TRUE);
  else
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(configRSABit2048Btn),FALSE);

  if (state & IPMSG_SIGN_MD5)
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(configMD5Btn),TRUE);
  else
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(configMD5Btn),FALSE);

  if (state & IPMSG_SIGN_SHA1)
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(configSHA1Btn),TRUE);
  else
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(configSHA1Btn),FALSE);

  if (hostinfo_refer_ipmsg_crypt_policy_is_speed())
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(keySelectAlgoCBtn),TRUE);
  else
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(keySelectAlgoCBtn),FALSE);

  if (hostinfo_refer_ipmsg_encrypt_public_key())
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(RSAKeyEncryptionCBtn),TRUE);
  else
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(RSAKeyEncryptionCBtn),FALSE);
  if (hostinfo_refer_ipmsg_use_lock())
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(useLockCBtn),TRUE);
  else
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(useLockCBtn),FALSE);

#else
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(configRC2Bit40Btn),FALSE);
  gtk_widget_set_sensitive(GTK_WIDGET(configRC2Bit40Btn), FALSE);

  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(configRC2Bit128Btn),FALSE);
  gtk_widget_set_sensitive(GTK_WIDGET(configRC2Bit128Btn), FALSE);

  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(configRC2Bit256Btn),FALSE);
  gtk_widget_set_sensitive(GTK_WIDGET(configRC2Bit256Btn), FALSE);

  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(configBFBit128Btn),FALSE);
  gtk_widget_set_sensitive(GTK_WIDGET(configBFBit128Btn), FALSE);

  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(configBFBit256Btn),FALSE);
  gtk_widget_set_sensitive(GTK_WIDGET(configBFBit256Btn), FALSE);

  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(configRSABit512Btn),FALSE);
  gtk_widget_set_sensitive(GTK_WIDGET(configRSABit512Btn), FALSE);

  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(configAESBit128Btn),FALSE);
  gtk_widget_set_sensitive(GTK_WIDGET(configAESBit128Btn), FALSE);

  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(configAESBit192Btn),FALSE);
  gtk_widget_set_sensitive(GTK_WIDGET(configAESBit192Btn), FALSE);

  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(configAESBit256Btn),FALSE);
  gtk_widget_set_sensitive(GTK_WIDGET(configAESBit256Btn), FALSE);

  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(configBFBit128Btn),FALSE);
  gtk_widget_set_sensitive(GTK_WIDGET(configBFBit128Btn), FALSE);

  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(configRSABit1024Btn),FALSE);
  gtk_widget_set_sensitive(GTK_WIDGET(configRSABit1024Btn), FALSE);

  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(configRSABit2048Btn),FALSE);
  gtk_widget_set_sensitive(GTK_WIDGET(configRSABit2048Btn), FALSE);

  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(configMD5Btn),FALSE);
  gtk_widget_set_sensitive(GTK_WIDGET(configMD5Btn), FALSE);

  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(configSHA1Btn),FALSE);
  gtk_widget_set_sensitive(GTK_WIDGET(configSHA1Btn), FALSE);

  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(keySelectAlgoCBtn),FALSE);
  gtk_widget_set_sensitive(GTK_WIDGET(keySelectAlgoCBtn), FALSE);

  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(RSAKeyEncryptionCBtn),FALSE);
  gtk_widget_set_sensitive(GTK_WIDGET(RSAKeyEncryptionCBtn), FALSE);

  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(useLockCBtn),FALSE);
  gtk_widget_set_sensitive(GTK_WIDGET(useLockCBtn), FALSE);

#endif  /* USE_OPENSSL  */

  update_rsa_encryption_button_state(GTK_TOGGLE_BUTTON(RSAKeyEncryptionCBtn));
  update_lockkey_button_state(GTK_TOGGLE_BUTTON(useLockCBtn));

  return configWindow;
}

int 
apply_crypt_config_window(GtkWindow *configWindow){
  int                                rc = 0;
  GtkWidget         *sendHostListChkBtn = NULL;
  GtkWidget       *obtainHostlistChkBtn = NULL;
  GtkWidget          *configRC2Bit40Btn = NULL;
  GtkWidget         *configRC2Bit128Btn = NULL;
  GtkWidget         *configRC2Bit256Btn = NULL;
  GtkWidget          *configBFBit128Btn = NULL;
  GtkWidget          *configBFBit256Btn = NULL;
  GtkWidget          *configAESBit128Btn = NULL;
  GtkWidget         *configAESBit192Btn = NULL;
  GtkWidget         *configAESBit256Btn = NULL;
  GtkWidget         *configRSABit512Btn = NULL;
  GtkWidget        *configRSABit1024Btn = NULL;
  GtkWidget        *configRSABit2048Btn = NULL;
  GtkWidget               *configMD5Btn = NULL;
  GtkWidget              *configSHA1Btn = NULL;
  GtkWidget          *keySelectAlgoCBtn = NULL;
  GtkWidget       *RSAKeyEncryptionCBtn = NULL;
  GtkWidget                *useLockCBtn = NULL;
  const char *configured_encoded_passwd = NULL;
  unsigned long                   state = 0;
  
  if (configWindow == NULL)
    return -EINVAL;

  sendHostListChkBtn = lookup_widget(GTK_WIDGET(configWindow), "sendHostListChkBtn");
  obtainHostlistChkBtn = lookup_widget(GTK_WIDGET(configWindow), "obtainHostlistChkBtn");

  configRC2Bit40Btn = lookup_widget(GTK_WIDGET(configWindow),"configRC2Bit40Btn");
  configRC2Bit128Btn = lookup_widget(GTK_WIDGET(configWindow), "configRC2Bit128Btn");
  configRC2Bit256Btn = lookup_widget(GTK_WIDGET(configWindow), "configRC2Bit256Btn");
  configBFBit128Btn = lookup_widget(GTK_WIDGET(configWindow), "configBFBit128Btn");
  configBFBit256Btn = lookup_widget(GTK_WIDGET(configWindow), "configBFBit256Btn");
  configAESBit128Btn = lookup_widget(GTK_WIDGET(configWindow),"configAESBit128Btn");
  configAESBit192Btn = lookup_widget(GTK_WIDGET(configWindow), "configAESBit192Btn");
  configAESBit256Btn = lookup_widget(GTK_WIDGET(configWindow), "configAESBit256Btn");
  configRSABit512Btn = lookup_widget(GTK_WIDGET(configWindow), "configRSABit512Btn");
  configRSABit1024Btn = lookup_widget(GTK_WIDGET(configWindow), "configRSABit1024Btn");
  configRSABit2048Btn = lookup_widget(GTK_WIDGET(configWindow), "configRSABit2048Btn");
  configMD5Btn = lookup_widget(GTK_WIDGET(configWindow), "configMD5Btn");
  configSHA1Btn = lookup_widget(GTK_WIDGET(configWindow), "configSHA1Btn");

  keySelectAlgoCBtn = lookup_widget(GTK_WIDGET(configWindow), "keySelectAlgoCBtn");
  RSAKeyEncryptionCBtn = lookup_widget(GTK_WIDGET(configWindow), "RSAKeyEncryptionCBtn");
  useLockCBtn = lookup_widget(GTK_WIDGET(configWindow), "useLockCBtn");

  /*
   *ホストリスト制御
   */
  hostinfo_set_ipmsg_is_allow_hlist(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(sendHostListChkBtn))); 
  hostinfo_set_ipmsg_is_get_hlist(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(obtainHostlistChkBtn)));

  /*
   *暗号選択
   */
#if defined(USE_OPENSSL)
  /*
   * 共通鍵
   */
  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(configRC2Bit40Btn)))
    state |= IPMSG_RC2_40;
  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(configRC2Bit128Btn)))
    state |= IPMSG_RC2_128;
  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(configRC2Bit256Btn)))
    state |= IPMSG_RC2_256;

  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(configBFBit128Btn)))
    state |= IPMSG_BLOWFISH_128;
  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(configBFBit256Btn)))
    state |= IPMSG_BLOWFISH_256;

  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(configAESBit128Btn)))
    state |= IPMSG_AES_128;
  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(configAESBit192Btn)))
    state |= IPMSG_AES_192;
  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(configAESBit256Btn)))
    state |= IPMSG_AES_256;

  /*
   * RSA
   */
  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(configRSABit512Btn)))
    state |= IPMSG_RSA_512;
  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(configRSABit1024Btn)))
    state |= IPMSG_RSA_1024;
  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(configRSABit2048Btn)))
    state |= IPMSG_RSA_2048;

  /*
   * 署名
   */
  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(configMD5Btn)))
    state |= IPMSG_SIGN_MD5;
  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(configSHA1Btn)))
    state |= IPMSG_SIGN_SHA1;

  /*
   * セキュリティ設定
   */
  hostinfo_set_ipmsg_crypt_policy_as_speed(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(keySelectAlgoCBtn)));
  
  configured_encoded_passwd = hostinfo_refer_encryption_key_password();

  if ( (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(RSAKeyEncryptionCBtn))) &&
       (pbkdf2_encoded_passwd_configured(configured_encoded_passwd) == 0) )  {
      hostinfo_set_ipmsg_encrypt_public_key(TRUE);      
  } else {
      hostinfo_set_ipmsg_encrypt_public_key(FALSE);
  }
  hostinfo_set_ipmsg_use_lock(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(useLockCBtn)));
#else

  /*
   * 暗号鍵
   */
  state &= ~SYM_CAPS;

  /*
   * RSA
   */
  state &= ~RSA_CAPS;

  /*
   * 署名
   */
  state &= ~SIGN_CAPS;

  /*
   * セキュリティ設定
   */
  hostinfo_set_ipmsg_crypt_policy_as_speed(FALSE);
  hostinfo_set_ipmsg_encrypt_public_key(FALSE);
  hostinfo_set_ipmsg_use_lock(FALSE);
#endif  /*  USE_OPENSSL  */

  hostinfo_set_ipmsg_cipher(state);  /*  鍵情報登録  */

  return 0;
}

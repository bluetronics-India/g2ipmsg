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

#define GLADE_HOOKUP_OBJECT(component,widget,name) \
  g_object_set_data_full (G_OBJECT (component), name, \
    gtk_widget_ref (widget), (GDestroyNotify) gtk_widget_unref)

static GList *store_wins=NULL;
static GList *displayed_wins=NULL;
static GList *opened_wins=NULL;
static guint init_win_timer_id;

static GtkWidget *normal_image=NULL;
static GtkWidget *reverse_image=NULL;

GStaticMutex openwin_mutex = G_STATIC_MUTEX_INIT;
GStaticMutex displayedwin_mutex = G_STATIC_MUTEX_INIT;
GStaticMutex store_win_mutex = G_STATIC_MUTEX_INIT;

int 
init_ipmsg_private(ipmsg_private_data_t **priv,const int type){
  ipmsg_private_data_t *new_priv;
  int rc=0;

  if  (!priv)
    return -EINVAL;

  rc=-ENOMEM;
  new_priv=g_malloc(sizeof(ipmsg_private_data_t));
  if (!new_priv)
    goto error_out;

  rc=0;
  new_priv->magic=IPMSG_PRIVATE_DATA_MAGIC;
  new_priv->type=type;
  new_priv->data=NULL;
  *priv=new_priv;

 error_out:
  return rc;
}
static void
destroy_recvmsg_private(ipmsg_private_data_t *private) {
  ipmsg_recvmsg_private_t *priv;

  IPMSG_ASSERT_PRIVATE(private,IPMSG_PRIVATE_RECVMSG);
  priv=private->data;

  dbg_out("destroy private:ipaddr %s pktno:%d\n",priv->ipaddr,priv->pktno);
  g_assert(priv->ipaddr);
  g_free(priv->ipaddr);
  priv->ipaddr=NULL;
  if (priv->ext_part){
    g_free(priv->ext_part);
    priv->ext_part=NULL;
  }
  g_free(priv);
}
int
init_recvmsg_private(const char *ipaddr,int flags,int pktno, ipmsg_private_data_t **priv) {
  ipmsg_recvmsg_private_t *element;
  ipmsg_private_data_t *new_prev;
  int rc=0;

  if ( (!ipaddr) || (!priv) )
    return -EINVAL;

  rc=-ENOMEM;
  element=g_malloc(sizeof(ipmsg_recvmsg_private_t));
  if (!element)
    goto no_free_out;

  memset(element,0,sizeof(ipmsg_recvmsg_private_t));
  if (init_ipmsg_private(&new_prev,IPMSG_PRIVATE_RECVMSG)<0)
    goto error_out;
  
  element->pktno=pktno;
  element->flags=flags;

  element->ipaddr=g_strdup(ipaddr);
  if (element->ipaddr) {
    new_prev->data=(void *)element;
    *priv=new_prev;
    return 0;
  }

 no_ipaddr:
  g_free(new_prev);
 error_out:
  g_free(element);
 no_free_out:
  return rc;
}
void
destroy_ipmsg_private(gpointer data) {
  ipmsg_private_data_t *private;

  g_assert(data);
  dbg_out("Called destroy private::%x\n",(unsigned int)data);
  private = (ipmsg_private_data_t *)data;
  dbg_out("Called destroy private magic::%x\n",private->magic);
  if (private->magic != IPMSG_PRIVATE_DATA_MAGIC)
    goto error_out;

  switch(private->type) {
  case IPMSG_PRIVATE_RECVMSG:
    destroy_recvmsg_private(private);
    break;
  case IPMSG_PRIVATE_PASSWD:
    private->data = NULL; /* ただの整数型なので問題無し  */
    break;
  default:
    g_assert_not_reached();
    break;
  }

 error_out:
  dbg_out("Free private ::%x\n",(unsigned int)private);
  g_free(private);
  dbg_out("Complete free private ::%x\n",(unsigned int)private);
  return;
}

/** メッセージウィンドウの開封ボタン押下動作処理
 *  @param[in]  button     開封ボタンウィジェットへのポインタ
 *  @param[in]  user_data  付帯情報(未使用)
 */
void
on_sealBtn_clicked                     (GtkButton       *button,
                                        gpointer         user_data){
	int                               rc = 0;
	GtkTextView               *text_view = NULL;
	GtkScrolledWindow    *scrolledwindow = NULL;
	GtkWindow                       *top = NULL;
	GtkWidget                 *container = NULL;
	GtkToggleButton         *cite_toggle = NULL;
	GtkWindow           *download_window = NULL;
	GtkDialog              *passwdDialog = NULL;
	ipmsg_private_data_t        *private = NULL;
	ipmsg_recvmsg_private_t *sender_info = NULL;
	GtkTextIter      start_iter,end_iter;
	GtkTextBuffer                *buffer = NULL;
	gchar                       *message = NULL;
	gint                       text_line = 0;

	g_assert(button != NULL);

	private = 
		(ipmsg_private_data_t *)lookup_widget(GTK_WIDGET(button), 
		    "senderInfo"  );

	IPMSG_ASSERT_PRIVATE(private, IPMSG_PRIVATE_RECVMSG);

	sender_info = private->data;

	top = GTK_WINDOW(lookup_widget(GTK_WIDGET(button), "viewWindow"));
	g_assert(top != NULL);

	cite_toggle = GTK_TOGGLE_BUTTON(lookup_widget(GTK_WIDGET(button), 
		"viewwindowCitationCheck"));
	g_assert(cite_toggle != NULL);
	
	/*
	 * 錠前付きメッセージの処理
	 */
	if (sender_info->flags & IPMSG_PASSWORDOPT) {
		rc = password_confirm_window(HOSTINFO_PASSWD_TYPE_LOCK, NULL);
		if (rc == -EPERM)
			return;  /*  解錠失敗  */

		sender_info->flags &= ~IPMSG_PASSWORDOPT; /* 錠解除 */
	}

	if ( (sender_info->ipaddr) && 
	    (!(sender_info->flags & IPMSG_NO_REPLY_OPTS) ) ) {
		dbg_out("recv notify:%s(%d)\n", 
		    sender_info->ipaddr, sender_info->pktno);
		ipmsg_send_read_msg(udp_con, sender_info->ipaddr, 
		    sender_info->pktno);
	}

	sender_info->flags &= ~IPMSG_SECRETOPT;  
	dbg_out("new flags %x\n", sender_info->flags);

	text_view = GTK_TEXT_VIEW(lookup_widget(GTK_WIDGET(button), 
		"viewwindowTextView"));

	container = gtk_widget_get_parent( GTK_WIDGET(button) );
	g_assert(container != NULL);

	dbg_out("Seal Btn Container Name:%s(0x%x)\n", 
	    gtk_widget_get_name(container), (unsigned int)container);

	scrolledwindow = 
		GTK_SCROLLED_WINDOW(lookup_widget(GTK_WIDGET(button), 
			"scrolledwindow6"));

	/*
	 * View Portを取り除くと自動的にviewport(とその上のボタン)が破棄される.
	 */
	gtk_container_remove(GTK_CONTAINER(scrolledwindow), 
	    GTK_WIDGET (container));
	gtk_container_add(GTK_CONTAINER (scrolledwindow), 
	    GTK_WIDGET(text_view));
	
	/*
	 * ログ記録
	 */
	if (hostinfo_refer_ipmsg_log_locked_message_handling()) {

	  buffer = gtk_text_view_get_buffer(text_view);
	  text_line = gtk_text_buffer_get_line_count(buffer);
	  gtk_text_buffer_get_start_iter(buffer, &start_iter);
	  gtk_text_buffer_get_end_iter(buffer, &end_iter);
	  message = gtk_text_buffer_get_text(buffer, &start_iter, &end_iter,
				  FALSE);
	  if (message != NULL) {
	    /* ログの成否に関わらず先に進む  */
	    logfile_recv_log(sender_info->ipaddr, sender_info->flags, message);
	    g_free(message); /* メッセージ開放  */
	  }
	}

	gtk_widget_set_sensitive(GTK_WIDGET(cite_toggle), TRUE);
	if (hostinfo_refer_ipmsg_default_citation()) 
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cite_toggle), 
		    TRUE);
	else
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cite_toggle), 
		    FALSE);

	if ((sender_info->flags & IPMSG_FILEATTACHOPT) 
	    && (sender_info->ext_part) ){

		/*  ダウンロードウィンドウ生成  */
		dbg_out("This message has attachment\n");
		rc = internal_create_download_window(top, &download_window);
		if (rc == 0) {
			g_assert (download_window != NULL);
			
			append_displayed_window(download_window);
			gtk_widget_show(GTK_WIDGET(download_window));

		} else {
		  ipmsg_err_dialog("%s : %s (%d)",
				   _("Can not create download window"),
				   strerror(-rc), rc);
		}
	}

	append_opened_window(top);

	if (sender_info->ext_part != NULL) {
		g_free(sender_info->ext_part);
		sender_info->ext_part = NULL;
	}
}

void
on_sealBtn_destroy                        (GtkObject       *object,
                                        gpointer         user_data)
{
  dbg_out("here\n");
}
static GtkWindow *
internal_create_recv_message_window(const msg_data_t *msg,const char *from){
	int                     rc = 0;
	char              *message = NULL;
	int                  pktno = 0;
	int                  flags = 0;
	gchar    *internal_message = NULL;
	GtkWindow          *window = NULL;
	GtkTextView     *text_view = NULL;
	GtkTextBuffer      *buffer = NULL;
	GtkWidget         *SealBtn = NULL;
	gint                   len = 0;
	GtkWidget  *scrolledwindow = NULL;
	GtkLabel        *userlabel = NULL;
	GtkLabel        *timelabel = NULL;
	GtkWidget     *cite_toggle = NULL;
	ipmsg_private_data_t *priv = NULL;
	userdb_t        *user_info = NULL;
	char          label_string[64];
	char           time_string[26];
	char           seal_string[96];
	gint      width = 0, height = 0;


	if ( (msg == NULL) || (from == NULL) )
		return NULL;

	message = msg->message;
	if (message == NULL)
		return NULL;

	pktno = msg->pkt_seq_no;
	flags = msg->command_opts;
  
	convert_string_internal(message, (const gchar **)&internal_message);
	if (internal_message == NULL) {
		ipmsg_err_dialog(_("Can not convert message from %s into ineternal representation"), 
		    from);
		return NULL;
	}

	dbg_out("Recive_win:(from:%s)%s\n", from, internal_message);
	window = GTK_WINDOW(create_viewWindow());

	timelabel = GTK_LABEL(lookup_widget(GTK_WIDGET(window), "viewwindowDateLabel"));
	if (get_current_time_string(time_string, ((msg->tv).tv_sec)))
		gtk_label_set_text(timelabel, _("UnKnown"));
	else
		gtk_label_set_text(timelabel, time_string);

	userlabel = GTK_LABEL(lookup_widget(GTK_WIDGET(window), "viewwindowUserInfoLabel"));
	g_assert(userlabel);

	rc = userdb_search_user_by_addr(from, (const userdb_t **)&user_info);
	if (rc == 0) {
		g_assert(userlabel);
		snprintf(label_string, 63, "%s@%s (%s)", 
		    user_info->nickname, user_info->group, user_info->host);
		label_string[63] = '\0';
		gtk_label_set_text(userlabel,label_string);
		g_assert(destroy_user_info(user_info) == 0);
	} else {
		dbg_out("Can not find user:rc=%d\n", rc);
		gtk_label_set_text(userlabel, refer_user_name_from_msg(msg));
	}

	cite_toggle = lookup_widget(GTK_WIDGET(window), "viewwindowCitationCheck");
	g_assert(cite_toggle != NULL);

	if (hostinfo_refer_ipmsg_default_citation()) 
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cite_toggle),
		    TRUE);
	else
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cite_toggle),
		    FALSE);

	text_view = 
		GTK_TEXT_VIEW(lookup_widget(GTK_WIDGET(window), 
			"viewwindowTextView"));
	buffer = gtk_text_view_get_buffer(text_view);

	len = strlen(internal_message);
	gtk_text_buffer_set_text(buffer, internal_message, len);

	gtk_widget_set_events(GTK_WIDGET(text_view), GDK_BUTTON_PRESS_MASK);
	gtk_signal_connect (GTK_OBJECT (text_view), "button_press_event",
	    GTK_SIGNAL_FUNC (on_viewwindowTextView_button_press_event), NULL);

	/*
	 * 添付ファイル
	 */

	if (init_recvmsg_private(from,flags,pktno,&priv) == 0) {
		dbg_out("Hook private\n");
		IPMSG_HOOKUP_DATA(window,priv, "senderInfo");
	}

	if ((priv != NULL) && (flags & IPMSG_FILEATTACHOPT) ){
		ipmsg_recvmsg_private_t *element;
		dbg_out("This message has attachment:%s\n", msg->extstring);
		element = (ipmsg_recvmsg_private_t *)priv->data;
		element->ext_part = g_strdup(msg->extstring);
	}

	/*
	 * 封書
	 */
	if (flags & IPMSG_SECRETOPT) {
		/*  disable citation  */
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cite_toggle), 
		    FALSE);
		gtk_widget_set_sensitive(cite_toggle, FALSE);

		scrolledwindow = 
			lookup_widget(GTK_WIDGET(window), "scrolledwindow6");
		g_assert(scrolledwindow);

		memset(seal_string, 0, 96);
		snprintf(seal_string, 96, "%s %s %s",
		    _("Remove the seal"), 
		    ( (flags & IPMSG_FILEATTACHOPT) ? _("[Attachment]") : ""),
		    ( (flags & IPMSG_PASSWORDOPT) ? _("(Locked)") : "") );
		seal_string[95] = '\0';

		SealBtn = gtk_button_new_with_label (seal_string);

		if (SealBtn != NULL){
			gtk_container_remove(GTK_CONTAINER(scrolledwindow), 
			    GTK_WIDGET(text_view));
			dbg_out("Seal set\n");
			gtk_widget_show(SealBtn);

			gtk_scrolled_window_add_with_viewport(
				GTK_SCROLLED_WINDOW(scrolledwindow), SealBtn);

			g_signal_connect ((gpointer) SealBtn, "clicked",
			    G_CALLBACK (on_sealBtn_clicked), NULL);

			g_signal_connect ((gpointer) SealBtn, "destroy",
			    G_CALLBACK (on_sealBtn_destroy), NULL);
			GLADE_HOOKUP_OBJECT (window, SealBtn, "SealBtn");
		}
	}

	g_free(internal_message);


	/*
	 *ウィンドウサイズ復元
	 */
	if (hostinfo_get_ipmsg_recv_window_size(&width, &height) == 0) {
		dbg_out("Resize:(%d,%d)\n",width,height);
		gtk_window_resize (GTK_WINDOW(window), width, height);
	}

	return window;
}

static void
print_one_window(gpointer data,gpointer user_data) {

  if (!data)
    return;

  dbg_out("stored window: %x\n",(unsigned int)data);

  return;
}

static void
listup_current_stored_windows(void){

  g_static_mutex_lock(&store_win_mutex);
  g_list_foreach(store_wins,
		 print_one_window,
		 NULL);
  g_static_mutex_unlock(&store_win_mutex);
}
static void
show_one_window(gpointer data,gpointer user_data) {
  int                                rc = 0;
  GtkWindow                       *top = NULL;
  GtkWindow           *download_window = NULL;
  ipmsg_private_data_t        *private = NULL;
  ipmsg_recvmsg_private_t *sender_info = NULL;

  if (data == NULL)
    return;


  top = GTK_WINDOW(data);
  dbg_out("show stored window: %x\n", (unsigned int)top);

  private = (ipmsg_private_data_t *)lookup_widget(GTK_WIDGET(top), 
      "senderInfo"  );

  IPMSG_ASSERT_PRIVATE(private,IPMSG_PRIVATE_RECVMSG);

  sender_info = private->data;
  g_assert(sender_info != NULL);

  /* ダウンロードウィンドウ生成    */
  if ((!(sender_info->flags & IPMSG_SECRETOPT))  &&
      (sender_info->flags & IPMSG_FILEATTACHOPT) &&
      (sender_info->ext_part)){
    dbg_out("This message has attachment\n"); 
    rc = internal_create_download_window(top, &download_window);
    if (!rc) {
      g_assert (download_window);
      append_displayed_window(download_window);
      gtk_widget_show (GTK_WIDGET(download_window));
    }else{
      err_out("Can not create download window %s (%d)\n", strerror(-rc), rc);
    }
  }
  if (!(sender_info->flags & IPMSG_SECRETOPT)) {
    append_opened_window(top);
  }

  append_displayed_window(top);

  gtk_widget_show (GTK_WIDGET(top));

  play_sound();

  return;
}

void
show_stored_windows(void) {

  g_static_mutex_lock(&store_win_mutex);
  g_list_foreach(store_wins,
		 show_one_window,
		 NULL);  
  g_list_free(store_wins);
  store_wins=NULL;
  g_static_mutex_unlock(&store_win_mutex);
}

void
store_message_window(const msg_data_t *msg,const char *from){
	GtkWindow *window = NULL;

	window = internal_create_recv_message_window(msg, from);

	if (window != NULL) {

		g_static_mutex_lock(&store_win_mutex);

		store_wins = g_list_append(store_wins, window);

		g_static_mutex_unlock(&store_win_mutex);

		listup_current_stored_windows();
	}
}

gboolean 
recv_windows_are_stored(void) {
  GList *first;

  g_static_mutex_lock(&store_win_mutex);
  first=g_list_first(store_wins);
  g_static_mutex_unlock(&store_win_mutex);

  if (first)
    return TRUE;

  return FALSE;
}
void
recv_message_window(const msg_data_t *msg,const char *from){
	int                               rc = 0;
	GtkWindow                    *window = NULL;
	GtkTextView               *text_view = NULL;
	GtkWindow           *download_window = NULL;
	ipmsg_private_data_t        *private = NULL;
	ipmsg_recvmsg_private_t *sender_info = NULL;

	if ( (msg == NULL) || (from == NULL) )
		return;

	window = internal_create_recv_message_window(msg, from);
	if (window != NULL) {
		private = 
			(ipmsg_private_data_t *)lookup_widget(
				GTK_WIDGET(window), "senderInfo");

		IPMSG_ASSERT_PRIVATE(private, IPMSG_PRIVATE_RECVMSG);

		sender_info = private->data;

		/* 封書でなければここでダウンロードウィンドウ生成    */

		if ( (!(sender_info->flags & IPMSG_SECRETOPT))  &&
		    (sender_info->flags & IPMSG_FILEATTACHOPT) && 
		    (sender_info->ext_part != NULL) ) {

			dbg_out("This message has attachment\n"); 
			rc = 
				internal_create_download_window(window, 
				    &download_window);
			if (rc == 0) {
				g_assert(download_window != NULL);

				gtk_widget_show(GTK_WIDGET(download_window));

				append_displayed_window(download_window);
			} else {
				err_out("Can't create download win (%d)\n",  
				    strerror(-rc), rc);
			}
		}

		if (!(sender_info->flags & IPMSG_SECRETOPT)) {
			append_opened_window(window);
			text_view = GTK_TEXT_VIEW(
				lookup_widget(GTK_WIDGET(window),
				    "viewwindowTextView"));
		}

		gtk_widget_show(GTK_WIDGET(window));

		append_displayed_window(window);

		play_sound();
	}
}

int 
append_opened_window(GtkWindow *window) {

  g_assert(window);

  dbg_out("Append openlist:%x\n",(unsigned int)window);
  g_static_mutex_lock(&openwin_mutex);
  opened_wins=g_list_append(opened_wins,window);
  g_static_mutex_unlock(&openwin_mutex);

  return 0;
}
int 
destroy_all_opened_windows(void) {
  GList *node;
  GtkWidget *window;

  g_static_mutex_lock(&openwin_mutex);
  while((node=g_list_first (opened_wins))) {
    window=node->data;
    g_assert(window);
    opened_wins=g_list_remove (opened_wins,node);   /*  参照のみなのでここではデータは開放しない  */
    g_static_mutex_unlock(&openwin_mutex);
    gtk_widget_destroy(window);
    g_static_mutex_lock(&openwin_mutex);
  }
  g_static_mutex_unlock(&openwin_mutex);

  return 0;
}
int
remove_window_from_openlist(GtkWidget *window){

  g_static_mutex_lock(&openwin_mutex);
  dbg_out("Remove %x from openlist\n",(unsigned int)window);
  /*  参照のみなのでここではデータは開放しない  */
  opened_wins=g_list_remove(opened_wins,window);
  g_static_mutex_unlock(&openwin_mutex);  

  return 0;
}
int 
append_displayed_window(GtkWindow *window) {

  g_assert(window);

  dbg_out("Append displayed windows list:%x\n",(unsigned int)window);
  g_static_mutex_lock(&displayedwin_mutex);
  if (g_list_find(displayed_wins,window)) {
    dbg_out("Already exist %x\n",(unsigned int)window);
    goto unlock_out;
  }
  displayed_wins=g_list_append(displayed_wins,window);
 unlock_out:
  g_static_mutex_unlock(&displayedwin_mutex);

  return 0;
}
int
remove_window_from_displaylist(GtkWidget *window){

  g_static_mutex_lock(&displayedwin_mutex);
  dbg_out("Remove %x from displaylist\n",(unsigned int)window);
  if (g_list_find(displayed_wins,window))   /*  参照のみなのでここではデータは開放しない  */
    displayed_wins=g_list_remove(displayed_wins,window);
  g_static_mutex_unlock(&displayedwin_mutex);  

  return 0;
}
void
present_all_displayed_windows(void) {
  GList *node;
  GtkWidget *window;

  g_static_mutex_lock(&displayedwin_mutex);
  for(node=g_list_first (displayed_wins);
      node;
      node=g_list_next (node)) {
    window=node->data;
    g_assert(window);
    gtk_window_present (GTK_WINDOW(window));
  }
  g_static_mutex_unlock(&displayedwin_mutex);

  return ;
}

static void
on_pixmap_destroy(GtkObject *object, gpointer user_data)
{
 //  dbg_out("here %x\n",object);
}
static GtkWidget*
lookup_widget_with_applet(GtkWidget       *widget,
			  const gchar     *widget_name) {
  GtkWidget *found_widget;

  found_widget = (GtkWidget*) g_object_get_data (G_OBJECT (widget),
                                                 widget_name);
  if (!found_widget)
    found_widget=lookup_widget(widget,widget_name);
  if (!found_widget)
    g_warning ("Widget not found: %s", widget_name);

  return found_widget;
}
static gboolean 
default_message_watcher(GtkWidget *window,gboolean is_normal) {
  GtkWidget *startBtn;
  GtkWidget *current_image;
  GtkWidget *new_image;

  g_assert(window);
  current_image=lookup_widget_with_applet(window,"image3");
  g_assert(current_image);

  startBtn = lookup_widget_with_applet(window,"startBtn");
  g_assert(startBtn);

  g_signal_connect ((gpointer) current_image, "destroy",
                    G_CALLBACK (on_pixmap_destroy),NULL);


  if  (is_normal) 
    new_image=normal_image;
  else
    new_image=reverse_image;

  gtk_widget_show (new_image);
  //dbg_out("remove:%x\n",current_image);
  gtk_container_remove (GTK_CONTAINER (startBtn), current_image);
  gtk_container_add (GTK_CONTAINER (startBtn), new_image);
  //dbg_out("new object relation:%x => %x\n",current_image,new_image);
  GLADE_HOOKUP_OBJECT (window, new_image, "image3");

}

static gboolean 
message_watcher(gpointer data) {
  static gboolean is_normal=TRUE;
  GtkWidget *init_win=data;

  g_assert(init_win);

  g_source_remove (init_win_timer_id);

  if ( (is_normal) && 
       (!recv_windows_are_stored()) && 
       (!hostinfo_is_ipmsg_absent()) )
    goto source_set_out;

  if ( (!is_normal) && 
       (!recv_windows_are_stored()) && 
       (hostinfo_is_ipmsg_absent()) )
    goto source_set_out;

  g_assert(normal_image);
  g_assert(reverse_image);

  if  (is_normal) 
    is_normal=FALSE;
  else
    is_normal=TRUE;

#if GTK_CHECK_VERSION(2,10,0)
  if ((GTK_IS_STATUS_ICON(init_win))) 
    systray_message_watcher(init_win,is_normal);
  else
#endif
    default_message_watcher(init_win,is_normal);

 source_set_out:
  init_win_timer_id=g_timeout_add(MSG_WATCH_INTERVAL,message_watcher,init_win);  

  return TRUE; 
}

int
start_message_watcher(GtkWidget *init_win){
  if (!init_win)
    return -EINVAL;

  normal_image = create_pixmap (init_win, "g2ipmsg/ipmsg.xpm");
  reverse_image = create_pixmap (init_win, "g2ipmsg/ipmsgrev.xpm");

  g_assert(normal_image);
  g_assert(reverse_image);

  gtk_widget_ref (normal_image);
  gtk_widget_ref (reverse_image);

  dbg_out("Start message watcher \n");
  init_win_timer_id=g_timeout_add(MSG_WATCH_INTERVAL,message_watcher,init_win);

  return 0;
}

int
cleanup_message_watcher(void){

  dbg_out("End message watcher \n");
  gtk_widget_unref (normal_image);
  gtk_widget_unref (reverse_image);
  g_source_remove (init_win_timer_id);
  dbg_out("Cleanuped message watcher \n");
  return 0;
}

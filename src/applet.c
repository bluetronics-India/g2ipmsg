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

GThread *ui_thread;
GThread *tcp_thread;
static GtkWidget *applet_p=NULL;

#define GLADE_HOOKUP_OBJECT(component,widget,name) \
  g_object_set_data_full (G_OBJECT (component), name, \
    gtk_widget_ref (widget), (GDestroyNotify) gtk_widget_unref)

#define GLADE_HOOKUP_OBJECT_NO_REF(component,widget,name) \
  g_object_set_data (G_OBJECT (component), name, widget)

static void
g2ipmsg_applet_preferences_cb (BonoboUIComponent *uic,
                               GtkWidget        *applet,
                               const gchar       *cname)
{
  dbg_out("here\n");
  on_preferences1_activate(GTK_MENU_ITEM(applet),NULL);
}
static void
g2ipmsg_applet_security_preferences_cb(BonoboUIComponent *uic,
                               GtkWidget        *applet,
                               const gchar       *cname)
{
  dbg_out("here\n");
  on_menu_security_configuration_activate(GTK_MENU_ITEM(applet), NULL);
}
static void
g2ipmsg_applet_new_message_cb (BonoboUIComponent *uic,
                               GtkWidget        *applet,
                               const gchar       *cname)
{
  dbg_out("here\n");
  on_startBtn_clicked (GTK_BUTTON(applet),(gpointer)cname);
}
static void
g2ipmsg_applet_download_cb (BonoboUIComponent *uic,
                               GtkWidget        *applet,
                               const gchar       *cname)
{
  on_create_download_monitor();
}
static void
g2ipmsg_applet_remove_window_cb (BonoboUIComponent *uic,
                               GtkWidget        *applet,
                               const gchar       *cname)
{
  dbg_out("here\n");
  destroy_all_opened_windows();
}
static void
g2ipmsg_applet_foreground_window_cb (BonoboUIComponent *uic,
                               GtkWidget        *applet,
                               const gchar       *cname)
{
  dbg_out("here\n");
  present_all_displayed_windows();
}
static void
g2ipmsg_applet_about_cb (BonoboUIComponent *uic,
                               GtkWidget        *applet,
                               const gchar       *cname)
{
  dbg_out("here\n");
  ipmsg_show_about_dialog();
}
static void
g2ipmsg_applet_showlog_cb (BonoboUIComponent *uic,
                               GtkWidget        *applet,
                               const gchar       *cname){
  dbg_out("here\n");
  show_ipmsg_log();
}
static void
g2ipmsg_applet_fuzai_cb (BonoboUIComponent *uic,
                               GtkWidget        *applet,
                               const gchar       *cname)
{
  GtkWidget *window;

  dbg_out("here\n");

  window=internal_create_fuzai_editor();
  gtk_widget_show(window);

}
static void
g2ipmsg_applet_zaiseki_cb (BonoboUIComponent *uic,
                               GtkWidget        *applet,
                               const gchar       *cname)
{
  dbg_out("here\n");
  hostinfo_set_ipmsg_absent(FALSE);
  ipmsg_send_br_absence(udp_con,0);
}
static void
g2ipmsg_applet_destroy_cb (BonoboUIComponent *uic,
                               GtkWidget        *applet,
                               const gchar       *cname)
{
  dbg_out("here\n");
  cleanup_message_watcher();
  cleanup_ipmsg();
  release_lock_file();
}
static gboolean
applet_event_button_press_event (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{

  dbg_out("buttonWin: button press: %d\n", event->button);
  switch (event->button) {
  case 1:
    on_startBtn_clicked (GTK_BUTTON(widget),user_data);
  default:	
    return FALSE;
    break;
  }
  return TRUE;
}

static const BonoboUIVerb g2ipmsg_applet_menu_verbs[] = {
        BONOBO_UI_UNSAFE_VERB ("G2ipmsgNewMessage",
                               g2ipmsg_applet_new_message_cb),
	BONOBO_UI_UNSAFE_VERB ("G2ipmsgDownLoadMonitor",
                               g2ipmsg_applet_download_cb),
	BONOBO_UI_UNSAFE_VERB ("G2ipmsgRemoveWindows",
                               g2ipmsg_applet_remove_window_cb),
	BONOBO_UI_UNSAFE_VERB ("G2ipmsgForegroundWindows",
			       g2ipmsg_applet_foreground_window_cb),
        BONOBO_UI_UNSAFE_VERB ("G2ipmsgPreferences",
                               g2ipmsg_applet_preferences_cb),
        BONOBO_UI_UNSAFE_VERB ("G2ipmsgSecurityPreferences",
                               g2ipmsg_applet_security_preferences_cb),
        BONOBO_UI_UNSAFE_VERB ("G2ipmsgAppletAbout",
                               g2ipmsg_applet_about_cb),
        BONOBO_UI_UNSAFE_VERB ("G2ipmsgAppletShowLog",
                               g2ipmsg_applet_showlog_cb),
        BONOBO_UI_UNSAFE_VERB ("G2ipmsgAppletFuzai",
                               g2ipmsg_applet_fuzai_cb),
	BONOBO_UI_UNSAFE_VERB ("G2ipmsgAppletZaiseki",
                               g2ipmsg_applet_zaiseki_cb),
        BONOBO_UI_VERB_END
};

void
g2ipmsg_start_btn_update_tooltips (int num)
{
  char *tooltip_string;
  GtkTooltips *tooltips;

  if (!applet_p)
    return;

  tooltips=GTK_TOOLTIPS(lookup_widget(applet_p,"tooltip"));
  if (!tooltips) {
    err_out("Can not find tooltip\n");
    return;
  }
  GLADE_HOOKUP_OBJECT_NO_REF (applet_p, tooltips, "tooltip");

  tooltip_string = g_strdup_printf ("%s\n%s %d", _("G2IPMSG Applet"),_("users:"), num);
  if (!tooltip_string)
    goto tooltip_free_out;

  gtk_tooltips_set_tip (tooltips, applet_p,tooltip_string , NULL);

  g_free(tooltip_string);
 tooltip_free_out:
  g_free (tooltips);

  return;
}

static gboolean
g2ipmsg_applet_fill (PanelApplet *applet,
		   const gchar *iid,
		   gpointer     data)
{
	GdkPixbuf *icon_pixbuf;
	GtkWidget *startBtn;
	GtkWidget *image;
	GtkWidget *hbox;
	GtkTooltips *tooltips;
	char *pixmap_path = NULL;
	int rc;

	if (create_lock_file()) {
	  return FALSE;
	}
	hostinfo_init_hostinfo();

	syslog(LOG_ERR|LOG_USER,"Here\n");
        if (strcmp (iid, "OAFIID:G2ipmsgApplet") != 0)
		return FALSE;

	
	rc=init_ipmsg();
	if (rc<0) {
	  syslog(LOG_ERR|LOG_USER,"Can not init %s \n",PACKAGE);
	  return FALSE;
	}
	tooltips = gtk_tooltips_new ();
	hbox=gtk_hbox_new(FALSE,1);

	image = create_pixmap(GTK_WIDGET(applet), "g2ipmsg/ipmsg.xpm");

	gtk_widget_show (image);

	gtk_box_pack_start(GTK_BOX(hbox),image,TRUE,FALSE,0);
	gtk_container_add (GTK_CONTAINER (applet), hbox);
	gtk_widget_show (hbox);
	gtk_widget_show_all (GTK_WIDGET (applet));
	gtk_tooltips_set_tip (tooltips, GTK_WIDGET(applet),_("G2IPMSG Applet") , NULL);

	gtk_widget_set_events (GTK_WIDGET(applet),GDK_BUTTON_PRESS_MASK);
	gtk_signal_connect (GTK_OBJECT (applet), "button_press_event",
		      GTK_SIGNAL_FUNC (applet_event_button_press_event),
		      NULL);

	g_signal_connect (GTK_WIDGET(applet),
			  "destroy",
			  G_CALLBACK (g2ipmsg_applet_destroy_cb),
			  applet);

        panel_applet_setup_menu_from_file (PANEL_APPLET (applet),
                                           DATADIR,
                                           "g2ipmsg.xml",
                                           NULL,
                                           g2ipmsg_applet_menu_verbs,
                                           applet);

	/* Store pointers to all widgets, for use by lookup_widget(). */
	/* Note:They should have same name of initial window. */
	GLADE_HOOKUP_OBJECT_NO_REF (applet, applet, "GladeParentKey");
	GLADE_HOOKUP_OBJECT (applet, hbox, "startBtn");
	GLADE_HOOKUP_OBJECT (applet, image, "image3");
	GLADE_HOOKUP_OBJECT_NO_REF (applet, tooltips, "tooltip");

	tcp_thread=g_thread_create(ipmsg_tcp_server_thread,
				   (gpointer )hostinfo_get_ipmsg_system_addr_family(),
				   FALSE,
				   NULL);
	start_message_watcher(GTK_WIDGET(applet));
	applet_p=GTK_WIDGET(applet);
	dbg_out("applet init ok\n");

        return TRUE;
}


int
main (int argc, char *argv [])
{
  GnomeProgram *program;
  GOptionContext *context;
  int           retval;

  bindtextdomain (PACKAGE, GNOMELOCALEDIR);
  bind_textdomain_codeset (PACKAGE, "UTF-8");
  textdomain (PACKAGE);
  context = g_option_context_new ("");
  program = gnome_program_init ("GNOME2 IPMSG Applet", VERSION,
				LIBGNOMEUI_MODULE,
				argc, argv,	
				GNOME_PARAM_GOPTION_CONTEXT, context,
				GNOME_PROGRAM_STANDARD_PROPERTIES,
				GNOME_CLIENT_PARAM_SM_CONNECT, FALSE,
				GNOME_PARAM_APP_DATADIR, PACKAGE_DATA_DIR,
				NULL);
  retval = panel_applet_factory_main ("OAFIID:G2ipmsgApplet_Factory", PANEL_TYPE_APPLET, g2ipmsg_applet_fill, NULL);
  g_object_unref (program);

  return retval;
}


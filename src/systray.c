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

#if GTK_CHECK_VERSION(2,10,0)

#define GLADE_HOOKUP_OBJECT(component,widget,name) \
  g_object_set_data_full (G_OBJECT (component), name, \
    gtk_widget_ref (widget), (GDestroyNotify) gtk_widget_unref)

#define GLADE_HOOKUP_OBJECT_NO_REF(component,widget,name) \
  g_object_set_data (G_OBJECT (component), name, widget)

static void
on_statusIcon_activate(GtkStatusIcon *status_icon,
		       gpointer       user_data) 
{
  dbg_out("here\n");
  /* We need following cast to avoid compiler warnings. */
  on_startBtn_clicked ((GtkButton *)status_icon,NULL);
}
static void
on_statusIcon_popupmenu(GtkStatusIcon *status_icon,
		       guint          button,
		       guint          activate_time,
		       gpointer       user_data)
{
  dbg_out("here\n");
  gtk_menu_popup(GTK_MENU(create_main_menu()), NULL, NULL, 
		 gtk_status_icon_position_menu, 
		 status_icon, 
		 button,activate_time);		 

}
gboolean 
systray_message_watcher(GtkWidget *status_icon,gboolean is_normal) {
  GtkStatusIcon *icon=(GtkStatusIcon *)status_icon;
  GdkPixbuf *statusIcon_icon_pixbuf=NULL;

  g_assert(icon);

  if (recv_windows_are_stored()) {
      if  (is_normal) {
	statusIcon_icon_pixbuf = create_pixbuf ("g2ipmsg/ipmsg.xpm");
      }else{
	statusIcon_icon_pixbuf = create_pixbuf ("g2ipmsg/ipmsgrev.xpm");
      }
  }else{
    gtk_status_icon_set_blinking(icon,FALSE);
    if (hostinfo_is_ipmsg_absent()) {
      statusIcon_icon_pixbuf = create_pixbuf ("g2ipmsg/ipmsgrev.xpm");
    }else{
      statusIcon_icon_pixbuf = create_pixbuf ("g2ipmsg/ipmsg.xpm");
    }
  }
  if (statusIcon_icon_pixbuf) {
    gtk_status_icon_set_from_pixbuf(icon,statusIcon_icon_pixbuf);
    gdk_pixbuf_unref (statusIcon_icon_pixbuf);
  }

  return TRUE; 
}
static gboolean
systray_event_button_press_event (GtkWidget       *widget,
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

GtkWidget *
create_ipmsg_status_icon(void){
  GtkStatusIcon *statusIcon = NULL;
  GdkPixbuf *statusIcon_icon_pixbuf = NULL;

  statusIcon_icon_pixbuf = create_pixbuf ("g2ipmsg/ipmsg.xpm");
  if (statusIcon_icon_pixbuf)
    {
      statusIcon = gtk_status_icon_new_from_pixbuf(statusIcon_icon_pixbuf);
      gdk_pixbuf_unref (statusIcon_icon_pixbuf);
    }

  g_signal_connect ((gpointer) statusIcon, "activate",
                    G_CALLBACK (on_statusIcon_activate),
                    NULL);
  g_signal_connect ((gpointer) statusIcon, "popup-menu",
                    G_CALLBACK (on_statusIcon_popupmenu),
                    NULL);

  gtk_status_icon_set_tooltip(statusIcon,_("G2ipmsg"));

  /* Store pointers to all widgets, for use by lookup_widget(). */
  GLADE_HOOKUP_OBJECT_NO_REF (statusIcon, statusIcon, "statusIcon");

  return (GtkWidget *)statusIcon;
}
#endif /*  GTK_CHECK_VERSION(2,10,0) */

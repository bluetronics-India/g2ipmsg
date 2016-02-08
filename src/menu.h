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

#if !defined(MENU_H)
#define MENU_H
#include <bonobo.h>
#include <gnome.h>

gboolean on_init_win_event_button_press_event (GtkWidget       *widget,
					       GdkEventButton  *event,
					       gpointer         user_data);

GtkWidget *create_main_menu(void);
int show_ipmsg_log(void);
void on_mainmenu_new_message_item(gpointer menuitem);
int download_monitor_add_waiter_window(GtkWidget *window);
int download_monitor_remove_waiter_window(GtkWidget *window);
int download_monitor_update_state(void);
void on_create_download_monitor(void);
GtkWidget* internal_create_viewConfigWindow (void);
void download_monitor_release_file(const pktno_t pktno,int fileid);
void release_download_info(gpointer data, gpointer user_data);
int  download_monitor_update_state_from_thread(void);
int download_monitor_delete_btn_action(GtkButton *button, gpointer user_data);
GtkWidget *create_menu_item(const char *name,
			    const char *label,
			    gpointer data,
			    void (*menu_item_callback_function)
			    (gpointer user_data));
#endif /*  MENU_H  */

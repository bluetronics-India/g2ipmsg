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

#ifndef UI_COMMON_H
#define UI_COMMON_H

#include <glib.h>

#define USER_VIEW_NICKNAME_ID    (0)
#define USER_VIEW_GROUP_ID       (1)
#define USER_VIEW_HOST_ID        (2)
#define USER_VIEW_IPADDR_ID      (3)
#define USER_VIEW_LOGON_ID       (4)
#define USER_VIEW_PRIO_ID        (5)
#define MAX_VIEW_ID              (6)

#define UICOMMON_DIALOG_DELIM    ":"

#define ipmsg_err_dialog(fmt,arg...) do{                                     \
    error_message_dialog(FALSE, __FILE__,__FUNCTION__,__LINE__, fmt, ##arg); \
  }while(0)

#define ipmsg_err_dialog_mordal(fmt,arg...) do{                              \
    error_message_dialog(TRUE, __FILE__,__FUNCTION__,__LINE__, fmt, ##arg);  \
  }while(0)


void recv_message_window(const msg_data_t *msg,const char *senderAddr);
void store_message_window(const msg_data_t *msg,const char *senderAddr);
void read_message_dialog(const gchar *user,const gchar *ipaddr, long time);
void error_message_dialog(gboolean is_mordal, const char *filename, const char *funcname, 
			  const int line, const char* format, ...);
int password_setting_window(int type);
int password_confirm_window(int type, gchar **passphrase_p);

gboolean recv_windows_are_stored(void);
void show_stored_windows(void);
int start_message_watcher(GtkWidget *init_win);
int cleanup_message_watcher(void);
void update_one_group_entry(gpointer data,GtkComboBox *user_data);
int append_opened_window(GtkWindow *window);
int destroy_all_opened_windows(void);
int remove_window_from_openlist(GtkWidget *window);
int append_displayed_window(GtkWindow *window);
int remove_window_from_displaylist(GtkWidget *window);
void present_all_displayed_windows(void);
void ipmsg_show_about_dialog(void);
gboolean on_init_win_event_button_press_event (GtkWidget       *widget,
					       GdkEventButton  *event,
					       gpointer         user_data);
void update_download_view(GtkWidget *window);
void update_download_monitor_button(GtkTreeSelection *sel);

void ipmsg_update_ui_user_list(void);
void update_user_entry(GList *top,GtkWidget *view,gboolean is_force) ;
void ipmsg_update_ui(void);
void userview_set_view_priority(GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, int prio);
void userview_set_view_priority_without_update(GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, int prio);
int remind_headers_state(GtkWidget *window);
void on_usermenu_group_item (gpointer data);
int info_message_window(const gchar *user,const gchar *ipaddr, unsigned long command,const char *message);

void update_rsa_encryption_button_state(GtkToggleButton *togglebutton);
void update_lockkey_button_state(GtkToggleButton *togglebutton);
int apply_crypt_config_window(GtkWindow *window);
void ipmsg_wait_ms(const int wait_ms);
#endif  /*  UI_COMMON_H  */

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

#include <gnome.h>


void
on_app1_drag_data_received             (GtkWidget       *widget,
                                        GdkDragContext  *drag_context,
                                        gint             x,
                                        gint             y,
                                        GtkSelectionData *data,
                                        guint            info,
                                        guint            time,
                                        gpointer         user_data);

gboolean
on_app1_drag_drop                      (GtkWidget       *widget,
                                        GdkDragContext  *drag_context,
                                        gint             x,
                                        gint             y,
                                        guint            time,
                                        gpointer         user_data);

gboolean
on_app1_destroy_event                  (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data);

void
on_app1_destroy                        (GtkObject       *object,
                                        gpointer         user_data);

void
on_new1_activate                       (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_quit1_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_preferences1_activate               (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_update1_activate                    (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_update2_activate                    (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_about1_activate                     (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_sendBtn_released                    (GtkButton       *button,
                                        gpointer         user_data);

void
on_sendBtn_pressed                     (GtkButton       *button,
                                        gpointer         user_data);

void
on_sendBtn_clicked                     (GtkButton       *button,
                                        gpointer         user_data);

void
on_encloseCheckBtn_toggled             (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_comboboxentry1_editing_done         (GtkCellEditable *celleditable,
                                        gpointer         user_data);

void
on_configOpenCheckChkBtn_toggled       (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_configNonPopupCheckBtn_toggled      (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_configNoSoundCheckBtn_toggled       (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_configEncloseEnableCheckBtn_toggled (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_configCitationCheckBtn_toggled      (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_configLoggingEnableCheckBtn_toggled (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_configUserEnableCheckBtn_toggled    (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_logFileDialogBtn_pressed            (GtkButton       *button,
                                        gpointer         user_data);

void
on_logFileDialogBtn_released           (GtkButton       *button,
                                        gpointer         user_data);

void
on_logFileDialogBtn_clicked            (GtkButton       *button,
                                        gpointer         user_data);

void
on_entry1_editing_done                 (GtkCellEditable *celleditable,
                                        gpointer         user_data);

void
on_configAddBcastBtn_pressed           (GtkButton       *button,
                                        gpointer         user_data);

void
on_configAddBcastBtn_released          (GtkButton       *button,
                                        gpointer         user_data);

void
on_configAddBcastBtn_clicked           (GtkButton       *button,
                                        gpointer         user_data);

void
on_configRemoveBcastBtn_pressed        (GtkButton       *button,
                                        gpointer         user_data);

void
on_configRemoveBcastBtn_released       (GtkButton       *button,
                                        gpointer         user_data);

void
on_configRemoveBcastBtn_clicked        (GtkButton       *button,
                                        gpointer         user_data);

void
on_configApplySettingBtn_clicked       (GtkButton       *button,
                                        gpointer         user_data);

void
on_configApplySettingBtn_pressed       (GtkButton       *button,
                                        gpointer         user_data);

void
on_configApplySettingBtn_released      (GtkButton       *button,
                                        gpointer         user_data);

void
on_configCancelBtn_pressed             (GtkButton       *button,
                                        gpointer         user_data);

void
on_configCancelBtn_released            (GtkButton       *button,
                                        gpointer         user_data);

void
on_configCancelBtn_clicked             (GtkButton       *button,
                                        gpointer         user_data);

void
on_startBtn_clicked                    (GtkButton       *button,
                                        gpointer         user_data);

void
on_attachment1_activate                (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_quit1_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_entry2_activate                     (GtkEntry        *entry,
                                        gpointer         user_data);

gboolean
on_downloadWindow_destroy_event        (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data);

void
on_clist4_click_column                 (GtkCList        *clist,
                                        gint             column,
                                        gpointer         user_data);

void
on_downloadSaveBtn_clicked             (GtkButton       *button,
                                        gpointer         user_data);

void
on_close_button1_clicked               (GtkButton       *button,
                                        gpointer         user_data);

void
on_configWindow_destroy                (GtkObject       *object,
                                        gpointer         user_data);

void
on_initialWindow_destroy               (GtkObject       *object,
                                        gpointer         user_data);

void
on_downloadWindow_destroy              (GtkObject       *object,
                                        gpointer         user_data);

void
on_downloadDialog_destroy              (GtkObject       *object,
                                        gpointer         user_data);

void
on_messageWindow_drag_data_get         (GtkWidget       *widget,
                                        GdkDragContext  *drag_context,
                                        GtkSelectionData *data,
                                        guint            info,
                                        guint            time,
                                        gpointer         user_data);

void
on_messageWindow_drag_end              (GtkWidget       *widget,
                                        GdkDragContext  *drag_context,
                                        gpointer         user_data);

void
on_downloadfilechooserdialog_destroy   (GtkObject       *object,
                                        gpointer         user_data);

GtkFileChooserConfirmation
on_downloadfilechooserdialog_confirm_overwrite
                                        (GtkFileChooser  *filechooser,
                                        gpointer         user_data);

void
on_downloadfilechooserdialog_current_folder_changed
                                        (GtkFileChooser  *filechooser,
                                        gpointer         user_data);

void
on_downloadfilechooserdialog_file_activated
                                        (GtkFileChooser  *filechooser,
                                        gpointer         user_data);

void
on_downloadfilechooserdialog_update_preview
                                        (GtkFileChooser  *filechooser,
                                        gpointer         user_data);

void
on_downloadfilechooserdialog_close     (GtkDialog       *dialog,
                                        gpointer         user_data);

void
on_downloadfilechooserdialog_response  (GtkDialog       *dialog,
                                        gint             response_id,
                                        gpointer         user_data);

void
on_logfileChooserdialog_destroy        (GtkObject       *object,
                                        gpointer         user_data);

void
on_logfileChooserdialog_close          (GtkDialog       *dialog,
                                        gpointer         user_data);

void
on_logfileChooserdialog_response       (GtkDialog       *dialog,
                                        gint             response_id,
                                        gpointer         user_data);

void
on_aboutdialog_destroy                 (GtkObject       *object,
                                        gpointer         user_data);

void
on_aboutdialog_close                   (GtkDialog       *dialog,
                                        gpointer         user_data);

void
on_aboutdialog_response                (GtkDialog       *dialog,
                                        gint             response_id,
                                        gpointer         user_data);

void
on_readNotifyDialog_close              (GtkDialog       *dialog,
                                        gpointer         user_data);

void
on_readNotifyDialog_response           (GtkDialog       *dialog,
                                        gint             response_id,
                                        gpointer         user_data);

void
on_readNotifyDialog_destroy            (GtkObject       *object,
                                        gpointer         user_data);

void
on_readNotifyDialogOKBtn_pressed       (GtkButton       *button,
                                        gpointer         user_data);

gboolean
on_messageUserTree_select_cursor_row   (GtkTreeView     *treeview,
                                        gboolean         start_editing,
                                        gpointer         user_data);

gboolean
on_messageUserTree_drag_drop           (GtkWidget       *widget,
                                        GdkDragContext  *drag_context,
                                        gint             x,
                                        gint             y,
                                        guint            time,
                                        gpointer         user_data);

void
on_messageWindow_show                  (GtkWidget       *widget,
                                        gpointer         user_data);

void
on_viewWindow_destroy                  (GtkObject       *object,
                                        gpointer         user_data);

void
on_viewWindow_show                     (GtkWidget       *widget,
                                        gpointer         user_data);

void
on_initialWindow_show                  (GtkWidget       *widget,
                                        gpointer         user_data);

void
on_viewwindowCloseBtn_clicked          (GtkButton       *button,
                                        gpointer         user_data);

void
on_viewwindowReplyBtn_clicked          (GtkButton       *button,
                                        gpointer         user_data);

void
on_viewwindowCitationCheck_toggled     (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_comboboxentry1_editing_done         (GtkCellEditable *celleditable,
                                        gpointer         user_data);

void
on_comboboxentry1_set_focus_child      (GtkContainer    *container,
                                        GtkWidget       *widget,
                                        gpointer         user_data);

void
on_entry1_activate                     (GtkEntry        *entry,
                                        gpointer         user_data);

void
on_entry3_activate                     (GtkEntry        *entry,
                                        gpointer         user_data);

void
on_entry3_editing_done                 (GtkCellEditable *celleditable,
                                        gpointer         user_data);

void
on_configWindow_show                   (GtkWidget       *widget,
                                        gpointer         user_data);

void
on_comboboxentry1_editing_done         (GtkCellEditable *celleditable,
                                        gpointer         user_data);

void
on_configWindow_clicked                (GtkButton       *button,
                                        gpointer         user_data);

void
on_configWindowAddGroupBtn_clicked     (GtkButton       *button,
                                        gpointer         user_data);

void
on_loginNameLoggingToggle_toggled      (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_logIPAddrToggle_toggled             (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_enableLogToggle_toggled             (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_sendFailDialog_show                 (GtkWidget       *widget,
                                        gpointer         user_data);

void
on_sendFailDialog_destroy              (GtkObject       *object,
                                        gpointer         user_data);

void
on_sendFailDialog_response             (GtkDialog       *dialog,
                                        gint             response_id,
                                        gpointer         user_data);

void
on_sendFailDialog_close                (GtkDialog       *dialog,
                                        gpointer         user_data);

void
on_cancelbutton1_clicked               (GtkButton       *button,
                                        gpointer         user_data);

void
on_okbutton1_clicked                   (GtkButton       *button,
                                        gpointer         user_data);

void
on_messageUserTree_drag_data_received  (GtkWidget       *widget,
                                        GdkDragContext  *drag_context,
                                        gint             x,
                                        gint             y,
                                        GtkSelectionData *data,
                                        guint            info,
                                        guint            time,
                                        gpointer         user_data);

void
on_close1_activate                     (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_attachFileEditor_destroy            (GtkObject       *object,
                                        gpointer         user_data);

void
on_attachFileEditor_show               (GtkWidget       *widget,
                                        gpointer         user_data);

void
on_AttachFilePathEntry_activate        (GtkEntry        *entry,
                                        gpointer         user_data);

void
on_AttachFileFileChooseBtn_clicked     (GtkButton       *button,
                                        gpointer         user_data);

void
on_AttachFIleAddBtn_clicked            (GtkButton       *button,
                                        gpointer         user_data);

void
on_AttachFileRemoveBtn_clicked         (GtkButton       *button,
                                        gpointer         user_data);

void
on_attachedFilesView_drag_data_received
                                        (GtkWidget       *widget,
                                        GdkDragContext  *drag_context,
                                        gint             x,
                                        gint             y,
                                        GtkSelectionData *data,
                                        guint            info,
                                        guint            time,
                                        gpointer         user_data);

void
on_AttachFilePathEntry_changed         (GtkEditable     *editable,
                                        gpointer         user_data);

void
on_DownLoadOKBtn_clicked               (GtkButton       *button,
                                        gpointer         user_data);

void
on_DownLoadCancelBtn_clicked           (GtkButton       *button,
                                        gpointer         user_data);

void
on_DownLoadOpenBtn_clicked             (GtkButton       *button,
                                        gpointer         user_data);

void
on_messageSeparator_destroy            (GtkObject       *object,
                                        gpointer         user_data);

void
on_messageSeparator_drag_data_received (GtkWidget       *widget,
                                        GdkDragContext  *drag_context,
                                        gint             x,
                                        gint             y,
                                        GtkSelectionData *data,
                                        guint            info,
                                        guint            time,
                                        gpointer         user_data);

void
on_textview1_drag_data_received        (GtkWidget       *widget,
                                        GdkDragContext  *drag_context,
                                        gint             x,
                                        gint             y,
                                        GtkSelectionData *data,
                                        guint            info,
                                        guint            time,
                                        gpointer         user_data);

void
on_attachFileEditorMainFrame_drag_data_received
                                        (GtkWidget       *widget,
                                        GdkDragContext  *drag_context,
                                        gint             x,
                                        gint             y,
                                        GtkSelectionData *data,
                                        guint            info,
                                        guint            time,
                                        gpointer         user_data);

void
on_AttachFilePathEntry_drag_data_received
                                        (GtkWidget       *widget,
                                        GdkDragContext  *drag_context,
                                        gint             x,
                                        gint             y,
                                        GtkSelectionData *data,
                                        guint            info,
                                        guint            time,
                                        gpointer         user_data);

void
on_entry3_changed                      (GtkEditable     *editable,
                                        gpointer         user_data);

void
on_downloadMonitor_destroy             (GtkObject       *object,
                                        gpointer         user_data);

void
on_downloadMonitor_show                (GtkWidget       *widget,
                                        gpointer         user_data);

void
on_deleteBtn_clicked                   (GtkButton       *button,
                                        gpointer         user_data);

void
on_closeBtn_clicked                    (GtkButton       *button,
                                        gpointer         user_data);

void
on_updateBtn_clicked                   (GtkButton       *button,
                                        gpointer         user_data);

void
on_closeBtn_clicked                    (GtkButton       *button,
                                        gpointer         user_data);

void
on_messageWindow_check_resize          (GtkContainer    *container,
                                        gpointer         user_data);

void
on_initialWindow_check_resize          (GtkContainer    *container,
                                        gpointer         user_data);

void
on_downloadWindow_check_resize         (GtkContainer    *container,
                                        gpointer         user_data);

void
on_aboutdialog_check_resize            (GtkContainer    *container,
                                        gpointer         user_data);

void
on_viewWindow_check_resize             (GtkContainer    *container,
                                        gpointer         user_data);

void
on_attachFileEditor_check_resize       (GtkContainer    *container,
                                        gpointer         user_data);

void
on_downloadMonitor_check_resize        (GtkContainer    *container,
                                        gpointer         user_data);


void
on_initialWindow_size_allocate         (GtkWidget       *widget,
                                        GdkRectangle    *allocation,
                                        gpointer         user_data);

void
on_downloadWindow_size_allocate        (GtkWidget       *widget,
                                        GdkRectangle    *allocation,
                                        gpointer         user_data);

void
on_aboutdialog_size_allocate           (GtkWidget       *widget,
                                        GdkRectangle    *allocation,
                                        gpointer         user_data);

void
on_viewWindow_size_allocate            (GtkWidget       *widget,
                                        GdkRectangle    *allocation,
                                        gpointer         user_data);

void
on_attachFileEditor_size_allocate      (GtkWidget       *widget,
                                        GdkRectangle    *allocation,
                                        gpointer         user_data);

void
on_downloadMonitor_size_allocate       (GtkWidget       *widget,
                                        GdkRectangle    *allocation,
                                        gpointer         user_data);

void
on_messageWindow_size_allocate         (GtkWidget       *widget,
                                        GdkRectangle    *allocation,
                                        gpointer         user_data);

void
on_textview1_selection_received        (GtkWidget       *widget,
                                        GtkSelectionData *data,
                                        guint            time,
                                        gpointer         user_data);

void
on_textview1_set_anchor                (GtkTextView     *textview,
                                        gpointer         user_data);

gboolean
on_textview1_button_press_event        (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

gboolean
on_viewwindowTextView_button_press_event
                                        (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

void
on_absenceEditor_destroy               (GtkObject       *object,
                                        gpointer         user_data);

void
on_absenceEditor_show                  (GtkWidget       *widget,
                                        gpointer         user_data);

void
on_button7_clicked                     (GtkButton       *button,
                                        gpointer         user_data);

void
on_button8_clicked                     (GtkButton       *button,
                                        gpointer         user_data);

void
on_applyAndAbsentBtn_clicked           (GtkButton       *button,
                                        gpointer         user_data);

void
on_applyOnlyBtn_clicked                (GtkButton       *button,
                                        gpointer         user_data);

void
on_applyOnlyBtn_clicked                (GtkButton       *button,
                                        gpointer         user_data);

void
on_configIPV6CheckBtn_toggled          (GtkToggleButton *togglebutton,
                                        gpointer         user_data);


void
on_DownloadConfirmDialog_show          (GtkWidget       *widget,
                                        gpointer         user_data);

void
on_DownloadConfirmDialog_destroy       (GtkObject       *object,
                                        gpointer         user_data);

void
on_DownLoadConfirmClose_clicked        (GtkButton       *button,
                                        gpointer         user_data);

void
on_DownLoadConfirmExec_clicked         (GtkButton       *button,
                                        gpointer         user_data);

gboolean
on_messageWindow_key_release_event     (GtkWidget       *widget,
                                        GdkEventKey     *event,
                                        gpointer         user_data);

void
on_messageUserTree_drag_data_received  (GtkWidget       *widget,
                                        GdkDragContext  *drag_context,
                                        gint             x,
                                        gint             y,
                                        GtkSelectionData *data,
                                        guint            info,
                                        guint            time,
                                        gpointer         user_data);

void
on_textview1_drag_data_received        (GtkWidget       *widget,
                                        GdkDragContext  *drag_context,
                                        gint             x,
                                        gint             y,
                                        GtkSelectionData *data,
                                        guint            info,
                                        guint            time,
                                        gpointer         user_data);

gboolean
on_textview1_button_press_event        (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

void
on_absenceEditorCloseBtn_clicked       (GtkButton       *button,
                                        gpointer         user_data);

void
on_messageUserTree_drag_data_received  (GtkWidget       *widget,
                                        GdkDragContext  *drag_context,
                                        gint             x,
                                        gint             y,
                                        GtkSelectionData *data,
                                        guint            info,
                                        guint            time,
                                        gpointer         user_data);

void
on_messageWinUpdateBtn_clicked         (GtkButton       *button,
                                        gpointer         user_data);

void
on_sendBtn_clicked                     (GtkButton       *button,
                                        gpointer         user_data);

void
on_encloseCheckBtn_toggled             (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_vpaned1_drag_data_received          (GtkWidget       *widget,
                                        GdkDragContext  *drag_context,
                                        gint             x,
                                        gint             y,
                                        GtkSelectionData *data,
                                        guint            info,
                                        guint            time,
                                        gpointer         user_data);

void
on_AttachFileFileChooseBtn_clicked     (GtkButton       *button,
                                        gpointer         user_data);

void
on_AttachFilePathEntry_activate        (GtkEntry        *entry,
                                        gpointer         user_data);

void
on_AttachFilePathEntry_changed         (GtkEditable     *editable,
                                        gpointer         user_data);

void
on_AttachFilePathEntry_drag_data_received
                                        (GtkWidget       *widget,
                                        GdkDragContext  *drag_context,
                                        gint             x,
                                        gint             y,
                                        GtkSelectionData *data,
                                        guint            info,
                                        guint            time,
                                        gpointer         user_data);

void
on_viewwindowCitationCheck_toggled     (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_absenceEditor_size_allocate         (GtkWidget       *widget,
                                        GdkRectangle    *allocation,
                                        gpointer         user_data);

void
on_messageWinCloseBtn_clicked          (GtkButton       *button,
                                        gpointer         user_data);

void
on_groupInverseBtn_clicked             (GtkButton       *button,
                                        gpointer         user_data);

void
on_secondKeyInverseBtn_clicked         (GtkButton       *button,
                                        gpointer         user_data);

void
on_configApplyBtn_clicked              (GtkButton       *button,
                                        gpointer         user_data);

void
on_set_priority1_activate              (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_set_priority_as_2_activate          (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_set_priority_as_3_activate          (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_set_priority_as_4_activate          (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_set_as_default_activate             (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_set_invisible_item_activate         (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_show_invisible_items1_activate      (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_set_all_as_default_activate         (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_groupselection_activate             (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_attachmentfile_activate             (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_set_user_list_view_config_activate  (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_groupInverseBtn_toggled             (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_secondKeyInverseBtn_toggled         (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_save_list_headers_state1_activate   (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_viewConfigCloseBtn_clicked          (GtkButton       *button,
                                        gpointer         user_data);

void
on_secondKeyUserRBtn_toggled           (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_secondKeyMachineRBtn_toggled        (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_secondKeyIPAddrRBtn_toggled         (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_get_version1_activate               (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_absence_info1_activate              (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_clientInfoOKBtn_clicked             (GtkButton       *button,
                                        gpointer         user_data);

void
on_sort_filter1_activate               (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_menu_set_priority_as_1_activate     (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_menu_set_priority_as_2_activate     (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_menu_set_priority_as_3_activate     (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_menu_set_priority_as_4_activate     (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_menu_set_them_as_default_activate   (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_menu_set_them_invisible_item_activate
                                        (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_menu_show_invisible_items_activate  (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_menu_set_all_as_default_activate    (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_menu_user_list_view_config_activate (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_menu_save_list_headers_state_activate
                                        (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_pubkeyPWDBtn_clicked                (GtkButton       *button,
                                        gpointer         user_data);

void
on_lockPWDBtn_clicked                  (GtkButton       *button,
                                        gpointer         user_data);

void
on_configSecurityOKBtn_clicked         (GtkButton       *button,
                                        gpointer         user_data);

void
on_configSecurityCancelBtn_clicked     (GtkButton       *button,
                                        gpointer         user_data);

void
on_passwordEntry_activate              (GtkEntry        *entry,
                                        gpointer         user_data);

void
on_passWordDialogCancelBtn_clicked     (GtkButton       *button,
                                        gpointer         user_data);

void
on_passWordDialogCOKBtn_clicked        (GtkButton       *button,
                                        gpointer         user_data);

void
on_security_config_activate            (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_decodeFailDialogOKButton_clicked    (GtkButton       *button,
                                        gpointer         user_data);

void
on_errorDialogCloseBtn_pressed         (GtkButton       *button,
                                        gpointer         user_data);

void
on_menu_security_configuration_activate
                                        (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_passWordOKBtn_clicked               (GtkButton       *button,
                                        gpointer         user_data);

void
on_PassWordCancelBtn_clicked           (GtkButton       *button,
                                        gpointer         user_data);

void
on_passwdEntry1_changed                (GtkEditable     *editable,
                                        gpointer         user_data);

void
on_passwdEntry2_changed                (GtkEditable     *editable,
                                        gpointer         user_data);

void
on_currentPassWordEntry_changed        (GtkEditable     *editable,
                                        gpointer         user_data);

void
on_RSAKeyEncryptionCBtn_toggled        (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_useLockCBtn_toggled                 (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_lockChkBtn_toggled                  (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_encloseCheckBtn_toggled             (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

gboolean
on_readNotifyDialog_window_state_event (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data);

void
on_readNotifyWindowOkBtn_clicked       (GtkButton       *button,
                                        gpointer         user_data);

void
on_enableLogToggle_toggled             (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

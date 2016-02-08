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

static void
onAbsenceSelectionChanged (GtkTreeSelection *sel,gpointer user_data)
{
  GtkTreeIter  selected_row;
  GtkWidget *text_view;
  GtkWidget *view;
  GtkWidget *entry;
  GtkTreeModel *model;
  GtkTextBuffer *buffer;
  gint index;
  gchar *title;
  gchar *message;

  view=GTK_WIDGET(gtk_tree_selection_get_tree_view(sel));
  g_assert(view);
  g_assert(gtk_tree_selection_get_mode(sel) == GTK_SELECTION_SINGLE);
  entry=GTK_WIDGET(lookup_widget(view,"AbsenceTitleEntry"));
  g_assert(entry);
  text_view=GTK_WIDGET(lookup_widget(view,"fuzaiText"));
  g_assert(text_view);

  if (gtk_tree_selection_get_selected(sel, &model, &selected_row))
    {
      gtk_tree_model_get (model, &selected_row, 
                          1, &index,
                          -1);
      dbg_out("Selected index:%d\n",index);
      if (!hostinfo_get_absent_title(index,(const char **)&title)) {
	gtk_entry_set_text(GTK_ENTRY(entry), title); 
	g_free(title);
      }
      if (!hostinfo_get_absent_message(index, &message)) {
	buffer=gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
	gtk_text_buffer_set_text(buffer,message,-1);
	g_free(message);
      }
    }
  else
    {
      g_assert_not_reached();
    }
}
static int
setup_fuzai_view(GtkTreeView *view) {
  GList *node;
  GtkTreeModel *model;
  GtkListStore        *liststore;
  GtkTreeViewColumn   *col;
  GtkCellRenderer     *renderer;
  GtkTreeSelection   *sel;

  if  (!view)
    return -EINVAL;

  liststore = gtk_list_store_new(2, 
				 G_TYPE_STRING,
				 G_TYPE_INT);

  g_assert(liststore);

  gtk_tree_view_set_model(GTK_TREE_VIEW(view), GTK_TREE_MODEL(liststore));
  g_object_unref(liststore); /* bind to view */

    /*
   * タイトル設定
   */
  /* --- Column #1 --- */

  col = gtk_tree_view_column_new();

  gtk_tree_view_column_set_title(col, _("title"));

  /* pack tree view column into tree view */
  gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);

  renderer = gtk_cell_renderer_text_new();

  /* pack cell renderer into tree view column */
  gtk_tree_view_column_pack_start(col, renderer, TRUE);

  /* connect 'text' property of the cell renderer to
   *  model column that contains the title */
  gtk_tree_view_column_add_attribute(col, renderer, "text", 0);

  gtk_tree_view_column_set_resizable (col,TRUE);

  sel=gtk_tree_view_get_selection(GTK_TREE_VIEW(view));
  gtk_tree_selection_set_mode(sel, GTK_SELECTION_SINGLE );
  g_signal_connect(sel, "changed", 
		   G_CALLBACK(onAbsenceSelectionChanged),
		   NULL);

  return 0;
}
GtkWidget *
internal_create_fuzai_editor(void){
  GtkWidget *window;
  GtkWidget *titleView;
  GtkWidget *entry;
  GtkWidget *text_view;
  int max_index;
  int max_message;
  int index;
  int rc;
  gchar *title;
  gchar *message;
  GtkTreeModel *model;
  GtkTreeIter   newrow;
  GtkTreeSelection   *sel;
  GtkTextBuffer *buffer;
  gint width,height;

  rc=hostinfo_refer_absent_length(&max_index);
  if (rc<0)
    goto error_out;
  rc=hostinfo_refer_absent_message_slots(&max_message);
  if (rc<0)
    goto error_out;

  if (max_index>max_message)
    max_index=max_message;

  window=create_absenceEditor();
  g_assert(window);
  titleView=GTK_WIDGET(lookup_widget(window,"absenseTitles"));
  g_assert(titleView);
  entry=GTK_WIDGET(lookup_widget(window,"AbsenceTitleEntry"));
  g_assert(entry);
  text_view=GTK_WIDGET(lookup_widget(window,"fuzaiText"));
  g_assert(text_view);

  setup_fuzai_view(GTK_TREE_VIEW(titleView));
  for(index=0;index<max_index;++index) {
    hostinfo_get_absent_title(index,(const char **)&title);

    model = gtk_tree_view_get_model(GTK_TREE_VIEW(titleView));
      
    gtk_list_store_append(GTK_LIST_STORE(model), &newrow);
      
    gtk_list_store_set(GTK_LIST_STORE(model), &newrow, 
		       0, title,
		       1, index,
		       -1);
    g_free(title);
  }
  sel=gtk_tree_view_get_selection(GTK_TREE_VIEW(titleView));
  gtk_tree_model_get_iter_first(model, &newrow);
  gtk_tree_selection_select_iter (sel, &newrow);

  hostinfo_get_absent_title(0,(const char **)&title);
  gtk_entry_set_text(GTK_ENTRY(entry), title); 
  g_free(title);

  hostinfo_get_absent_message(0, &message);
  buffer=gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
  gtk_text_buffer_set_text(buffer,message,-1);
  g_free(message);

  if (!hostinfo_get_ipmsg_attach_editor_size(&width,&height)){
    dbg_out("Resize:(%d,%d)\n",width,height);
    gtk_window_resize (GTK_WINDOW(window),width,height);
  }


  return window;
 error_out:
  return NULL;
}

int
update_fuzai_config(GtkWidget *view,gboolean enter) {
  GtkWidget *window;
  GtkWidget *entry;
  GtkWidget *text_view;
  int max_index;
  int max_message;
  int index;
  int rc;
  gchar *title;
  gchar *message;
  GtkTreeModel *model;
  GtkTreeIter   titer;
  GtkTextIter   siter;
  GtkTextIter   eiter;
  GtkTreeSelection   *sel;
  GtkTextBuffer *txt_buf;
  size_t len;
  int text_line;

  if (!view)
    return -EINVAL;

  rc=0;
  entry=GTK_WIDGET(lookup_widget(view,"AbsenceTitleEntry"));
  g_assert(entry);
  text_view=GTK_WIDGET(lookup_widget(view,"fuzaiText"));
  g_assert(text_view);

  sel=gtk_tree_view_get_selection(GTK_TREE_VIEW(view));
  if (gtk_tree_selection_get_selected(sel, &model, &titer))
    {
      gtk_tree_model_get (model, &titer, 
                          1, &index,
                          -1);
      dbg_out("Index:%d\n",index);

      title=(char *)gtk_entry_get_text(GTK_ENTRY(entry));
      rc=hostinfo_set_ipmsg_absent_title(index,title);
      if (rc<0)
	goto error_out;

      txt_buf=gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
      gtk_text_buffer_get_bounds(txt_buf,&siter,&eiter);
      message=gtk_text_buffer_get_text(txt_buf,&siter,&eiter,FALSE);
      rc=hostinfo_set_ipmsg_absent_message(index,message);
      g_free(message); /*成功,不成功にかかわらず開放は実施 */
      if (rc<0)
	goto error_out;

      if (enter) { /* 不在モードへ移行  */
	hostinfo_set_absent_id(index);
	hostinfo_set_ipmsg_absent(TRUE);
	ipmsg_send_br_absence(udp_con,0);
      }
      rc=0;
    }  
 error_out:
  return rc;
}

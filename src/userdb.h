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

#if !defined(USERDB_H)
#define USERDB_H
#include <gnome.h>
#include <glib.h>
#include <string.h>
#include "udp.h"
#include "message.h"

#define make_entry_canonical(the_string) ((strncmp((the_string),HOSTLIST_DUMMY,1))?(the_string):(_("UnKnown")))


typedef struct _userdb{
  gchar *user;
  gchar *host;
  gchar *group;
  gchar *nickname;
  gchar *ipaddr;
  int    prio;
  unsigned long cap; /*  端末ケイパビリティ(エントリパケットのフラグ)  */
  unsigned long crypt_cap; /*  端末ケイパビリティ(エントリパケットのフラグ)  */
  gchar *pub_key_e;  /* hexフォーマット(bigendian)の文字列  */
  gchar *pub_key_n;  /* hexフォーマット(bigendian)の文字列  */
  int    pf;         /* プロトコルファミリ  */
}userdb_t;
int userdb_init_userdb(void);
int userdb_del_user(const udp_con_t *con,const msg_data_t *msg);
int userdb_add_user(const udp_con_t *con,const msg_data_t *msg);
int userdb_update_user(const udp_con_t *con,const msg_data_t *msg);
int userdb_search_user_by_addr(const char *ipaddr,const userdb_t **entry_ref);
int destroy_user_info(userdb_t *entry);
void userdb_update_group_list(GtkComboBox *widget);
int userdb_get_hostlist_string(int start, int *length, const char **ret_string) ;
void userdb_print_user_list(void);
int update_users_on_message_window(GtkWidget *window,gboolean is_forced);
int userdb_invalidate_userdb(void);
int userdb_send_broad_cast(const udp_con_t *con, const char *msg,size_t len);
int userdb_replace_prio_by_addr(const char *ipaddr,int prio,gboolean need_notify);
void update_all_user_list_view(void);
int userdb_replace_public_key_by_addr(const char *ipaddr,const unsigned long peer_cap,const char *key_e,const char *key_n);
int userdb_get_public_key_by_addr(const char *ipaddr,unsigned long *cap_p,char **key_e,char **key_n);
int userdb_wait_public_key(const char *peer_addr,unsigned long *cap_p,char **key_e,char **key_n);
int userdb_get_cap_by_addr(const char *ipaddr, unsigned long *cap_p, unsigned long *crypt_cap_p);
int userdb_refer_proto_family(const char *ipaddr, int *family);
int userdb_cleanup_userdb(void);
GList *refer_user_list(void);
GList *get_group_list(void);
#endif /* USERDB_H  */

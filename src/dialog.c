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

/** @file 
 * @brief  ダイアログ関数群
 * @author Takeharu KATO
 */ 


/** クライアント情報ダイアログ用のメッセージ出力共通処理
 * @param[in] text_buffer  出力先テキストバッファ
 * @param[in] format       書式指定文字列
 * @return    なし
 */
static void
client_info_add_text(GtkTextBuffer *text_buffer, const char *format,...){
	char buffer[64];  /*  文字列整形用バッファ  */
	size_t      len;  /*  文字列長  */
	va_list      ap;  /*  可変長引数管理  */

	g_assert(text_buffer != NULL);

	memset(buffer, 0, 64);	  
	va_start(ap, format);
	vsnprintf(buffer, 63, format, ap);
	va_end(ap);
	buffer[63] = '\0';

	len = strlen(buffer);

	gtk_text_buffer_insert_at_cursor(text_buffer, buffer, len);
}

/** 受信確認ダイアログ生成
 * @param[in, out] user    ユーザ名(この関数内で開放することを想定)
 * @param[in] ipaddr       受信者IPアドレス(IPMSG_READMSG送信者アドレス, 開放不要)
 * @param[in] sec          IPMSG_READMSG受信時刻(Epoch 時刻)
 * @retval  0      
 * @retval -EINVAL 
 * @retval -ENOMEM 
 * @attention userが指し示す先は, 本関数内で開放するので, 呼び出し側で
 *            複製を引き渡すこと.
 */
void
read_message_dialog(const gchar *user,const gchar *ipaddr, long sec){
	GtkWidget   *dialog = NULL;
	GtkLabel *userlabel = NULL;
	GtkLabel *timelabel = NULL;
	userdb_t *user_info = NULL;
	char         buffer[64];
	char    time_string[26];
	va_list          ap;

	dbg_out("here\n");
	
	if (!hostinfo_refer_ipmsg_default_confirm())
		goto user_free_out;  /* 受信確認をしない場合は, 即時復帰  */

	dialog = GTK_WIDGET(create_readNotifyDialog());
	userlabel = 
		GTK_LABEL(lookup_widget(dialog, 
			"readNotifyDialogUserGroupLabel") );

	if (ipaddr != NULL) {
		if (!userdb_search_user_by_addr(ipaddr, 
			(const userdb_t **)&user_info) ) {

			g_assert(userlabel != NULL);

			snprintf(buffer, 63, "%s@%s (%s)", 
			    user_info->nickname, 
			    user_info->group, 
			    user_info->host);
			buffer[63]='\0';

			gtk_label_set_text(userlabel, buffer);

			g_assert(!destroy_user_info(user_info));

		} else {
			/* ipaddrがユーザDBに登録されていない場合, 
			 * ユーザ名だけを表示する.
			 */
			if (user != NULL)  
				gtk_label_set_text(userlabel, user);
			else
				gtk_label_set_text(userlabel, _("UnKnown"));
		}
	} else {
		gtk_label_set_text(userlabel, _("UnKnown"));
	}

	timelabel = GTK_LABEL(lookup_widget(dialog,
		"readNotifyLogonIPAddrLabel"));

	if (get_current_time_string(time_string, sec))
		gtk_label_set_text(timelabel, _("UnKnown"));
	else
		gtk_label_set_text(timelabel, time_string);

	if (hostinfo_refer_ipmsg_iconify_dialogs())
		gtk_window_iconify(GTK_WINDOW(dialog));
	else
		gtk_window_deiconify(GTK_WINDOW(dialog));

	gtk_widget_show(dialog);  /* ダイアログ表示 */

user_free_out:
	if (user != NULL)
		g_free((gchar *)user);

	return;
}

/** エラー発生ダイアログを出力する
 * @param[in] is_mordal モーダルダイアログを生成する
 * @param[in] filename  発生元ファイル名
 * @param[in] funcname  発生元関数名
 * @param[in] line      発生元行番号
 * @param[in] format    書式指定文字列(可変個引数)
 * @return    なし
 */
void
error_message_dialog(gboolean is_mordal, const char *filename, const char *funcname, 
    const int line, const char* format, ...){
	GtkWidget *dialog = NULL;
	GtkLabel  *file_label = NULL;
	GtkLabel  *func_label = NULL;
	GtkLabel  *line_label = NULL;
	GtkTextView *error_text_view = NULL;
	GtkTextBuffer *error_buffer = NULL;
	char error_message[256];
	char line_string[8];
	va_list ap;

	dbg_out("here\n");

	g_assert(filename != NULL);
	g_assert(funcname != NULL);

	/*
	 * エラーメッセージを整形する.
	 */
	if (format != NULL) {
		va_start(ap, format);
		vsnprintf(error_message, 256, format, ap);
		va_end(ap);
	} else {
		error_message[0] = '\0';
	}
	error_message[255] = '\0';

	snprintf(line_string, 8, "%d", line);
	line_string[7] = '\0';

	gdk_threads_enter();

	/*
	 * ウィジェットを取得
	 */
	dialog = GTK_WIDGET(create_errorDialog());
	g_assert(dialog);

	file_label = GTK_LABEL(lookup_widget(GTK_WIDGET(dialog),
		"errorDialogErrInfoFileNameContLabel"));
	g_assert(file_label);

	func_label = GTK_LABEL(lookup_widget(GTK_WIDGET(dialog),
		"errorDialogErrInfoFunctionContLabel"));
	g_assert(func_label);

	line_label = GTK_LABEL(lookup_widget(GTK_WIDGET(dialog),
		"errorDialogErrInfoFileLineContLabel"));
	g_assert(line_label);

	error_text_view = GTK_TEXT_VIEW(lookup_widget(GTK_WIDGET(dialog),
		"errorDialogErrorTextView"));
	g_assert(error_text_view);

	error_buffer = 
		GTK_TEXT_BUFFER(gtk_text_view_get_buffer(error_text_view));
	g_assert(error_buffer);

	/*
	 * エラー発生位置情報を設定
	 */
	gtk_label_set_text(file_label, filename);
	gtk_label_set_text(func_label, funcname);
	gtk_label_set_text(line_label, line_string);

	/*
	 * カット＆ペースト用に位置情報と発生内容を表示
	 */
	gtk_text_buffer_insert_at_cursor(error_buffer, _("FileName"), -1);
        gtk_text_buffer_insert_at_cursor(error_buffer, UICOMMON_DIALOG_DELIM, -1);
        gtk_text_buffer_insert_at_cursor(error_buffer, filename, -1);
        gtk_text_buffer_insert_at_cursor(error_buffer, "\n", -1);

        gtk_text_buffer_insert_at_cursor(error_buffer, _("Function"), -1);
        gtk_text_buffer_insert_at_cursor(error_buffer, UICOMMON_DIALOG_DELIM, -1);
        gtk_text_buffer_insert_at_cursor(error_buffer, funcname, -1);
        gtk_text_buffer_insert_at_cursor(error_buffer, "\n", -1);

        gtk_text_buffer_insert_at_cursor(error_buffer, _("Line"), -1);
        gtk_text_buffer_insert_at_cursor(error_buffer, UICOMMON_DIALOG_DELIM, -1);
        gtk_text_buffer_insert_at_cursor(error_buffer, line_string, -1);
        gtk_text_buffer_insert_at_cursor(error_buffer, "\n", -1);

        gtk_text_buffer_insert_at_cursor(error_buffer,  error_message, -1);
        gtk_text_buffer_insert_at_cursor(error_buffer,  "\n", -1);

	/*
	 * デバッグ可能なように, テキスト出力も表示
	 */
	dbg_out("file:%s func:%s line %s error:%s\n",
		filename,
		funcname,
		line_string,
		error_message);

	gtk_window_set_modal(GTK_WINDOW(dialog), is_mordal); /* モーダル設定  */	
	/*
	 * ダイアログ表示
	 */
	if (is_mordal) {
		gtk_dialog_run(GTK_DIALOG(dialog));
	} else {
		gtk_widget_show(dialog);
		ipmsg_update_ui();  /*  ダイアログを表示  */
	}



	gdk_threads_leave();

	return;
}

/** クライアント情報ダイアログを表示する
 * @param[in] user      ユーザ名
 * @param[in] ipaddr    IPアドレス
 * @param[in] command   情報種別
 *                        - IPMSG_SENDINFO         クライアント情報を表示
 *                        - IPMSG_SENDABSENCEINFO  不在情報を表示
 * @param[in] message   表示する文章
 * @retval    0         正常終了
 * @retval   -EINVAL    messageがNULLである.
 * @retval   -EINVAL    不正なcommandを引き渡した.
 */
int
info_message_window(const gchar *user,const gchar *ipaddr, 
    unsigned long command,const char *message){
	GtkWidget          *window = NULL;  /*  ダイアログウィンドウ  */
	GtkLabel        *userlabel = NULL;  /*  ユーザ名表示ラベル  */
	GtkLabel        *typelabel = NULL;  /*  情報種別表示ラベル  */
	userdb_t        *user_info = NULL;  /*  ユーザ情報          */
	char            buffer[64];         /*  文字列整形用バッファ  */
	GtkTextView     *text_view = NULL;  /*  表示領域用テキストビュー  */
	GtkTextBuffer *text_buffer = NULL;  /*  表示領域のテキストバッファ */
	gchar    *internal_message = NULL;  /*  表示文字列の内部形式格納領域  */
	size_t                 len = 0;     /*  表示文字列長  */
	unsigned long         skey = 0;     /*  対称鍵暗号処理能力  */
	unsigned long         akey = 0;     /*  非対称(公開鍵)暗号処理能力  */
	unsigned long         sign = 0;     /*  署名処理能力  */
	int                     rc = 0;     /*  返り値  */

	dbg_out("here\n");

	rc = -EINVAL;
	if (message == NULL)
		goto error_out;

	/*
	 *  ウィジェット獲得
	 */
	window = GTK_WIDGET(create_clientInfoWindow());
	g_assert(window);

	userlabel = GTK_LABEL(lookup_widget(window, "clientInfoUserLabel"));
	g_assert(userlabel);

	typelabel = GTK_LABEL(lookup_widget(window, "clientInfoTypeLabel"));
	g_assert(typelabel);

	text_view = GTK_TEXT_VIEW(lookup_widget(GTK_WIDGET(window), 
		"clientInfoTextview"));
	g_assert(text_view);

	text_buffer = gtk_text_view_get_buffer(text_view);
	g_assert(text_buffer);

	/*
	 * 種別判定
	 */
	switch(command) {
	case IPMSG_SENDINFO:
		gtk_label_set_text(typelabel, _("Version Information"));
		break;
	case IPMSG_SENDABSENCEINFO:
		gtk_label_set_text(typelabel, _("Absence Information"));
		break;
	default:
		rc = -EINVAL;
		goto error_out;
		break;
	}

	/*
	 * 出力メッセージ整形
	 */
	rc = convert_string_internal(message, 
	    (const gchar **)&internal_message);
	if (internal_message == NULL) {
		ipmsg_err_dialog(_("Can not convert message from %s into ineternal representation"), ipaddr);
		goto error_out;
	}

	len = strlen(internal_message);
	gtk_text_buffer_set_text(text_buffer, internal_message, len);
	g_free(internal_message);

	if (command == IPMSG_SENDABSENCEINFO)
		goto show_out;  /*  不在文の場合は, クライアント情報は必要ない  */

	/*
	 * クライアント情報出力
	 */
	if ( (user != NULL) && (ipaddr != NULL) ) {
		if (!userdb_search_user_by_addr(ipaddr, 
			(const userdb_t **)&user_info)) {
			g_assert(userlabel);

			snprintf(buffer, 63, "%s@%s (%s)",
			    user_info->nickname, user_info->host, 
			    user_info->group);
			buffer[63] = '\0';
			gtk_label_set_text(userlabel, buffer);

			/*
			 * IPアドレス
			 */
			client_info_add_text(text_buffer, "\n");
			client_info_add_text(text_buffer, "%s [%s] : %s\n",
					     _("IP Address"), 
					     (user_info->pf == PF_INET) ?
					     ("IPV4") : ("IPV6"),
					     user_info->ipaddr);
			client_info_add_text(text_buffer, "%s : 0x%08x\n", 
			    _("Capability"), (unsigned int)user_info->cap);


			/*
			 * Dial up
			 */
			if (user_info->cap & IPMSG_DIALUPOPT)
				client_info_add_text(text_buffer, 
				    "%s : %s\n", _("Dial up"), _("YES"));
			else
				client_info_add_text(text_buffer, 
				    "%s : %s\n", _("Dial up"), _("NO"));
			/*
			 * UTF-8
			 */
			if (user_info->cap & IPMSG_UTF8OPT)
				client_info_add_text(text_buffer, 
				    "%s : %s\n", _("UTF-8"), _("YES"));
			else
				client_info_add_text(text_buffer, 
				    "%s : %s\n", _("UTF-8"), _("NO"));
			/*
			 * File Attach
			 */
			if (user_info->cap & IPMSG_FILEATTACHOPT)
				client_info_add_text(text_buffer, 
				    "%s : %s\n", _("File Attach"), _("YES"));
			else
				client_info_add_text(text_buffer, 
				    "%s : %s\n", _("File Attach"), _("NO"));

			/*
			 * Encryption
			 */
			
			if (user_info->cap & IPMSG_ENCRYPTOPT) {

				client_info_add_text(text_buffer, 
				    "%s:%s (%s:0x%08x)\n", 
				    _("Encryption Capability"), _("YES"),
				    _("Capability"), user_info->crypt_cap);
				
				akey = get_asymkey_part(user_info->crypt_cap);
				skey = get_symkey_part(user_info->crypt_cap);
				sign = get_sign_part(user_info->crypt_cap);

				/*
				 * Public key
				 */
				client_info_add_text(text_buffer, 
				    "%s:", _("Public key encryption"));

				if (akey != 0) {
					if (akey & IPMSG_RSA_512)
						client_info_add_text(text_buffer, "[%s]", _("RSA 512 bits"));

					if (akey & IPMSG_RSA_1024)
						client_info_add_text(text_buffer, "[%s]", _("RSA 1024 bits"));

					if (akey & IPMSG_RSA_2048)
						client_info_add_text(text_buffer, "[%s]", _("RSA 2048 bits"));

				} else {
					client_info_add_text(text_buffer, "[%s]", _("UnKnown"));
				}
				client_info_add_text(text_buffer, "\n");

				/*
				 * Symmetric key
				 */

				client_info_add_text(text_buffer, "%s:",
				    _("Symmetric key encryption"));

				if (skey != 0) {

					if (skey & IPMSG_RC2_40)
						client_info_add_text(text_buffer, "[%s]", _("RC2 40 bits"));

					if (skey & IPMSG_RC2_128)
						client_info_add_text(text_buffer, "[%s]", _("RC2 128 bits"));

					if (skey & IPMSG_RC2_256)
						client_info_add_text(text_buffer, "[%s]", _("RC2 256 bits"));

					if (skey & IPMSG_BLOWFISH_128)
						client_info_add_text(text_buffer, "[%s]", _("Blowfish 128 bits"));

					if (skey & IPMSG_BLOWFISH_256)
						client_info_add_text(text_buffer, "[%s]", _("Blowfish 256 bits"));

					if (skey & IPMSG_AES_128)
						client_info_add_text(text_buffer, "[%s]", _("AES 128 bits"));

					if (skey & IPMSG_AES_192)
						client_info_add_text(text_buffer, "[%s]", _("AES 192 bits"));

					if (skey & IPMSG_AES_256)
						client_info_add_text(text_buffer, "[%s]", _("AES 256 bits"));


				} else {
					client_info_add_text(text_buffer, 
					    "[%s]", _("UnKnown"));
				}
				client_info_add_text(text_buffer, "\n");

				/*
				 * Signature
				 */
				client_info_add_text(text_buffer, 
				    "%s:", _("Signature"));

				if (sign != 0) {

					if (sign & IPMSG_SIGN_MD5)
						client_info_add_text(text_buffer, "[%s]", _("MD5"));

					if (sign & IPMSG_SIGN_SHA1)
						client_info_add_text(text_buffer, "[%s]", _("SHA1"));
				} else {
					client_info_add_text(text_buffer, 
					    "[%s]", _("UnKnown"));
				}
				client_info_add_text(text_buffer, "\n");

			} else {
				client_info_add_text(text_buffer, 
				    "%s:%s\n", _("Encryption Capability"), 
				    _("NO"));
			}
		} else {
			gtk_label_set_text(userlabel, user);
		}
	} else {
		gtk_label_set_text(userlabel, (_("UnKnown")));
	}

show_out:
	gtk_widget_show(window);

	rc=0;

error_out:
	if (user_info != NULL)
		g_assert(!destroy_user_info(user_info));

	if (user != NULL)
		g_free((gchar *)user);

	return rc;
}

/** パスワード入力確認ダイアログ生成
 * @param[in] type    パスワード種別
 *                    - HOSTINFO_PASSWD_TYPE_ENCKEY 暗号鍵保存用
 *                    - HOSTINFO_PASSWD_TYPE_LOCK   錠開封用
 * @param[out] passphrase_p パスワード返却先
 *                        (暗号化鍵操作に使用. 使用しない場合, NULLを設定).
 * @retval  0         パスワード一致確認
 * @retval -EINVAL    不正なtypeをtypeに指定した.
 * @retval -EPERM     パスワード不整合, 入力キャンセル
 * @retval -ENOENT    パスワード未設定
 * @retval -ENOMEM    メモリ不足
 */
int
password_confirm_window(int type, gchar **passphrase_p) {
	int                                rc = 0;
	GtkDialog                     *dialog = NULL;
	GtkEntry                *passwd_entry = NULL;
	const gchar            *passwd_string = NULL;
	const char *configured_encoded_passwd = NULL;
	gchar              *duplicated_passwd = NULL;
	gint                           result = 0;
  
	/*
	 * 種別判定
	 */
	switch(type) {
	case HOSTINFO_PASSWD_TYPE_ENCKEY:
		configured_encoded_passwd = hostinfo_refer_encryption_key_password();
		break;
	case HOSTINFO_PASSWD_TYPE_LOCK:
		configured_encoded_passwd = hostinfo_refer_lock_password();
		break;
	default:
		rc = -EINVAL;
		goto error_out;
		break;
	}

	/*
	 * パスワードが設定されているか?
	 */
	if (pbkdf2_encoded_passwd_configured(configured_encoded_passwd)	!= 0) {
		rc = -ENOENT;
		goto error_out;
	}

	/*
	 * ダイアログ生成, 入力エントリ獲得
	 */
	dialog = GTK_DIALOG(create_passwdDialog());
	g_assert(dialog != NULL);

	passwd_entry = GTK_ENTRY(lookup_widget(GTK_WIDGET(dialog), 
		"passwordEntry"));
	g_assert(passwd_entry);
	
	/*
	 * ダイアログ表示
	 */
	result = gtk_dialog_run(GTK_DIALOG(dialog));
	switch (result) {
	case GTK_RESPONSE_OK:
		/*
		 * パスワード確認
		 */
		dbg_out("Accept for password\n");

		passwd_string = gtk_entry_get_text(passwd_entry);
		dbg_out("Entered:%s\n", passwd_string);
		
		rc = pbkdf2_verify(passwd_string, configured_encoded_passwd);
		if (rc != 0) {
			rc = -EPERM;  /* 不正パスワード  */
			break;
		}
		/*
		 * パスフレーズの複製を返却
		 */
		if (passphrase_p != NULL) {
		  duplicated_passwd = g_strdup(passwd_string);
		  if (duplicated_passwd == NULL) {
		    rc = -ENOMEM; /* メモリ不足  */
		    break;
		  }
		  /* パスフレーズ返却  */
		  *passphrase_p = duplicated_passwd;
		}
		break;
	case GTK_RESPONSE_CANCEL:
	default:
		dbg_out("response:%d cancel\n", result);
		rc = -EPERM;
		break;
	}

	gtk_widget_destroy(GTK_WIDGET(dialog));  /*  ダイアログを破棄する  */

error_out:
	return rc;
}

/** パスワード設定ダイアログ生成
 * @param[in] type    パスワード種別
 *                    HOSTINFO_PASSWD_TYPE_ENCKEY 暗号鍵保存用
 *                    HOSTINFO_PASSWD_TYPE_LOCK   錠開封用
 * @param[in] togglebutton
 * @retval  0         設定完了
 * @retval -ENOENT    設定キャンセル
 */
int
password_setting_window(const int type){
	GtkWindow                     *window = NULL;
	GtkButton                  *ok_button = NULL;
	GtkEntry              *current_passwd = NULL;
	GtkEntry                  *new_passwd = NULL;
	GtkEntry            *confirmed_passwd = NULL;
	GtkLabel       *password_window_label = NULL;
	ipmsg_private_data_t            *priv = NULL;
	const char *configured_encoded_passwd = NULL;
	int                                rc = 0;

	dbg_out("here\n");

	window = GTK_WINDOW(create_passwdWindow()); /* パスワード設定画面生成 */
	g_assert(window != NULL);

	/*
	 * ウィジェット獲得
	 */
	current_passwd = 
		GTK_ENTRY(lookup_widget(GTK_WIDGET(window), 
			"currentPassWordEntry"));
	new_passwd = 
		GTK_ENTRY(lookup_widget(GTK_WIDGET(window), 
			"passwdEntry1"));

	confirmed_passwd = 
		GTK_ENTRY(lookup_widget(GTK_WIDGET(window), 
			"passwdEntry2"));
	
	ok_button = 
		GTK_BUTTON(lookup_widget(GTK_WIDGET(window), 
			"passWordOKBtn"));

	password_window_label = GTK_LABEL(lookup_widget(GTK_WIDGET(window), 
		"passWordWindowLabel"));
	
	/*
	 * 初期値をクリアしておく
	 */
	gtk_entry_set_text(current_passwd, "");
	gtk_entry_set_text(new_passwd, "");
	gtk_entry_set_text(confirmed_passwd, "");

	/*
	 * 種別に基づく設定
	 */
	switch(type) {
	case HOSTINFO_PASSWD_TYPE_ENCKEY:
		gtk_label_set_text(password_window_label, _("Password for public/private keys"));
		configured_encoded_passwd = hostinfo_refer_encryption_key_password();
		break;
	case HOSTINFO_PASSWD_TYPE_LOCK:
		gtk_label_set_text(password_window_label, _("Password message for lock."));
		configured_encoded_passwd = hostinfo_refer_lock_password();
		break;
	default:
		g_assert_not_reached();
		break;
	}

	/* パスワード未設定場合, エントリを無効にする */
	if (pbkdf2_encoded_passwd_configured(configured_encoded_passwd) != 0)
		gtk_widget_set_sensitive(GTK_WIDGET(current_passwd), FALSE);
	else
		gtk_widget_set_sensitive(GTK_WIDGET(current_passwd), TRUE);

	/* OKボタンを無効にする */
	gtk_widget_set_sensitive(GTK_WIDGET(ok_button), FALSE);
	/*
	 * パスワード要求種別を設定する
	 */
	rc = init_ipmsg_private(&priv, IPMSG_PRIVATE_PASSWD);
	g_assert(rc == 0);
	priv->data = (void *)type;
	IPMSG_HOOKUP_DATA(window, priv, "passWordType");

	gtk_widget_show(GTK_WIDGET(window)); /*  パスワード設定ウィンドウを表示する  */
	
	return 0;
}

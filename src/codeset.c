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
 * @brief  文字コードセット操作
 * @author Takeharu KATO
 */ 

/** 文字コード変換排他処理用ロック変数
 */
GStaticMutex convert_mutex = G_STATIC_MUTEX_INIT;

/** サポートしているコードセット
 */
static char *supported_codes[]={
  "CP367",
  "UTF-8",
  "ISO-10646-UCS-2",
  "UCS-2BE",
  "UCS-2LE",
  "ISO-10646-UCS-4",
  "UCS-4BE",
  "UCS-4LE",
  "UTF-16",
  "UTF-16BE",
  "UTF-16LE",
  "UTF-32",
  "UTF-32BE",
  "UTF-32LE",
  "UTF-7",
  "UCS-2-INTERNAL",
  "UCS-2-SWAPPED",
  "UCS-4-INTERNAL",
  "UCS-4-SWAPPED",
  "C99",
  "JAVA",
  "CP819",
  "ISO-8859-2",
  "ISO-8859-3",
  "ISO-8859-4",
  "CYRILLIC",
  "ARABIC",
  "ECMA-118",
  "HEBREW",
  "ISO-8859-9",
  "ISO-8859-10",
  "ISO-8859-11",
  "ISO-8859-13",
  "ISO-8859-14",
  "ISO-8859-15",
  "ISO-8859-16",
  "KOI8-R",
  "KOI8-U",
  "KOI8-RU",
  "CP1250",
  "CP1251",
  "CP1252",
  "CP1253",
  "CP1254",
  "CP1255",
  "CP1256",
  "CP1257",
  "CP1258",
  "CP850",
  "CP862",
  "CP866",
  "MAC",
  "MACCENTRALEUROPE",
  "MACICELAND",
  "MACCROATIAN",
  "MACROMANIA",
  "MACCYRILLIC",
  "MACUKRAINE",
  "MACGREEK",
  "MACTURKISH",
  "MACHEBREW",
  "MACARABIC",
  "MACTHAI",
  "HP-ROMAN8",
  "NEXTSTEP",
  "ARMSCII-8",
  "GEORGIAN-ACADEMY",
  "GEORGIAN-PS",
  "KOI8-T",
  "CP154",
  "MULELAO-1",
  "CP1133",
  "ISO-IR-166",
  "CP874",
  "VISCII",
  "TCVN",
  "ISO-IR-14",
  "JISX0201-1976",
  "ISO-IR-87",
  "ISO-IR-159",
  "ISO646-CN",
  "ISO-IR-58",
  "CN-GB-ISOIR165",
  "EUC-JP",
  "EUC-JP-MS",
  "CP932",
  "ISO-2022-JP",
  "ISO-2022-JP-1",
  "ISO-2022-JP-2",
  "EUC-CN",
  "GBK",
  "CP936",
  "GB18030",
  "ISO-2022-CN",
  "ISO-2022-CN-EXT",
  "HZ-GB-2312",
  "EUC-TW",
  "BIG-5",
  "CP950",
  "BIG5-HKSCS:1999",
  "BIG5-HKSCS:2001",
  "BIG5-HKSCS",
  "EUC-KR",
  "CP949",
  "CP1361",
  "ISO-2022-KR",
  "CP856",
  "CP922",
  "CP943",
  "CP1046",
  "CP1124",
  "CP1129",
  "CP1161",
  "CP1162",
  "CP1163",
  "DEC-KANJI",
  "DEC-HANYU",
  "CP437",
  "CP737",
  "CP775",
  "CP852",
  "CP853",
  "CP855",
  "CP857",
  "CP858",
  "CP860",
  "CP861",
  "CP863",
  "CP864",
  "CP865",
  "CP869",
  "CP1125",
  "EUC-JISX0213",
  "SHIFT_JISX0213",
  "ISO-2022-JP-3",
  "BIG5-2003",
  "ISO-IR-230",
  "ATARI",
  "RISCOS-LATIN1",
  NULL /* 番兵 */
};
/** 指定されたエンコーディングが有効であることを確認する
 *  @param[in]      encoding  エンコーディング指定文字列
 *  @retval         0         正常終了
 *  @retval        -EINVAL    引数異常
 *  @retval        -ENOSYS    未サポート
 */
static int
check_valid_encoding(const char *encoding) {
  int rc = 0;
  GIConv  converter;

  if (encoding == NULL)
    return -EINVAL;

  /* 内部コードへの変換可否 */
  converter = g_iconv_open(IPMSG_INTERNAL_CODE, encoding);
  if (converter == (GIConv)-1)
    return -ENOSYS;  /* 未サポート  */
  g_iconv_close(converter);

  /* 外部コードへの変換可否 */
  converter = g_iconv_open(encoding, IPMSG_INTERNAL_CODE);
  if (converter == (GIConv)-1)
    return -ENOSYS;  /* 未サポート  */
  g_iconv_close(converter);
  

  return 0;
}

/** 文字コード一覧をComboBoxに登録する
 *  @param[out]      combobox  エンコーディング指定コンボボックス
 *  @retval  0       正常終了
 *  @retval -EINVAL 引数異常(comboboxがNULL)
 *  @retval -ENOMEM メモリ不足
 */
int
setup_encoding_combobox(GtkComboBox *combobox) {
  int                        i =  0;
  int                       rc =  0;
  int                    index = -1;
  int                  code_nr =  0;
  const char *current_encoding = NULL;

  if (combobox == NULL) {
    rc = -EINVAL;
    goto error_out;
  }

  /* デフォルトエンコードを先頭に持ってくる
   * 指定されたデフォルトエンコードが使用できなかった場合は,
   * Windows版のデフォルトであるCP932を使用する.
   */
  rc = check_valid_encoding(IPMSG_EXTERNAL_CHARCODE);
  if (rc == 0)
    gtk_combo_box_append_text(combobox, IPMSG_EXTERNAL_CHARCODE);
  else
    gtk_combo_box_append_text(combobox, IPMSG_WINIPMSG_PROTO_CODE);

  /*
   * 現在の設定値を確認
   */
  current_encoding = hostinfo_refer_encoding();
  if (!strcasecmp(IPMSG_EXTERNAL_CHARCODE, current_encoding))
    index = 0; /* 現在の設定値を設定  */

  for(i = 0, code_nr = 1; supported_codes[i] != NULL; ++i) {
    /*
     * エンコーディングの有効性を確認
     */
    rc = check_valid_encoding(supported_codes[i]);
    if (rc != 0) {
      err_out("Codeset:^s can not supported on this platform.\n", supported_codes[i]);
      continue; /* 使用不可能 */
    }

    /*
     * デフォルト以外を登録
     */
    if (strcasecmp(IPMSG_EXTERNAL_CHARCODE, supported_codes[i])) {
      gtk_combo_box_append_text(combobox, supported_codes[i]);

      /*
       * エンコードの設定値をデフォルトにする
       */
      if (!strcasecmp(supported_codes[i], current_encoding))
	index = code_nr; /* インデクスを設定  */
      ++code_nr;
    }
  }

  /* 現在のエンコードをアクティブにする  */
  if (index > 0)
    gtk_combo_box_set_active(combobox, index); 
  else
    gtk_combo_box_set_active(combobox, 0);

 success_out:
  rc = 0;

  error_out:
    return rc;
}
/** 文字コードを内部形式に変換する
 *  @param[in]  string    変換対象の文字列
 *  @param[out] to_string 変換後の文字列を指すポインタ変数のアドレス
 *  @retval  0       正常終了(string, to_stringのいずれかがNULL).
 *  @retval -EINVAL 引数異常
 *  @retval -ENOMEM メモリ不足
 */
int
convert_string_internal(const char *string, const gchar **to_string) {
	int                      rc = 0;
	gsize              read_len = 0;
	gsize             write_len = 0;
	GError          *error_info = NULL;
	gchar     *converted_string = NULL;
	const char *external_encode = NULL;

	if ( (string == NULL) || (to_string == NULL) )
		return -EINVAL;
	
	external_encode = hostinfo_refer_encoding(); /* 外部エンコード取得 */

	g_static_mutex_lock(&convert_mutex);
	converted_string = g_convert((const gchar *)string,
	    -1, /* ヌルターミネート */
	    IPMSG_INTERNAL_CODE,
	    (external_encode != NULL) ? (external_encode) : (IPMSG_PROTO_CODE),
	    &read_len,
	    &write_len,
	    &error_info);

	rc = -EINVAL;
	if (converted_string == NULL) {
		if (error_info != NULL) {
			dbg_out("%s\n",error_info->message);
			rc = error_info->code;
			g_error_free(error_info);
		}
		if (rc > 0)
			rc = -rc;
		g_static_mutex_unlock(&convert_mutex);
		goto error_out;
	}
	
	*to_string = converted_string;
	rc = 0; /* 正常終了  */

	g_static_mutex_unlock(&convert_mutex);

error_out:
	return rc;
}

/** 文字コードを外部形式に変換する
 *  @param[in]  ipaddr    変換後の文字列を送信する先を表すIPアドレス
 *                           - IPMSG_PROTOCOL_ENTRY_PKT_ADDR デフォルトコードセットへ変換
 *                           - 上記以外                      UTF-8ホストの場合UTF-8変換
 *  @param[in]  string    変換対象の文字列
 *  @param[out] to_string 変換後の文字列を指すポインタ変数のアドレス
 *  @retval  0       正常終了
 *  @retval -EINVAL 引数異常(string, to_stringのいずれかがNULL).
 *  @retval -ENOMEM メモリ不足
 */
int
ipmsg_convert_string_external(const char *ipaddr, const char *string, const gchar **to_string) {
	int                      rc = 0;
	ipmsg_cap_t        peer_cap = 0;
	gsize              read_len = 0;
	gsize             write_len = 0;
	ipmsg_cap_t  peer_crypt_cap = 0;
	GError          *error_info = NULL;
	gchar     *converted_string = NULL;
	const char *external_encode = NULL;

	if ( (string == NULL) || (to_string == NULL) )
		return -EINVAL;
	
#if defined(IPMSG_UTF8_SUPPORT)
	/*
	 * UTF-8ホスト対応
	 */
	if (ipaddr != IPMSG_PROTOCOL_ENTRY_PKT_ADDR) {
		/*
		 * ブロードキャスト系以外のパケットの場合, UTF-8送信を試みる
		 */
		rc = userdb_get_cap_by_addr(ipaddr, &peer_cap, &peer_crypt_cap);
		if (rc != 0) {
			goto start_default_encoding; /* 能力不明  */
		}

		if ( !(peer_cap & IPMSG_UTF8OPT) )
			goto start_default_encoding; /* 非UTF-8ホスト  */
		/*
		 * UTF-8で送信
		 */

		/* GNOME2の内部コードはUTF-8なので単にコピーする  */    
		converted_string = g_strdup(string); 
		if (converted_string == NULL) {
			rc = -ENOMEM;
			goto error_out;
		}

		goto convert_end;
	}
#endif  /*  IPMSG_UTF8_SUPPORT  */

start_default_encoding:	
	/*
	 * デフォルトエンコーディング
	 */

	external_encode = hostinfo_refer_encoding(); /* 外部エンコード取得 */

	g_static_mutex_lock(&convert_mutex);
	converted_string = g_convert((const gchar *)string,
	    -1, /* ヌルターミネート */
	    (external_encode != NULL) ? (external_encode) : (IPMSG_PROTO_CODE),
	    IPMSG_INTERNAL_CODE,
	    &read_len,
	    &write_len,
	    &error_info);

	g_static_mutex_unlock(&convert_mutex);

	rc = -EINVAL;
	if (converted_string == NULL) {
		if (error_info != NULL) {
			dbg_out("%s\n", error_info->message);
			rc = error_info->code;
			g_error_free(error_info);
		}
		if (rc > 0)
			rc = -rc;
		goto error_out;
	}
	
convert_end:
	*to_string = converted_string;
	rc = 0;

error_out:
	return rc;
}

/** 文字コードを内部形式に変換する
 *  @param[in]  ipaddr    変換後の文字列を送信する先を表すIPアドレス
 *                           - IPMSG_PROTOCOL_ENTRY_PKT_ADDR 
 *                                                         デフォルトコードセットから変換
 *                           - 上記以外                    UTF-8ホストの場合UTF-8から変換
 *  @param[in]  string    変換対象の文字列
 *  @param[out] to_string 変換後の文字列を指すポインタ変数のアドレス
 *  @retval  0       正常終了
 *  @retval -EINVAL 引数異常(string, to_stringのいずれかがNULL).
 *  @retval -ENOMEM メモリ不足
 */
int
ipmsg_convert_string_internal(const char *ipaddr, const char *string, const gchar **to_string) {
  return convert_string_internal(string,to_string);
}



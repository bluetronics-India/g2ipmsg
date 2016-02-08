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
 * @brief  雑多なユーティリティ関数群
 * @author Takeharu KATO
 */ 

/** スレッド間での環境変数アクセスへの排他用ロック
 * @attention 内部リンケージ
 */
GStaticMutex serialize_mutex = G_STATIC_MUTEX_INIT;

/** スレッド間での時刻管理情報アクセスへの排他用ロック
 * @attention 内部リンケージ
 */
GStaticMutex time_var_mutex = G_STATIC_MUTEX_INIT;

/** 環境変数の値を取得し, その複製を返却する.
 *  @param[in]  var    環境変数名
 *  @param[out] val_p  環境変数の内容を差すポインタのアドレス
 *  @retval  0       正常終了
 *  @retval -EINVAL 引数異常(var, val_pのいずれかがNULL).
 *  @retval -ENOMEM メモリ不足
 */
int
get_envval(const char *var, char **val_p) {
	char *val = NULL; 
	char *val_str = NULL;
	int rc = 0;

	if ( (var == NULL) || (val_p == NULL) ) 
		return -EINVAL;

	/* ただの気休めに過ぎないが, 一応排他 */
	g_static_mutex_lock(&serialize_mutex); 

	rc = -ENOENT;
	val = getenv(var);
	if (val == NULL)
		goto unlock_out;

	rc = -ENOMEM;
	val_str = g_strdup(val);
	if (val_str == NULL)
		goto unlock_out;

	*val_p=val_str;

	rc=0;

unlock_out:
	g_static_mutex_unlock(&serialize_mutex);

	return (rc);
}

/** 指定された時刻の文字列表現を返却する.
 *  @param[out] time_string  26文字長の時刻を返却する領域
 *  @param[in]  sec          epoch時刻
 *  @retval  0       正常終了
 *  @retval -ENOMEM メモリ不足
 *  @attention 時刻返却先(time_string)は, 26文字以上を格納できるように
 *             割り当て済みでなければならない.
 */
int
get_current_time_string(char time_string[], long sec) {
	struct tm recv_tm;
	char time_buffer[32];

#if defined(HAVE_LOCALTIME_R)
	localtime_r((time_t *)&sec, &recv_tm);
#else
	g_static_mutex_lock(&time_var_mutex);
	memmove(&recv_tm, localtime((time_t *)&sec), sizeof(struct tm));
	g_static_mutex_unlock(&time_var_mutex);
#endif  /*  HAVE_LOCALTIME_R  */

#if defined(HAVE_ASCTIME_R)
	asctime_r(&recv_tm, time_buffer);
#else
	g_static_mutex_lock(&time_var_mutex);
	memmove(time_buffer, asctime(&recv_tm), 26);
	g_static_mutex_unlock(&time_var_mutex);
#endif  /*  HAVE_ASCTIME_R  */
	time_buffer[25] = '\0'; /* 26文字以上値を埋めるlibcへの対処 */

	strncpy(time_string, time_buffer, 26);

	return 0;
}


/** メモリの割り当て領域を拡張し拡張した領域を0で埋める.
 *  @param[out] ptr          g_mallocで獲得した領域を保持するポインタのアドレス
 *  @param[in] new_size      再割り当て後のサイズ
 *  @param[in] old_size      現在のサイズ
 *  @retval  0       正常終了
 *  @retval -EINVAL 引数異常
 *  @retval -ENOMEM メモリ不足
 */
int
internal_realloc(void **ptr, size_t new_size, size_t old_size){
	void  *orig_ptr = NULL;
	ssize_t clr_len = 0;

	if (ptr == NULL)
		return -EINVAL;

	orig_ptr = *ptr;
	*ptr = g_realloc(*ptr, new_size);
  
	if ( (*ptr) == NULL ) {
		*ptr = orig_ptr;
		return -ENOMEM;
	}

	clr_len = new_size - old_size;

	if (clr_len > 0)
		memset( ((*ptr)+old_size), 0, clr_len);

	return 0;
}

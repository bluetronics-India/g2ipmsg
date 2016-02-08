/*
 * Following codes(particularly check_secure_directory function) 
 * are derived from 
 * ``Secure Programming Cookbook for C and C++''.
 * URL:http://www.oreilly.com/catalog/secureprgckbk/ 
 *     http://www.secureprogramming.com/
 * 
 * Licensing Information can be obtain from following URL:
 *
 *  http://www.secureprogramming.com/?action=license
 *
 * Copyright  2003 by John Viega and Matt Messier.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met: 

 * 1. Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 *  documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the developer of this software nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include "common.h"

/** @file 
 * @brief  公開鍵暗号関数群
 * @author Takeharu KATO
 */ 

/** スレッド間でのディレクトリ属性アクセス排他用ロック
 * @attention 内部リンケージ
 */
GStaticMutex  dir_check_mutex = G_STATIC_MUTEX_INIT;

/** スレッド間でのファイル属性アクセス排他用ロック
 * @attention 内部リンケージ
 */
GStaticMutex file_check_mutex = G_STATIC_MUTEX_INIT;

/** スレッド間でのRSA鍵アクセス排他用ロック
 * @attention 内部リンケージ
 */
GStaticMutex    rsa_key_mutex = G_STATIC_MUTEX_INIT;

/** RSA公開鍵鍵テーブル
 * @attention 内部リンケージ
 */
static RSA *rsa_keys[RSA_KEY_MAX];

/** 鍵種別インデクスから公開鍵暗号化能力IDへの変換テーブル
 * @attention 内部リンケージ
 */
static const int key2ipmsg_key_type[]={
	IPMSG_RSA_2048,
	IPMSG_RSA_1024,
	IPMSG_RSA_512,
	-1};

/** IPMSGの公開鍵暗号化能力IDからRSA公開鍵テーブルのインデクスを取得する.
 *  @param[in]  cap    ipmsgの公開鍵暗号化能力ID
 *  @param[out] indp   インデクス値変換領域のアドレス
 *  @retval  0       正常終了
 *  @retval -EINVAL 引数異常(capが不正なIDを示しているかindpがNULLである).
 *  @attention 内部リンケージ
 */
static int
get_rsa_key_index(const ipmsg_cap_t cap, int *indp){
	int    rc = 0;
	int index = 0;

	if ( (indp == NULL) ||  (!(cap & RSA_CAPS)) )
		return -EINVAL;

	switch(cap) {
	case IPMSG_RSA_512:
		index = RSA_KEY_INDEX_512;
		break;
	case IPMSG_RSA_1024:
		index = RSA_KEY_INDEX_1024;
		break;
	case IPMSG_RSA_2048:
		index = RSA_KEY_INDEX_2048;
		break;
	default:
		g_assert_not_reached();
		break;
	}

	*indp = index;

	return 0;
}

/** RSA鍵を格納したファイルを読み取り用に開く
 *  @param[out]  fd_p  開いたファイルを操作するためのファイル記述子格納領域のアドレス
 *  @param[in]   file  RSA鍵ファイルのファイル名
 *  @retval  0       正常終了
 *  @retval -EINVAL  引数異常(fd_pまたはfileがNULLである).
 *  @retval -EPERM   以下のいずれかが発生した.
 *                   - ファイルに対するアクセス権がない
 *                   - rootが所有するファイルである,
 *                   - 自身が所有するファイルでない.
 *                   - シンボリックリンクである
 *  @attention 内部リンケージ
 */
static int
open_key_file_for_read(int *fd_p, const unsigned char *file) {
	int    rc = 0;
	int    fd = 0;
	uid_t uid = 0;
	struct stat fbuff,lbuff;


	if ( (fd_p == NULL) || (file == NULL) )
		return -EINVAL;

	g_static_mutex_lock(&file_check_mutex); /* 厳密にいえば不要 */

	fd = open(file, O_RDONLY);
	if (fd < 0) {
		rc = -errno;
		goto unlock_out;
	}

	rc = fstat(fd, &fbuff);
	if (rc < 0) {
		rc = -errno;
		goto error_out;
	}

	rc = lstat(file,&lbuff);
	if (rc < 0) {
		rc = -errno;
		goto error_out;
	}

	uid = geteuid();

	/*
	 * 偏執的ではあるが, リンクでないことを確認する
	 */
	rc = -EPERM;
	if ( (lbuff.st_mode != fbuff.st_mode) ||
	    (lbuff.st_ino != fbuff.st_ino) ||
	    (lbuff.st_dev != fbuff.st_dev) )
		goto error_out;

	/*
	 * 本人以外に書き込めないことを確認する
	 */
	if ( ((fbuff.st_mode) & RSA_DIR_INVALID_FLAGS))
		goto error_out;

	/*
	 * rootが所有するファイルには, 開かない
	 */
	if ( (fbuff.st_uid == 0) || ( (fbuff.st_uid) && (fbuff.st_uid != uid) ) )
		goto error_out;

	rc = 0;
	*fd_p = fd;

 unlock_out:
	g_static_mutex_unlock(&file_check_mutex); /* 厳密にいえば不要 */

	return rc;

 error_out:
	close(fd);
	g_static_mutex_unlock(&file_check_mutex); /* 厳密にいえば不要 */

	return rc;
}

/** RSA鍵を格納したファイルを書き込み用に開く
 *  @param[out]  fd_p  開いたファイルを操作するためのファイル記述子格納領域のアドレス
 *  @param[in]   file  RSA鍵ファイルのファイル名
 *  @retval  0       正常終了
 *  @retval -EINVAL  引数異常(fd_pまたはfileがNULLである).
 *  @retval -EPERM   以下のいずれかが発生した.
 *                   - ファイルに対するアクセス権がない
 *                   - rootが所有するファイルである,
 *                   - 自身が所有するファイルでない.
 *                   - シンボリックリンクである
 *  @attention 内部リンケージ
 */
static int
open_key_file_for_write(int *fd_p, const unsigned char *file) {
	int    rc = 0;
	int    fd = 0;
	uid_t uid = 0;
	struct stat fbuff,lbuff;


	if ( (fd_p == NULL) || (file == NULL) )
		return -EINVAL;

	g_static_mutex_lock(&file_check_mutex); /* 厳密にいえば不要 */

	fd = open(file, O_CREAT|O_EXCL|O_WRONLY, S_IRUSR|S_IWUSR);

	if (fd < 0){
		if (errno != EEXIST) {
			rc = -errno;
			goto unlock_out;
		} else {
			fd = open(file, O_WRONLY);
			if (fd < 0){
				rc = -errno;
				goto unlock_out;
			}
		}
	}

	rc = fstat(fd, &fbuff);
	if (rc < 0) {
		rc = -errno;
		goto error_out;
	}

	rc = lstat(file, &lbuff);
	if (rc < 0) {
		rc = -errno;
		goto error_out;
	}

	uid = geteuid();

	/*
	 * 偏執的ではあるが, リンクでないことを確認する
	 */
	rc = -EPERM;
	if ( (lbuff.st_mode != fbuff.st_mode) ||
	    (lbuff.st_ino != fbuff.st_ino) ||
	    (lbuff.st_dev != fbuff.st_dev) )
		goto error_out;
	/*
	 * 本人以外に書き込めないことを確認する
	 */
	if ( ((fbuff.st_mode) & RSA_DIR_INVALID_FLAGS))
		goto error_out;
	/*
	 * rootが所有するファイルには, 書き込まない
	 */
	
	if ( (fbuff.st_uid == 0) ||
	    ( (fbuff.st_uid > 0) && (fbuff.st_uid != uid) ) )
		goto error_out;

	rc = 0;
	*fd_p = fd;

unlock_out:
	g_static_mutex_unlock(&file_check_mutex); /* 厳密にいえば不要 */
	return rc;

error_out:
	close(fd);
	g_static_mutex_unlock(&file_check_mutex); /* 厳密にいえば不要 */

  return rc;
}

/** ディレクトリが安全であることを確認する
 *  @param[in]   dir 確認対象のディレクトリへのパス
 *  @retval  0       正常終了
 *  @retval -EINVAL  引数異常(fd_pまたはfileがNULLである).
 *  @retval -EPERM   以下のいずれかが発生した.
 *                   - ディレクトリへの移動に失敗した
 *                   - rootまたは自分自身以外に書き込めるディレクトリが
 *                     指定されたパス上に存在する.
 *                   - rootが所有するディレクトリである,
 *                   - root以外の他ユーザが所有するディレクトリが指定された
 *                     パス上に存在する
 *                   - シンボリックリンクが指定されたパス上に存在する
 *  @attention 内部リンケージ
 */
static int
check_secure_directory(const unsigned char *dir) {
	DIR    *fd = NULL;
	DIR *start = NULL;
	int     rc = 0;
	uid_t  uid = 0;
	void  *ref = NULL;
	struct stat fbuff,lbuff;
	char new_dir[PATH_MAX + 1];

	g_static_mutex_lock(&dir_check_mutex); /* 厳密にいえば不要 */

	start = opendir(".");
	if (start == NULL) {
		rc = -errno;
		goto unlock_out;
	}
	
	rc = lstat(dir, &lbuff);
	if (rc < 0) {
		rc = -errno;
		goto close_dir_out;
	}

	uid = geteuid();

	do {
		rc = chdir(dir);
		if (rc != 0) {
			rc = -errno;
			goto ret_to_work_dir_out;
		}

		fd = opendir(".");
		if (fd == 0)
			goto ret_to_work_dir_out;

		rc = fstat(dirfd(fd), &fbuff);
		closedir(fd);

		if (rc < 0) {
			rc = -errno;
			goto ret_to_work_dir_out;
		}

		/*
		 * 偏執的ではあるが, リンクでないことを確認する
		 */
		rc = -EPERM;
		if ( (lbuff.st_mode != fbuff.st_mode) ||
		    (lbuff.st_ino != fbuff.st_ino) ||
		    (lbuff.st_dev != fbuff.st_dev) )
			goto ret_to_work_dir_out;


		/*
		 * 本人以外に書き込めないことを確認する
		 */
		if ( ((fbuff.st_mode) & RSA_DIR_INVALID_FLAGS))
			goto ret_to_work_dir_out;


		/*
		 * 他者が所有するディレクトリがある場合には, 書き込まない
		 */
		if ( (fbuff.st_uid > 0) && (fbuff.st_uid != uid) ) 
			goto ret_to_work_dir_out;

		/*
		 *１つ上のディレクトリを探査する
		 */
		dir="..";
		rc = lstat(dir, &lbuff);
		if (rc < 0) {
			rc = -errno;
			goto ret_to_work_dir_out;
		}

		memset(new_dir, 0, PATH_MAX + 1);
		ref = getcwd(new_dir, PATH_MAX + 1); /* 次に調査するディレクトリの
						      *  親ディレクトリを獲得
						      */
		if (ref == NULL)
			goto ret_to_work_dir_out;

	} while (new_dir[1] != '\0');

	if (new_dir[1] == '\0')
		rc = 0;
	else
		rc = -EPERM;

ret_to_work_dir_out:
	fchdir(dirfd(start));

close_dir_out:
	closedir(start);

unlock_out:
	g_static_mutex_unlock(&dir_check_mutex); /* 厳密にいえば不要 */

	return rc;
}

/** パスワード入力ダイアログを生成する
 *  @param[out]    passwd_p  入力したパスワードへのポインタを返却する領域のアドレス
 *  @retval  0       正常終了
 *  @retval -EINVAL  引数異常(passwd_pがNULLである).
 *  @retval -EPERM   以下のいずれかが発生した.
 *  @retval -ENOMEM  メモリが不足しいる.
 *  @attention 内部リンケージ
 */
static int
ipmsg_pem_passwd_dialog(gchar **passwd_p){
	int              rc = -EPERM;
	gchar *input_passwd = NULL;

	if (passwd_p == NULL)
		return -EINVAL;

	rc = password_confirm_window(HOSTINFO_PASSWD_TYPE_ENCKEY, &input_passwd);
	if ( (rc == -EPERM) || (rc == -ENOMEM) ) {
		g_assert(input_passwd == NULL); /* 割り当てられていないはず  */
		goto error_out;
	}

	dbg_out("passwd call back buffer = %s\n", input_passwd);
	*passwd_p = input_passwd;

error_out:
	return rc;
}

/** 暗号化PEMを開く際によばれるコールバック関数
 *  @param[out]       buf      パスフレーズ格納先バッファ
 *  @param[in]        len      パスフレーズ格納先バッファ長
 *  @param[in]        rwflag   
 *                              0 ... 読み込み用にオープンした
 *                              1 ... 書き込み用にオープンした
 *  @param[in]        cb_arg    コールバック関数への引数
 *                               (NULLのときは暗号化しない)
 *  @retval  0       異常終了をOpenSSLに通知
 *  @retval  1       設定されているパスワードをOpenSSLに通知
 *  @attention 内部リンケージ
 */
static int
ipmsg_pem_passwd_call_back(char *buf, int len, int rwflag, void *cb_arg){
	int              rc = 0;

	/*
	 * パスワード要求は既に受け付けたので, 単純にエラーにする.
	 */
	if (cb_arg == NULL) {
	  rc = 0; /* 失敗させる  */
	  goto error_out;
	}
	/*
	 * 暗号化ファイルが残っていた場合
	 */
	snprintf(buf, len, "%s", cb_arg);
	buf[len - 1]='\0';
	dbg_out("passwd len = %d passwd=%s\n", len, buf);
	rc = 1;
 error_out:
	return rc;
}

/** 秘密鍵を3DES-EDE-CBCに従って暗号化してファイルに保存する
 *  @param[in] fpath   保存先ファイル名
 *  @param[in] rsa     保存対象RSA情報のアドレス
 *  @param[in] passwd  鍵保存時のPKCS#5鍵導出に使用するパスフレーズ
 *                     NULLの場合は, 暗号化せずに鍵を保存する.
 *  @retval    0       正常終了
 *  @retval   -EINVAL  引数異常(fpathまたはrsaがNULLである).
 *  @retval   -EPERM   ファイルへの書込み権がないか, 
 *                     安全でないファイルへ書き込みを試みた
 *  @retval   -ENOMEM  メモリが不足しいる.
 *  @attention 内部リンケージ
 */
static int
store_private_key(const char *fpath, const RSA *rsa, const char *passwd){
	int          rc = 0;
	int          fd = 0;
	FILE        *fp = NULL;
	EVP_CIPHER *enc = NULL;
	char     errbuf[G2IPMSG_CRYPT_EBUFSIZ];

	if ( (fpath == NULL) || (rsa == NULL) )
		return -EINVAL;
	
	rc = open_key_file_for_write(&fd, fpath);
	if (rc != 0)
		goto error_out;

	fp = fdopen(fd, "w");
	if (fp == NULL) {
		rc = -errno;
		goto error_out;
	}
	
	if (passwd != NULL) 
		enc = (EVP_CIPHER *)EVP_des_ede_cbc();
  
	rc = PEM_write_RSAPrivateKey(fp, (RSA *)rsa, enc, NULL, 0, 
				     ipmsg_pem_passwd_call_back, (void *)passwd);  

	if (rc == 0) {
		rc = ERR_get_error();
		err_out("Can not store private key %s : err=%s\n", 
		    fpath, ERR_error_string(rc, errbuf));
		rc = -EPERM;
		fclose(fp);
		goto error_out;
	}

	rc = fclose(fp);
	if (rc != 0) {
		rc = -errno;
		goto error_out;
	}
	
	rc = 0;  

	dbg_out("Store key into %s\n", fpath);

error_out:
	return rc;
}

/** 公開鍵をファイルに保存する
 *  @param[in] fpath   保存先ファイル名
 *  @param[in] rsa     保存対象RSA情報のアドレス
 *  @retval    0       正常終了
 *  @retval   -EINVAL  引数異常(fpathまたはrsaがNULLである).
 *  @retval   -EPERM   ファイルへの書込み権がないか, 
 *                     安全でないファイルへ書き込みを試みた
 *  @retval   -ENOMEM  メモリが不足しいる.
 *  @attention 内部リンケージ
 */
static int
store_public_key(const char *fpath, const RSA *rsa){
	int          rc = 0;
	int          fd = 0;
	FILE        *fp = NULL;
	EVP_CIPHER *enc = NULL;
	char     errbuf[G2IPMSG_CRYPT_EBUFSIZ];

	if ( (fpath == NULL) || (rsa == NULL) )
		return -EINVAL;

	rc = open_key_file_for_write(&fd, fpath);
	if (rc < 0)
		goto error_out;

	fp = fdopen(fd, "w");
	if (fp == NULL) {
		rc = -errno;
		goto error_out;
	}
  
	rc = PEM_write_RSAPublicKey(fp, rsa);
	if (rc == 0){
		rc = ERR_get_error();
		err_out("Can not store public key %s : err=%s\n", 
		    fpath, ERR_error_string(rc, errbuf));
		rc = -rc;
		fclose(fp);
		goto error_out;
	}

	rc = fclose(fp);
	if (rc != 0) {
		rc = -errno;
		goto error_out;
	}

	rc=0;  

error_out:
	return rc;
}

/** 秘密鍵をファイルから読み込む
 *  @param[in] fpath   保存先ファイル名
 *  @param[out] rsa    読み込んだ秘密鍵を格納するRSA情報のアドレスの返却領域
 *  @param[in] passwd  鍵保存時のPKCS#5鍵導出に使用するパスフレーズ
 *                     NULLの場合は, 暗号化せずに鍵を保存する.
 *  @retval    0       正常終了
 *  @retval   -EINVAL  引数異常(fpathまたはrsaがNULLである).
 *  @retval   -EPERM   ファイルへの読み込み権がないか, 
 *                     安全でないファイルからの読み取りを試みた
 *  @retval   -ENOMEM  メモリが不足しいる.
 *  @attention 内部リンケージ
 */
static int
load_private_key(const char *fpath, RSA **rsa, const char *passwd){
	int     rc = 0;
	int     fd = 0;
	FILE   *fp = NULL;
	void   *ref = NULL;
	char errbuf[G2IPMSG_CRYPT_EBUFSIZ];


	if ( (fpath == NULL) || (rsa == NULL) )
		return -EINVAL;

	rc = open_key_file_for_read(&fd, fpath);
	if (rc != 0)
		goto error_out;

    
	fp = fdopen(fd, "r");
	if (fp == NULL) {
		rc = -errno;
		goto error_out;
	}

	ref = PEM_read_RSAPrivateKey(fp,rsa, ipmsg_pem_passwd_call_back, 
				     (void *)passwd);

	if (ref == NULL){
	  rc = ERR_get_error();
	  err_out("Can not load private key %s : err=%s\n", 
	      fpath, ERR_error_string(rc, errbuf));
	  rc = -EPERM;
	  fclose(fp);
	  goto error_out;
	}

	rc = fclose(fp);
	if (rc != 0) {
		rc = -errno;
		goto error_out;
	}
	
	rc=0;  

error_out:
	return rc;
}

/** 公開鍵をファイルから読み込む
 *  @param[in] fpath   保存先ファイル名
 *  @param[out] rsa    読み込んだ公開鍵を格納するRSA情報のアドレスの返却領域
 *  @retval    0       正常終了
 *  @retval   -EINVAL  引数異常(fpathまたはrsaがNULLである).
 *  @retval   -EPERM   ファイルからの読み取り権がないか, 
 *                     安全でないファイルからの読み取りを試みた
 *  @retval   -ENOMEM  メモリが不足しいる.
 *  @attention 内部リンケージ
 */
static int
load_public_key(const char *fpath, RSA **rsa){
	int          rc = 0;
	int          fd = 0;
	FILE        *fp = NULL;
	EVP_CIPHER *enc = NULL;
	RSA        *ref = NULL;
	char     errbuf[G2IPMSG_CRYPT_EBUFSIZ];

	if ( (fpath == NULL) || (rsa == NULL) )
		return -EINVAL;

	rc = open_key_file_for_read(&fd, fpath);
	if (rc != 0)
		goto error_out;

	fp = fdopen(fd, "r");
	if (fp == NULL) {
		rc = -errno;
		goto error_out;
	}
  
	ref = PEM_read_RSAPublicKey(fp, rsa, NULL, NULL);
	if (ref == NULL){
		rc = ERR_get_error();
		err_out("Can not read public key %s : err=%s\n", 
		    fpath, ERR_error_string(rc, errbuf));
		rc = -rc;
		fclose(fp);
		goto error_out;
	}

	rc = fclose(fp);
	if (rc != 0) {
		rc = -errno;
		goto error_out;
	}

	rc = 0;  

 error_out:
	return rc;
}

/** RSAの鍵ペア(公開鍵/秘密鍵)を生成する
 *  @param[in] fpath   保存先ファイル名
 *  @param[out] rsa    読み込んだ秘密鍵を格納するRSA情報のアドレスの返却領域
 *  @retval    0       正常終了
 *  @retval   -EINVAL  引数異常(fpathまたはrsaがNULLである).
 *  @retval   -EPERM   安全な鍵を生成できなかった.
 *  @attention 内部リンケージ
 */
static int 
generate_rsa_key(RSA **rsa_p, ipmsg_cap_t key_type) {
	char     errbuf[G2IPMSG_CRYPT_EBUFSIZ];
	int          rc = 0;
	RSA        *rsa = NULL;
	int      keylen = 0;
	int retry_count = RSA_KEYGEN_RETRY;

	if (rsa_p == NULL)
		return -EINVAL;

	rc = pcrypt_get_rsa_key_length(key_type, &keylen);
	if (rc != 0)
		return rc;
  
	dbg_out("generate key length:%d\n", keylen);

	/*
	 * RSA 鍵生成 
	 */
 retry:
	rsa = RSA_generate_key(keylen, RSA_F4, NULL, NULL);
	if ( rsa == NULL ){
		rc = ERR_get_error();
		dbg_out("in generate_key: err=%s\n", ERR_error_string(rc, errbuf));

		return -rc;
	}

	/*
	 * RSA鍵の安全性を検証する
	 */
	if (RSA_check_key(rsa) > 0){ /* 安全な鍵である  */
		*rsa_p = rsa;
		rc = 0;
	} else {
		/*
		 * 安全な鍵の生成に失敗した場合は, 鍵生成をやり直す.
		 */
		rc = ERR_get_error();
		err_out("This is invalid key: err=%s\n", ERR_error_string(rc, errbuf));

		RSA_free(rsa);

		rc = -EPERM;
		--retry_count;
		if (retry_count > 0)
			goto retry;
	}

  return rc;
}

/** ピアから送られてきた16進文字列形式(hex形式)の公開鍵をRSA鍵情報に変換する
 *  @param[in]  peer_e ピアの暗号化指数(hex形式)
 *  @param[in]  peer_n ピアの公開モジューロ(法)(hex形式)
 *  @param[out] rsa    公開鍵情報を格納するRSA情報のアドレスの返却領域
 *  @retval    0       正常終了
 *  @retval   -EINVAL  引数異常(peer_eまたはpeer_n, rsaのいずれかがNULLである).
 *  @retval   -ENOMEM  メモリ不足
 *  @attention 内部リンケージ
 */
static int
convert_peer_key(const char *peer_e,const char *peer_n,RSA **rsa){
	int              rc = 0;
	RSA         *pubkey = NULL;
	BIGNUM        *bn_e = NULL;
	BIGNUM        *bn_n = NULL;
	size_t size_in_byte = 0;

	if ( (peer_e == NULL) || (peer_n == NULL) || (rsa == NULL) )
		return -EINVAL;

	pubkey = RSA_new();
	if (pubkey == NULL)
		return -ENOMEM;

	rc = -ENOMEM;
	bn_e = BN_new();
	if (bn_e == NULL)
		goto free_pubkey_out;

	bn_n = BN_new();
	if (bn_n == NULL)
		goto free_bn_e_out;

	rc = BN_hex2bn(&bn_e, peer_e);
	if (rc == 0)
		goto free_bn_e_out;

	rc = BN_hex2bn(&bn_n, peer_n);
	if (rc == 0)
		goto free_bn_e_out;

	pubkey->e = bn_e;
	pubkey->n = bn_n;
	*rsa = pubkey;
  
  return 0;

free_bn_n_out:
  if (bn_n != NULL)
	  BN_free(bn_n);

free_bn_e_out:
  if (bn_e != NULL)
	  BN_free(bn_e);

free_pubkey_out:
  if (pubkey != NULL)
	  RSA_free(pubkey);

  return rc;
}

/** RSA鍵情報をRSA鍵テーブルに登録する
 *  @param[in]  cap    ipmsgの公開鍵暗号化能力ID
 *  @param[in]  rsa    登録するRSA鍵情報
 *  @retval    0       正常終了
 *  @retval   -EINVAL  引数異常(不正な暗号化能力IDを指定したか,  rsaがNULLである).
 *  @attention 内部リンケージ
 */
static int
pcrypt_crypt_set_rsa_key(ipmsg_cap_t cap, const RSA *rsa) {
	int    rc = 0;
	int index = 0;

	if ( (!(cap & RSA_CAPS)) || (rsa == NULL) )
		return -EINVAL;

	rc = get_rsa_key_index(cap, &index);
	if (rc != 0)
		return rc;

	g_static_mutex_lock(&rsa_key_mutex);

	if (rsa_keys[index] != NULL)
		RSA_free(rsa_keys[index]);
  
	rsa_keys[index] = (RSA *)rsa;

	g_static_mutex_unlock(&rsa_key_mutex);

  return 0;
}

/** IPMSG_ANSPUBKEYコマンドで返却するRSA鍵種別を算出する
 *  @param[in]  peer_cap      ピアの暗号化能力
 *  @param[out] selected_key  ピアに返却するRSA鍵種別(使用可能なRSA公開鍵
 *                            暗号化能力IDの論理和)を格納する領域.
 *  @param[in]  speed         速度優先度選択を行う指示を指定する.
 *                            真の場合, 処理速度優先で選択する.
 *                            偽の場合, 暗号強度優先で選択する.
 *  @retval    0       正常終了
 *  @retval   -EINVAL  引数異常(不正な暗号化能力IDを指定したか,  rsaがNULLである).
 *  @retval   -ENOENT  使用可能な鍵が見付からなかった.
 *  @attention 内部リンケージ
 */
static int 
select_rsa_key_for_answer(ipmsg_cap_t peer_cap, ipmsg_cap_t *selected_key, 
    const int speed) {
	ipmsg_cap_t   candidates = 0;
	int                    i = 0;
	int            added_num = 1;
	int                first = 0;
	int             key_type = 0;

	if (selected_key == NULL)
		return -EINVAL;

	candidates = 
		(get_asymkey_part(peer_cap) & hostinfo_get_ipmsg_crypt_capability() );

	if (speed) {
		first = RSA_KEY_MAX - 1;
		added_num = -1;
	}

	for (i = first;
	     ((i >= 0) && 
		 (i < RSA_KEY_MAX) && (!(key2ipmsg_key_type[i] & candidates) ) );
	     i += added_num);

	if ( !( (i >= 0) && (i < RSA_KEY_MAX) ) )
		return -ENOENT;

	*selected_key = key2ipmsg_key_type[i];
	
	dbg_out("Selected key name in ipmsg:0x%x\n", key2ipmsg_key_type[i]);
	
	return 0;
}

/** ipmsgの公開鍵暗号化能力IDからRSAの鍵長を取得する.
 *  @param[in]  key_type      ipmsgの公開鍵暗号化能力ID 
 *  @param[out] len           鍵長を返却する先のアドレス
 *  @retval    0       正常終了
 *  @retval   -EINVAL  引数異常(不正な暗号化能力IDを指定した).
 */
int
pcrypt_get_rsa_key_length(ipmsg_cap_t key_type, size_t *len){
	size_t keylen = 0;
	int        rc = 0;

	if (len == NULL)
		return -EINVAL;

	switch (key_type) {
	case IPMSG_RSA_512:
		keylen = 512;
		break;
	case IPMSG_RSA_1024:
		keylen = 1024;
		break;
	default: /* デフォルトは, 2048 */
	case IPMSG_RSA_2048:
		keylen = 2048;
		break;
	}

	*len = keylen;

	rc = 0;

	return rc;
}

/** IPMSG_ANSPUBKEYパケットの解析を行う
 *  @param[in]  string    IPMSG_ANSPUBKEYパケット(拡張部)を記録した文字列
 *  @param[out] peer_cap  ピアの暗号化能力格納先アドレスを示す
 *                        ポインタ変数のアドレス
 *  @param[out] crypt_e   ピアの暗号化指数(hex形式)格納先アドレスを示す
 *                        ポインタ変数のアドレス(複製を返却, 呼び出し元で要開放)
 *  @param[out] crypt_n   ピアの公開モジューロ(hex形式)格納先アドレスを示す
 *                        ポインタ変数のアドレス(複製を返却, 呼び出し元で要開放)
 *  @retval     0       正常終了
 *  @retval    -EINVAL  引数異常(不正な暗号化能力IDを指定した).
 *  @retval    -ENOMEM  メモリ不足
 */
int 
pcrypt_crypt_parse_anspubkey(const char *string, ipmsg_cap_t *peer_cap, 
    unsigned char **crypt_e, unsigned char **crypt_n) {
	char       *val_e = NULL;
	char       *val_n = NULL;
	char          *sp = NULL;
	char          *ep = NULL;
	char      *buffer = NULL;
	char         *hex = NULL;
	ssize_t   remains = 0;
	ipmsg_cap_t   cap = 0;
	size_t        len = 0;
	int            rc = 0;

	if  ( (string == NULL) || (peer_cap == NULL) || 
	    (crypt_e == NULL)  || (crypt_n == NULL) )
		return -EINVAL;
  
	buffer = g_strdup(string);
	if (buffer == NULL)
		return -ENOMEM;

	len = strlen(string);
	remains = len;

	/*
	 * 暗号化能力
	 */
	sp = buffer;
	ep = memchr(sp, ':', remains);
	if (ep == NULL) {
		rc = -EINVAL;
		goto error_out;
	}

	*ep = '\0';
	remains = len - ( (unsigned long)ep - (unsigned long)buffer );
	if (remains <= 0) {
		rc = -EINVAL;
		goto error_out;
	}
	++ep;
	cap = (ipmsg_cap_t)strtol(sp, (char **)NULL, 16);
	dbg_out("parsed capability:%x\n",cap);
	sp = ep;

	/*
	 * 公開指数
	 */
	ep = memchr(sp, '-', remains);
	if (ep == NULL) {
		rc = -EINVAL;
		goto error_out;
	}
	*ep = '\0';
	remains = len - ( (unsigned long)ep - (unsigned long)buffer );
	if (remains <= 0) {
		rc = -EINVAL;
		goto error_out;
	}
	++ep;
	dbg_out("parsed public exponential:%s\n", sp);

	val_e = g_strdup(sp);
	if (val_e == NULL) {
		rc = -ENOMEM;
		goto error_out;
	}
	sp=ep;


	/*
	 *公開モジューロ(法)
	 */
	dbg_out("parsed public modulo:%s\n", sp);
	val_n = g_strdup(sp);
	if (val_n == NULL) {
		rc = -ENOMEM;
		dbg_out("Error code:%d\n", rc);
		goto error_out;
	}

	/*
	 *返却
	 */
	*peer_cap = cap;
	*crypt_e = val_e;
	*crypt_n = val_n;
	rc = 0;

error_out:
	if (buffer != NULL)
		g_free(buffer);

	return rc;
}

/** IPMSG_ANSPUBKEYパケットの生成を行う
 *  @param[in]  peer_cap  ピアの暗号化能力
 *  @param[out] message_p 生成したIPMSG_ANSPUBKEYパケットを指し示すポインタ変数の  
 *                        アドレス.
 *  @retval     0       正常終了
 *  @retval    -EINVAL  引数異常
 *                      以下のいずれかまたは両方が発生した.
 *                        - 不正な暗号化能力IDをpeer_capに指定した
 *                        - message_pがNULL).
 *  @retval    -ENOMEM  メモリ不足
 */
int 
pcrypt_crypt_generate_anspubkey_string(ipmsg_cap_t peer_cap, const char **message_p) {
	char            *buffer = NULL;
	char            *string = NULL;
	char             *hex_e = NULL;
	char             *hex_n = NULL;
	RSA                *rsa = NULL;
	int                rc = 0;
	ipmsg_cap_t skey_type = 0;
	ipmsg_cap_t akey_type = 0;
	ipmsg_cap_t sign_type = 0;
	size_t      total_len = 0;
	int                 i = 0;

	if ( (message_p == NULL) && (!(peer_cap & RSA_CAPS)) )
		return -EINVAL;

	rc = select_rsa_key_for_answer(peer_cap, &akey_type,
	    hostinfo_refer_ipmsg_crypt_policy_is_speed());

	if (rc != 0)
		return rc;

	dbg_out("RSA key:%x\n", akey_type);

	rc = select_symmetric_key(peer_cap, &skey_type,
	    hostinfo_refer_ipmsg_crypt_policy_is_speed());

	if (rc != 0)
		return rc;

	dbg_out("sym key:%x\n", skey_type);

	rc = select_signature(peer_cap, &sign_type, 
	    hostinfo_refer_ipmsg_crypt_policy_is_speed());

	if (rc != 0)
		dbg_out("No sign:%x\n", sign_type);
	else
		dbg_out("Sign type:%x\n", sign_type);

	rc = pcrypt_crypt_refer_rsa_key(akey_type, &rsa);
	if (rc != 0){
		dbg_out("Can not find rsa key:%d\n", rc);
		return -EINVAL;
	}

	buffer = g_malloc(_MSG_BUF_SIZE);
	if (buffer == NULL)
		return -ENOMEM;
  
	memset(buffer, 0, _MSG_BUF_SIZE);
	rc = -ENOMEM;

	hex_e = BN_bn2hex(rsa->e);
	if (hex_e == NULL)
		goto free_buffer_out;

	hex_n = BN_bn2hex(rsa->n);
	if (hex_n == NULL)
		goto free_hex_n_out;

	dbg_out("hex-e:%s hex-n:%s\n", hex_e, hex_n);

	snprintf(buffer, _MSG_BUF_SIZE, "%x:%s-%s", 
	    (akey_type|skey_type|sign_type), hex_e, hex_n);

	dbg_out("Generated:%s\n", buffer);
	string = g_strdup(buffer);
	total_len = strlen(buffer);

	/*
	 * 小文字に変換
	 */
	for(i = 0; i < total_len; ++i) {
	  string[i] = tolower((int)string[i]);
	}

	*message_p = string;

	rc = 0;

 free_hex_n_out:
	if (hex_n != NULL)
		OPENSSL_free(hex_n);
 free_hex_e_out:
	if (hex_e != NULL)
		OPENSSL_free(hex_e);
 free_buffer_out:
	if (buffer != NULL)
		g_free(buffer);

  return rc;
}

/** IPMSG_GETPUBKEYパケットの生成を行う
 *  @param[in]  cap       自ホストの暗号化能力
 *  @param[out] message_p 生成したIPMSG_ANSPUBKEYパケットを指し示すポインタ変数の  
 *                        アドレス.
 *  @retval     0         正常終了
 *  @retval    -EINVAL    引数異常
 *                        (不正な暗号化能力IDをcapに指定したか, またはmessage_pがNULL).
 *  @retval    -ENOMEM  メモリ不足
 */
int 
pcrypt_crypt_generate_getpubkey_string(ipmsg_cap_t cap, const char **message_p) {
	char *buffer = NULL;
	char *string = NULL;
	int       rc = 0;

	if (message_p == NULL)
		return -EINVAL;

	buffer = g_malloc(IPMSG_GETPUBKEY_LEN);
	if (buffer == NULL) {
		rc = -ENOMEM;
		goto no_free_out;		
	}

	snprintf(buffer, IPMSG_GETPUBKEY_LEN, "%x", cap);
	
	dbg_out("Generated:%s\n", buffer);

	string = g_strdup(buffer);

	if (string == NULL) {
		rc = -ENOMEM;
		goto buffer_free_out;
	}

	*message_p = string;

	rc = 0;

buffer_free_out:
	if (buffer != NULL)
		g_free(buffer);
no_free_out:
	return rc;
}

/** インデクス値からRSA鍵情報のアドレスを獲得する
 *  @param[in]  index   自ホストの暗号化能力
 *  @param[out] rsa_p   RSA鍵情報を指し示すポインタ変数の  
 *                      アドレス.
 *  @retval     0       正常終了
 *  @retval    -EINVAL  引数異常
 *                      (不正なインデクスをindexに指定した).
 *  @retval    -ENOENT  指定されたインデクスに鍵が登録されていなかった.
 */
int
pcrypt_crypt_refer_rsa_key_with_index(int index, RSA **rsa_p) {
	int rc = 0;

	if ( (index < 0) || (RSA_KEY_MAX <= index) )
		return -EINVAL;

	rc = 0;

	g_static_mutex_lock(&rsa_key_mutex);

	if (rsa_keys[index] != NULL)
		*rsa_p = rsa_keys[index];
	else
		rc = -ENOENT;

	g_static_mutex_unlock(&rsa_key_mutex);

	return rc;
}

/** IPMSGの公開鍵暗号化能力IDからRSA鍵情報のアドレスを獲得する
 *  @param[in]  cap     公開鍵暗号化能力ID
 *  @param[out] rsa_p   RSA鍵情報を指し示すポインタ変数の  
 *                      アドレス.
 *  @retval     0       正常終了
 *  @retval    -EINVAL  引数異常
 *                      (不正なIDをcapに指定した).
 *  @retval    -ENOENT  指定されたインデクスに鍵が登録されていなかった.
 */
int
pcrypt_crypt_refer_rsa_key(ipmsg_cap_t cap, RSA **rsa_p) {
	int    rc = 0;
	int index = 0;

	if (!(cap & RSA_CAPS))
		return -EINVAL;

	rc = get_rsa_key_index(cap, &index);
	if (rc != 0)
		return rc;

	g_static_mutex_lock(&rsa_key_mutex);

	if (rsa_keys[index]) {
		*rsa_p = rsa_keys[index];
		rc = 0;
	} else {
		rc = -ENOENT;
	}

	g_static_mutex_unlock(&rsa_key_mutex);

	return rc;
}

/** RSA鍵を初期化する.
 *  
 *  @retval     0       正常終了
 *  @attention  鍵格納用パスフレーズが不正だった場合は, 強制終了する.
 */
int
pcrypt_crypt_init_keys(void) {
	int          rc = 0;
	int           i = 0;
	int      keylen = 0;
	RSA        *rsa = NULL;
	gchar   *passwd = NULL;
	size_t pass_len = 0;

	OpenSSL_add_all_algorithms();
	ERR_load_crypto_strings();

	if (hostinfo_refer_ipmsg_encrypt_public_key()) {
		rc = ipmsg_pem_passwd_dialog(&passwd);
		if (rc == -EPERM) {
			ipmsg_err_dialog_mordal("%s", 
			    _("Can not load RSA key due to invalid passphrase."));
			exit(1);
		}
		g_assert( (passwd == NULL) || (rc == 0) );
	}

	for(i = 0;key2ipmsg_key_type[i] >= 0; ++i) {
		rc = pcrypt_load_rsa_key(key2ipmsg_key_type[i], passwd);
		if (rc == 0)
			continue; /* 鍵をロードした  */
		dbg_out("Can not load key:rc=%d\n", rc);
		rc = generate_rsa_key(&rsa, key2ipmsg_key_type[i]);
		if (rc != 0) {
			rc = pcrypt_get_rsa_key_length(key2ipmsg_key_type[i], &keylen);
			if (rc != 0)
				err_out("Can not generate key length:%d\n", 
				    keylen);
			else
				err_out("Can not generate key length invalid key index:%d\n", i);
			rsa_keys[i] = NULL;
			continue;
		}

		rc = pcrypt_crypt_set_rsa_key(key2ipmsg_key_type[i], rsa);
		if (rc != 0) {
			err_out("Can not set key length:%d\n", keylen);
		} else {
			/*
			 * 生成した鍵を保存する
			 */
			rc = pcrypt_store_rsa_key(key2ipmsg_key_type[i], passwd);
			if (rc != 0)
				err_out("Can not store key: length: %d\n", keylen);
		}
	}

passwd_free_out:
	if (passwd != NULL) {
		pass_len = strlen(passwd);
		memset(passwd, 0xdd, pass_len);  /*  パスワード格納領域を廃棄  */
		g_free(passwd);
	}

	return 0;
}

/** RSA鍵テーブルを開放する.
 *  
 *  @retval     0       正常終了
 */
int
pcrypt_crypt_release_keys(void) {
	int     rc = 0;
	int      i = 0;
	int keylen = 0;

	ERR_free_strings();
	for(i = 0; key2ipmsg_key_type[i] >= 0; ++i) {
		rc = pcrypt_get_rsa_key_length(key2ipmsg_key_type[i], &keylen);
		if (rc == 0)
			dbg_out("clear key length:%d\n", keylen);
     
		if (rsa_keys[i] != NULL){
			RSA_free(rsa_keys[i]);
			rsa_keys[i] = NULL;
		}
	}

	return 0;
}

/** ピアの公開鍵で電文暗号化に使用する共通鍵を暗号化する
 *  @param[in]   peer_cap      ピアの公開鍵暗号化能力ID
 *  @param[in]   peer_e        ピアの暗号化指数(hex形式)
 *  @param[in]   peer_n        ピアの公開モジューロ(hex形式)
 *  @param[in]   plain         平文(電文暗号化に使用する共通鍵)
 *  @param[in]   plain_length  平文の長さ(バイト長)
 *  @param[out]  ret_buff      暗号化電文(hex形式)の返却領域
 *  @param[out]  akey_type     公開鍵暗号鍵種別を表す暗号化ケイパビリティ
 *  @retval     0              正常終了
 *  @retval    -EINVAL         引数異常
 *                             (不正なIDをcapに指定した).
 *  @retval    -ENOMEM         メモリ不足
 */
int
pcrypt_encrypt_message(const ipmsg_cap_t peer_cap, const char *peer_e, 
    const char *peer_n, const char *plain, size_t plain_length, 
		       char **ret_buff, ipmsg_cap_t *akey_type){
	int                 rc = 0;
	RSA            *pubkey = NULL;
	BIGNUM *bn_encoded_key = NULL;
	size_t         enc_len = 0;
	size_t       crypt_len = 0;
	char    *encrypted_key = NULL;
	char      *encoded_key = NULL;
	ipmsg_cap_t   key_type = 0;
	char            errbuf[G2IPMSG_CRYPT_EBUFSIZ];

	if ( (peer_e == NULL) || (peer_n == NULL) || 
	    (plain == NULL) || (ret_buff == NULL) )
		return -EINVAL;

	/*
	 * hex形式のピアの鍵からRSA鍵情報を生成する
	 */
	rc = convert_peer_key(peer_e, peer_n, &pubkey);
	if (rc != 0)
		return rc;

	/*
	 * 電文の暗号化に使用する鍵をピアの公開鍵で暗号化する.
	 */
	rc = -ENOMEM;
	enc_len = RSA_size(pubkey);  /*  暗号化電文長を取得する  */
	dbg_out("Public key:e=%s n=%s len=%d\n",  peer_e,  peer_n,  enc_len);

	encrypted_key = g_malloc(enc_len);  /*  暗号化した共通鍵格納領域を確保  */
	if (encrypted_key == NULL)
		goto free_pubkey_out;
	
	rc = RSA_public_encrypt(plain_length, plain, encrypted_key, pubkey, 
	    RSA_PKCS1_PADDING); /* PKCS1パディングによる暗号化を実施  */

	if (rc <= 0) {
		rc = ERR_get_error();
		err_out("Can not encrypt key with public key: err=%s\n", 
		    ERR_error_string(rc, errbuf));
		rc = -rc;
		goto free_encrypted_key_out;
	}

	/*
	 * 使用した鍵情報を取得
	 */
	rc = select_asymmetric_key(peer_cap, pubkey, &key_type);
	if (rc != 0)
		goto free_encrypted_key_out;

	/*
	 * 暗号化した共通鍵をhex形式に変換
	 */
	rc = -ENOMEM;
	bn_encoded_key = BN_bin2bn(encrypted_key, enc_len, NULL); /* エンディアン変換 */
	if (bn_encoded_key == NULL)
		goto free_encrypted_key_out;

	encoded_key = BN_bn2hex(bn_encoded_key); /* hex形式に変換 */
	if (encoded_key == NULL)
		goto free_bn_encoded_key_out;

	/*
	 * 暗号化した共通鍵と使用した鍵種別を返却
	 */
	*ret_buff = encoded_key;
	*akey_type = key_type;

	rc = 0;

free_bn_encoded_key_out:
	if (bn_encoded_key != NULL)
		BN_free(bn_encoded_key);

 free_encrypted_key_out:
	if (encrypted_key != NULL)
		g_free(encrypted_key);

 free_pubkey_out:
	if (pubkey != NULL) {
		BN_free(pubkey->n);
		BN_free(pubkey->e);
		pubkey->n = NULL;
		pubkey->e = NULL;    
		RSA_free(pubkey);
	}

	return rc;
}

/** 自ホストの秘密鍵で電文復号化用共通鍵を復号化する
 *  @param[in]   cap             暗号化に使用した鍵種別
 *  @param[in]   encrypted_skey 暗号化された共通鍵(hex形式)
 *  @param[out]  ret_buff       復号化した共通鍵(バイナリ形式)の返却領域
 *  @param[out]  decrypted_len  復号化した共通鍵長を返却する領域のアドレス
 *  @retval     0               正常終了
 *  @retval    -EINVAL          引数異常
 *                              (encrypted_skey, ret_buff, decrypted_lenの
 *                               いずれかがNULL).
 *  @retval    -ENOMEM          メモリ不足
 */
int
pcrypt_decrypt_message(ipmsg_cap_t cap, const char *encrypted_skey, 
    char **ret_buff, size_t *decrypted_len) {
	int                      rc = 0;
	RSA                *privkey = NULL;
	char        *decrypted_skey = NULL;
	BIGNUM    *bn_encrypted_key = NULL;
	char              *bin_skey = NULL;
	size_t bn_encrypted_key_len = 0;
	size_t   decrypted_skey_len = 0;
	char                 errbuf[G2IPMSG_CRYPT_EBUFSIZ];

	if ( (encrypted_skey == NULL) || (ret_buff == NULL) || (decrypted_len == NULL) )
		return -EINVAL;

	rc = pcrypt_crypt_refer_rsa_key(cap, &privkey);
	if (rc != 0)
		return rc;

	/*
	 * ネットワークバイトオーダ形式の共通鍵格納領域確保
	 */
	rc = -ENOMEM;
	bn_encrypted_key = BN_new(); 
	if (bn_encrypted_key == NULL)
		goto no_free_out;

	/*
	 * hex形式をネットワークバイトオーダ形式に変換
	 */
	rc = BN_hex2bn(&bn_encrypted_key, encrypted_skey);
	if (rc == 0) {
		rc = ERR_get_error();
		err_out("Can not convert hex-key to BIGNUM: err=%s\n", 
		    ERR_error_string(rc, errbuf));
		rc = -rc;
		goto free_bn_encrypted_key_out;
	}
  
	/*
	 * バイナリ形式の電文暗号化鍵格納領域を確保
	 */
	bn_encrypted_key_len = BN_num_bytes(bn_encrypted_key); 

	rc = -ENOMEM;
	bin_skey = g_malloc(bn_encrypted_key_len);
	if (bin_skey == NULL)
		goto free_bn_encrypted_key_out;
	
	/*
	 * バイナリ形式に変換
	 */
	rc = BN_bn2bin(bn_encrypted_key, bin_skey);
	if (rc == 0) {
		rc = ERR_get_error();
		err_out("Can not convert BIGNUM:to binary err=%s\n", 
		    ERR_error_string(rc, errbuf));
		rc = -rc;
		goto free_bin_skey_out;
	}

	/*
	 * 秘密鍵長から電文暗号化鍵長を算出
	 */
	decrypted_skey_len = BN_num_bytes(privkey->n);

	/*
	 * 電文暗号化鍵格納領域を確保
	 */
	rc = -ENOMEM;
	decrypted_skey = g_malloc(decrypted_skey_len);
	if (decrypted_skey == NULL)
		goto free_bin_skey_out;

	/*
	 * 電文暗号化鍵を復号化
	 */
	rc = RSA_private_decrypt(bn_encrypted_key_len, bin_skey, 
	    decrypted_skey, privkey, RSA_PKCS1_PADDING);

	if (rc <= 0) {
		if (decrypted_skey != NULL)
			g_free(decrypted_skey);
		rc = ERR_get_error();
		err_out("Can not decrypt secret key err=%s\n", 
		    ERR_error_string(rc, errbuf));
		rc = -rc;
		goto free_bin_skey_out;
	}

	/*
	 * 復号化した鍵を返却
	 */
	*decrypted_len = decrypted_skey_len;
	*ret_buff = decrypted_skey;

	rc = 0;

free_bin_skey_out:
	if (bin_skey != NULL)
		g_free(bin_skey);

free_bn_encrypted_key_out:
	if (bn_encrypted_key != NULL)
		BN_free(bn_encrypted_key);
no_free_out:
	return rc;
}

/** 自ホストの秘密鍵を使用して電文に署名を付す
 *  @param[in]   cap            通信に使用する書名種別を含む暗号化ケイパビリティ
 *  @param[in]   sign_type      使用する署名種別
 *                               - IPMSG_SIGN_MD5   MD5署名を使用
 *                               - IPMSG_SIGN_SHA1  SHA1を使用
 *  @param[in]   msg_str        署名対象となる暗号化鍵を含む暗号化電文
 *                              (IPMSG暗号化電文パケットの本文部)
 *  @param[out]  ret_buff       msg_strに付した署名を指し示すためのポインタ変数の
 *                              アドレスを指定
 *  @retval     0               正常終了
 *  @retval    -EINVAL          引数異常
 *                              (msg_str,  ret_buffのいずれかがNULL).
 *  @retval    -ENOMEM          メモリ不足
 */
int 
pcrypt_sign(const ipmsg_cap_t cap, const ipmsg_cap_t sign_type, 
    const unsigned char *msg_str, unsigned char **ret_buff) {
	int                     rc = 0;
	size_t             out_len = 0;
	size_t             msg_len = 0;
	int          nid_sign_type = 0;
	size_t          digest_len = 0;
	RSA                   *key = NULL;
	unsigned char         *out = NULL;
	BIGNUM            *bn_hash = NULL;
	unsigned char *hash_string = NULL;
	void                  *ref = NULL;
	BIGNUM             *bn_ref = NULL;  
	char                errbuf[G2IPMSG_CRYPT_EBUFSIZ];
	unsigned char         hash[MD5SHA1_DIGEST_LEN];

	if ( (msg_str == NULL) || (ret_buff == NULL) )
		return -EINVAL;

	/*
	 * 使用した暗号化ケイパビリティからRSA鍵情報を獲得する
	 */
	rc = pcrypt_crypt_refer_rsa_key(cap, &key);
	if (rc != 0)
		return rc;

	msg_len = strlen(msg_str);  /*  署名対象電文長を獲得  */

	rc = -ENOMEM;
	out = g_malloc(RSA_size(key)); /*  署名のサイズを獲得  */
	if (out == NULL)
		goto no_free_out;

	/*
	 *  署名のハッシュ値を獲得  
	 */
	switch(sign_type) {
	case IPMSG_SIGN_MD5:
		nid_sign_type = NID_md5;
		digest_len = MD5_DIGEST_LEN;

		ref = MD5(msg_str, msg_len, hash);
		if (ref == NULL) {
			rc = ERR_get_error();
			err_out("Can not calculate MD5 hash value: err=%s\n", 
			    ERR_error_string(rc, errbuf));
			rc = -rc;
			goto out_free_out;
		}
		break;
	case IPMSG_SIGN_SHA1:
		nid_sign_type = NID_sha1;
		digest_len = SHA1_DIGEST_LEN;
		
		ref = SHA1(msg_str, msg_len, hash);
		if (ref == NULL) {
			rc = ERR_get_error();
			err_out("Can not calculate SHA1 hash value: err=%s\n", 
			    ERR_error_string(rc, errbuf));
			rc = -rc;
			goto out_free_out;
		}
		break;
	default:
		rc = -EINVAL;
		goto out_free_out;
		break;
	}

	/*
	 * バイナリ形式の署名を生成
	 */
	rc = RSA_sign(nid_sign_type, hash, digest_len, out, &out_len, key);
	if (rc < 0)
		goto out_free_out;

	/*
	 * 署名をネットワークバイトオーダに変換
	 */
	rc = -ENOMEM;
	bn_hash = BN_new();
	if (bn_hash == NULL)
		goto out_free_out;

	bn_ref = BN_bin2bn(out, out_len, bn_hash); /* bin2bnは, 複製を作らないことに注意 */
	if (bn_ref == NULL) {
		rc = ERR_get_error();
		err_out("Can not convert binary sign to BIGNUM: err=%s\n", 
		    ERR_error_string(rc, errbuf));
		rc = -rc;
		goto bn_hash_free_out;
	}

	/*
	 * 署名をhex形式に変換
	 */
	hash_string = BN_bn2hex(bn_hash);
	if (hash_string == NULL) {
		rc = ERR_get_error();
		err_out("Can not convert BIGNUM sign to hex: err=%s\n", 
		    ERR_error_string(rc, errbuf));
		rc = -rc;		
		goto bn_hash_free_out;
	}

	rc = 0;
	*ret_buff = hash_string;  /*  署名を返却  */

bn_hash_free_out:
	if (bn_hash != NULL)
		BN_free(bn_hash);  
out_free_out:
	if (out != NULL)
		g_free(out);

no_free_out:
	return rc;
}

/** 電文に付加された署名を検証し, 電文の完全性を検証する.
 *  @param[in]   cap            通信に使用した書名種別を含む暗号化ケイパビリティ
 *  @param[in]   sign_type      使用した署名種別
 *                               - IPMSG_SIGN_MD5   MD5署名を使用
 *                               - IPMSG_SIGN_SHA1  SHA1を使用
 *  @param[in]   msg_str        検証対象となる暗号化鍵を含む暗号化電文(hex形式)
 *  @param[in]   sign           msg_strに付された署名(hex形式)
 *  @param[in]   peer_e         ピアの暗号化指数(hex形式)
 *  @param[in]   peer_n         ピアの公開モジューロ(hex形式)
 *  @retval     0               正常終了
 *  @retval    -EINVAL          引数異常
 *                              (msg_str, sign, peer_e, peer_nの
 *                               いずれかがNULL).
 *  @retval    -ENOMEM          メモリ不足
 */
int 
pcrypt_verify_sign(const ipmsg_cap_t cap, const ipmsg_cap_t sign_type, 
    const unsigned char *msg_str, const unsigned char *sign, 
    const unsigned char *peer_e, const unsigned char *peer_n) {
	int                  rc = 0;
	RSA                *key = NULL;
	BIGNUM         *bn_sign = NULL;
	void               *ref = NULL;
	unsigned char *raw_sign = NULL;
	size_t          msg_len = 0;
	int       nid_sign_type = 0;
	size_t       digest_len = 0;
	unsigned char      hash[MD5SHA1_DIGEST_LEN];
	char             errbuf[G2IPMSG_CRYPT_EBUFSIZ];


	if ( (msg_str == NULL) || (sign == NULL) || 
	    (peer_e == NULL) || (peer_n == NULL) )
		return -EINVAL;

	/*
	 * ピアの公開鍵からRSA鍵情報を獲得
	 */
	rc = convert_peer_key(peer_e, peer_n, &key);
	if (rc != 0)
		return rc;

	msg_len = strlen(msg_str);
	rc = -ENOMEM;

	/*
	 *hex形式の署名をネットワークバイトオーダ形式に変換
	 */
	rc = BN_hex2bn(&bn_sign, sign);
	if (rc == 0) {
		rc = ERR_get_error();
		err_out("Can not convert hex-key to BIGNUM: err=%s\n", 
		    ERR_error_string(rc, errbuf));
		rc = -rc;
		goto bn_sign_free_out;
	}
	/*
	 * バイナリ形式の署名用の領域を確保
	 */
	raw_sign = g_malloc(BN_num_bytes(bn_sign));
	if (raw_sign == NULL)
		goto bn_sign_free_out;

	/*
	 * バイナリ形式に変換
	 */
	rc = BN_bn2bin(bn_sign, raw_sign);
	if (rc != RSA_size(key)) {
		err_out("hash size is invalid(rc, rsa-key-size)=(%d, %d)\n",
		    rc, RSA_size(key));
		goto raw_sign_free_out;
	}

	/*
	 * タイミング攻撃対策
	 */
	rc = RSA_blinding_on(key, NULL);
	if (rc == 0) {
		rc = ERR_get_error();
		err_out("Can not make RSA key blind: err=%s\n",
		    ERR_error_string(rc, errbuf));
		rc = -rc;
		goto raw_sign_free_out;
	}
	
	/*
	 * 署名検証用に本文からハッシュ値を算出
	 */
	switch(sign_type) {
	default:
		rc = -EINVAL;
		goto raw_sign_free_out;
		break;
	case IPMSG_SIGN_MD5:
		nid_sign_type = NID_md5;
		ref = MD5(msg_str, msg_len, hash);
		if (rc == 0) {
			rc = ERR_get_error();
			err_out("Can not calculate MD5 hash value : err=%s\n", 
			    ERR_error_string(rc, errbuf));
			rc = -rc;
			dbg_out("Verify with  MD5 sign\n");
			goto raw_sign_free_out;
		}
		digest_len = MD5_DIGEST_LEN;
		break;
	case IPMSG_SIGN_SHA1:
		nid_sign_type = NID_sha1;
		ref = SHA1(msg_str, msg_len, hash);
		if (rc == 0) {
			rc = ERR_get_error();
			err_out("Can not calculate SHA1 hash value : err=%s\n", 
			    ERR_error_string(rc, errbuf));
			rc = -rc;
			goto raw_sign_free_out;
		}
		digest_len = SHA1_DIGEST_LEN;
		dbg_out("Verify with  SHA1 sign\n");
		break;
	}

	/*
	 * 署名検証
	 */
	rc = RSA_verify(nid_sign_type, hash,digest_len, raw_sign, RSA_size(key), key);
	if (rc == 0){
		rc = ERR_get_error();
		err_out("Sign invalid : err=%s\n", ERR_error_string(rc, errbuf));
		rc = -rc;
		goto raw_sign_free_out;
	} else {
		rc = 0;
	}
	
raw_sign_free_out:
	if (raw_sign != NULL)
		g_free(raw_sign);
 bn_sign_free_out:
	if (bn_sign != NULL)
		BN_free(bn_sign);
 free_key_out:
	if (key != NULL) {
		g_assert(key->n);
		g_assert(key->e);
		BN_free(key->n);
		BN_free(key->e);
		key->n = NULL;
		key->e = NULL;    
		RSA_free(key);
	}

no_free_out:
	return rc;
}

/** IPMSGの公開鍵暗号化能力IDから鍵情報格納先ファイル名を割り出す.
 *  @param[in]   key_type       IPMSGの公開鍵暗号化能力ID   
 *  @param[out]  pubkey         公開鍵格納ファイル
 *  @param[out]  privkey        秘密鍵格納ファイル
 *  @retval     0               正常終了
 *  @retval    -EINVAL          引数異常
 *                              (msg_str, sign, peer_e, peer_nの
 *                               いずれかがNULL).
 *  @retval    -ENOMEM          メモリ不足
 */
int
pcrypt_get_key_filename(const ipmsg_cap_t key_type, char **pubkey, char **privkey) {
	int                    rc = 0;
	size_t            key_len = 0;
	size_t      key_fname_len = 0;
	RSA                  *rsa = NULL;
	size_t key_priv_fname_len = 0;
	size_t  key_pub_fname_len = 0;
	char        *pub_key_file = NULL;
	char       *priv_key_file = NULL;
	int                     j = 0;

  
	if ( (pubkey == NULL) || (privkey == NULL) )
		return -EINVAL;

	rc = pcrypt_get_rsa_key_length(key_type, &key_len);
	if (rc != 0)
		return rc;

	dbg_out("Key bits:%d\n", key_len);

	/* キー長をbit長で表した値の桁数を算出  */
	for(j = key_len, key_fname_len = 1;j != 0;j /= 10, ++key_fname_len);

	/* +1 は, 末尾のヌルターミネート分 */
	key_priv_fname_len = strlen(RSA_PRIVKEY_FNAME_PREFIX) + key_fname_len + 1;
	priv_key_file = g_malloc(key_priv_fname_len);
	if (priv_key_file == NULL)
		return -EINVAL;

	key_pub_fname_len = strlen(RSA_PUBKEY_FNAME_PREFIX) + key_fname_len + 1;
	pub_key_file = g_malloc(key_pub_fname_len);
	if (pub_key_file == NULL)
		goto priv_key_file_free_out;

	snprintf(priv_key_file,key_priv_fname_len - 1, "%s%d", 
	    RSA_PRIVKEY_FNAME_PREFIX, key_len);

	snprintf(pub_key_file, key_pub_fname_len - 1, "%s%d", 
	    RSA_PUBKEY_FNAME_PREFIX, key_len);

	dbg_out("Public key files are Private-key:%s Public-key:%s\n",
	    priv_key_file, pub_key_file);
  
	*pubkey  = pub_key_file;
	*privkey = priv_key_file;

	return 0;

pub_key_file_free_out:
	if (pub_key_file != NULL)
		g_free(pub_key_file);

priv_key_file_free_out:
	if (priv_key_file != NULL)
		g_free(priv_key_file);

	return rc;
}

/** RSAのキーペア(公開鍵/秘密鍵)をファイルに書き込む
 *  @param[in]   key_type  IPMSGの公開鍵暗号化能力ID   
 *  @param[in]     passwd  鍵保存時のPKCS#5鍵導出に使用するパスフレーズ
 *                         NULLの場合は, 暗号化せずに鍵を保存する.
 *  @retval     0          正常終了
 *  @retval    -EPERM      書き込み権限が無い
 *  @retval    -ENOMEM     メモリ不足
 */
int 
pcrypt_store_rsa_key(const ipmsg_cap_t key_type, const char *passwd) {
	int               rc = 0;
	gchar     *store_dir = NULL;
	char       *home_dir = NULL;
	size_t       key_len = 0;
	RSA             *rsa = 0;
	int                j = 0;
	size_t key_fname_len = 0;
	char  *priv_key_file = NULL;
	char   *pub_key_file = NULL;
	char      *file_path = NULL;


	rc = get_envval("HOME", &home_dir);
	if (rc != 0) 
		goto no_free_out;

	store_dir = g_build_filename(home_dir,G2IPMSG_KEY_DIR, NULL);
	if (store_dir == NULL)
		goto home_dir_free_out;
  
	rc = check_secure_directory(store_dir);
	if ( (rc != 0) && (rc != -ENOENT) ) {
		err_out("Directory %s is not secure.\n", store_dir);
		goto store_dir_free_out;
	}

	if (rc == -ENOENT) {
		dbg_out("Directory %s does not exist.\n", store_dir);
		rc = mkdir(store_dir, S_IRWXU);

		if (rc != 0){
			rc = -errno;
			goto store_dir_free_out;
		}
	}

	dbg_out("Directory check:%s OK\n", store_dir);

	rc = pcrypt_crypt_refer_rsa_key(key_type, &rsa);
	if (rc != 0)
		goto store_dir_free_out;

	rc = pcrypt_get_key_filename(key_type, &pub_key_file, &priv_key_file);
	if (rc != 0)
		goto store_dir_free_out;

	/*
	 * Private key
	 */
	key_fname_len = strlen(store_dir) + strlen(priv_key_file) + 3;
	file_path = g_malloc(key_fname_len);
	if (file_path == NULL)
		goto filename_free_out;

	snprintf(file_path, key_fname_len, "%s" G_DIR_SEPARATOR_S "%s",
	    store_dir, priv_key_file);

	rc = store_private_key(file_path, rsa, passwd);
	if (rc != 0)
		goto filename_free_out;
	if (file_path != NULL)
		g_free(file_path);
	file_path = NULL;

	/*
	 * Public key
	 */
	key_fname_len = strlen(store_dir) + strlen(pub_key_file) + 3;
	file_path = g_malloc(key_fname_len);
	if (file_path == NULL)
		goto filename_free_out;

	snprintf(file_path, key_fname_len, "%s" G_DIR_SEPARATOR_S "%s",
	   store_dir, pub_key_file);

	rc = store_public_key(file_path, rsa);

	if (rc != 0)
		goto filename_free_out;

	if (file_path != NULL)
		g_free(file_path);
	file_path = NULL;
  
	rc = 0;

filename_free_out:
	if (file_path != NULL)
		g_free(file_path);

pub_key_file_free_out:
	if (pub_key_file != NULL)
		g_free(pub_key_file);
  if (priv_key_file != NULL)
	  g_free(priv_key_file);

store_dir_free_out:
  if (store_dir != NULL)
	  g_free(store_dir);

home_dir_free_out:
  if (home_dir != NULL)
	  g_free(home_dir);

no_free_out:
  return rc;
}

/** RSAのキーペア(公開鍵/秘密鍵)をファイルから読み込む
 *  @param[in]   key_type  IPMSGの公開鍵暗号化能力ID   
 *  @param[in]     passwd  鍵保存時のPKCS#5鍵導出に使用するパスフレーズ
 *                         NULLの場合は, 暗号化なしのファイルを仮定する.
 *  @retval     0          正常終了
 *  @retval    -EPERM      読み取り権限が無い
 *  @retval    -ENOMEM     メモリ不足
 */
int 
pcrypt_load_rsa_key(const ipmsg_cap_t key_type, const gchar *passwd){
	int               rc = 0;
	gchar     *store_dir = NULL;
	char       *home_dir = NULL;
	size_t       key_len = 0;
	RSA             *rsa = NULL;
	int                j = 0;
	size_t key_fname_len = 0;
	char  *priv_key_file = NULL;
	char   *pub_key_file = NULL;
	char      *file_path = NULL;
	char          errbuf[G2IPMSG_CRYPT_EBUFSIZ];

	rc = get_envval("HOME", &home_dir);
	if (rc != 0) 
		goto no_free_out;

	store_dir = g_build_filename(home_dir, G2IPMSG_KEY_DIR, NULL);
	if (store_dir == NULL) {
		rc = -EPERM;
		goto home_dir_free_out;
	}

	rc = check_secure_directory(store_dir);
	if  (rc != 0) {
		err_out("Directory %s is not secure or does not exist.\n",store_dir);
		goto store_dir_free_out;
	}

	dbg_out("Directory check:%s OK\n",store_dir);

	rc = pcrypt_get_key_filename(key_type, &pub_key_file, &priv_key_file);
	if (rc != 0)
		goto store_dir_free_out;

	/*
	 * Private key
	 */
	key_fname_len = 
		strlen(store_dir) + strlen(priv_key_file) + 3;
	file_path = g_malloc(key_fname_len);
	if (file_path == NULL)
		goto filename_free_out;

	snprintf(file_path, key_fname_len, "%s" G_DIR_SEPARATOR_S "%s",
	    store_dir, priv_key_file);

	rc = load_private_key(file_path, &rsa, passwd);
	if (rc != 0)
		goto filename_free_out;
	if (file_path != NULL) {
		g_free(file_path);
		file_path = NULL;
	}

	/*
	 * Public key
	 */
	key_fname_len = 
		strlen(store_dir) + strlen(pub_key_file) + 3;
	file_path = g_malloc(key_fname_len);
	if (file_path == NULL)
		goto filename_free_out;

	snprintf(file_path, key_fname_len,"%s" G_DIR_SEPARATOR_S "%s",
	    store_dir, pub_key_file);

	rc = load_public_key(file_path, &rsa);
	if (rc != 0)
		goto filename_free_out;

	if (file_path != NULL) {
		g_free(file_path);
		file_path=NULL;
	}

	rc = RSA_check_key(rsa);
	if (rc == 0) {
		rc = ERR_get_error();
		err_out("Invalid RSA key : err=%s\n", ERR_error_string(rc, errbuf));
		rc = -rc;
		RSA_free(rsa);
		goto filename_free_out;
	}

	rc = pcrypt_crypt_set_rsa_key(key_type, rsa);
	if (rc != 0) {
		RSA_free(rsa);
		goto filename_free_out;
	}

	rc = pcrypt_get_rsa_key_length(key_type, &key_len);
	g_assert(rc == 0);
	dbg_out("RSA key length: %d has been loaded successfully.\n", key_len);

	rc = 0;

filename_free_out:
	if (file_path != NULL)
		g_free(file_path);

pub_key_file_free_out:
	if (pub_key_file != NULL)
		g_free(pub_key_file);
	if (priv_key_file != NULL)
		g_free(priv_key_file);

store_dir_free_out:
	if (store_dir != NULL)
		g_free(store_dir);

home_dir_free_out:
	if (home_dir != NULL)
		g_free(home_dir);

no_free_out:
	return rc;
}

/** 通信に使用する公開鍵を選択する.
 *  @param[in]   peer_cap     ピアの公開鍵暗号化能力ID
 *  @param[in]   rsa          RSA鍵情報
 *  @param[out]  selected_key 通信に使用するRSA鍵種別(使用するRSA公開鍵
 *                            暗号化能力IDの論理和)を格納する領域.
 *  @retval     0             正常終了
 *  @retval    -EINVAL        rsaまたはselected_keyがNULLである.
 *  @retval    -ENOENT        使用可能な鍵が無い.
 */
int 
select_asymmetric_key(const ipmsg_cap_t peer_cap, const  RSA *rsa, 
    ipmsg_cap_t *selected_key) {
	ipmsg_cap_t candidates = 0;
	int                    i = 0;
	int                   rc = 0;
	int            added_num = 1;
	int                first = 0;
	int             key_type = 0;
	size_t       size_in_bit = 0;

	if ( (selected_key == NULL) || (rsa == NULL) )
		return -EINVAL;

	candidates = (peer_cap & hostinfo_get_ipmsg_crypt_capability());
	if (candidates == 0)
		return -ENOENT;

	size_in_bit = RSA_size(rsa) * 8;
	dbg_out("key size=%d.\n", size_in_bit);

	switch(size_in_bit) {
	case 512:
		dbg_out("This is 512 bit key.\n");
		key_type = IPMSG_RSA_512;
		break;
	case 1024:
		dbg_out("This is 1024 bit key.\n");
		key_type = IPMSG_RSA_1024;
		break;
	case 2048:
		dbg_out("This is 2048 bit key.\n");
		key_type = IPMSG_RSA_2048;
		break;
	default:
		rc = -ENOENT;
		goto error_out;
		break;
	}
  
	if (!(key_type & candidates))
		return -ENOENT;

	*selected_key = key_type;
	dbg_out("Selected key name in ipmsg:0x%x\n", key_type);

error_out:
	return rc;
}

/** 通信に使用する署名を選択する.
 *  @param[in]   peer_cap      ピアの公開鍵暗号化能力ID
 *  @param[out]  selected_algo 署名に使用するアルゴリズム
 *                             暗号化能力IDの論理和)を格納する領域.
 *  @param[in]  speed          速度優先で選択を行う指示を指定する.
 *                             - 真の場合, 処理速度優先で選択する.
 *                             - 偽の場合, 暗号強度優先で選択する.
 *  @retval     0              正常終了
 *  @retval    -EINVAL         selected_algoがNULLである.
 *  @retval    -ENOENT         使用可能な署名が無い.
 */
int 
select_signature(const ipmsg_cap_t peer_cap, ipmsg_cap_t *selected_algo, 
    const int speed) {
	int                  rc = 0;
	ipmsg_cap_t   candidate = 0;

	if (selected_algo == NULL)
		return -EINVAL;

	if ( !(peer_cap & SIGN_CAPS) )
		return -ENOENT;

       /*
	* 速度が早い順に探査し, 速度優先の場合は, 見つかったところでぬける
	* 下記の処理は, 処理速度順で判定を配置していくこと.
	*/

	rc = 0; /* 正常終了を仮定  */

	

	if (peer_cap & IPMSG_SIGN_MD5) { /* MD5 */
		candidate = IPMSG_SIGN_MD5;
		if (speed)
		  goto selected_out;  
	}

	if (peer_cap & IPMSG_SIGN_SHA1) { /* SHA1 */
		candidate = IPMSG_SIGN_SHA1;
		if (speed)
			goto selected_out;  
	} 

	if (candidate == 0) {
		/* 上記でチェックしているので, ここには来ない */
		g_assert_not_reached(); 
		goto error_out;  /* 見つからなかった  */
	}

selected_out:
	*selected_algo = candidate;

error_out:
	return rc;
}

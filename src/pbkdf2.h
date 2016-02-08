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

#if !defined(IPMSG_PBKDF2_H)
#define IPMSG_PBKDF2_H

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <errno.h>

#define PBKDF2_PRF_OUT_LEN 20
#define PBKDF2_SALT_LEN 8
#define PBKDF2_KEY_LEN  16
#define PBKDF2_PREFIX   "$10$"
#define PBKDF2_FORMAT   PBKDF2_PREFIX "%s$%u$%s"
#define PBKDF2_PREFIX_LEN       (4)
#define PBKDF2_FORMAT_ADDED_LEN (PBKDF2_PREFIX_LEN+2) /* strlen("$10$")+strlen("$")*2 */
#define PBKDF2_ITER_CNT 10000
#if defined(USE_OPENSSL)
int pbkdf2_encrypt(const char *key, const char *salt,char **enc_pass);
int pbkdf2_encoded_passwd_configured(const char *enc_pass);
int pbkdf2_verify(const char *plain_passwd,const char *crypt_passwd);
#else
#define pbkdf2_encrypt(key, salt, enc_pass)        (-ENOSYS)
#define pbkdf2_verify(plain_passwd, crypt_passwd)  (-ENOSYS)
#define pbkdf2_encoded_passwd_configured(enc_pass) (-ENOSYS)
#endif  /*  USE_OPENSSL  */
#endif  /*  IPMSG_PBKDF2_H  */

/*
 * Following codes are derived from 
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

/*  pkcs5  */
static void
pkcs5_initial_prf(unsigned char *p,
		  size_t plen,
		  unsigned char *salt,
		  size_t saltlen,
		  size_t i,
		  unsigned char *out,
		  size_t *outlen){
  size_t swapped_i;
  HMAC_CTX ctx;

  HMAC_CTX_init(&ctx);
  HMAC_Init(&ctx,p,plen,EVP_sha1());
  HMAC_Update(&ctx,salt,saltlen);
  swapped_i=htonl(i);
  HMAC_Update(&ctx,(unsigned char *)&swapped_i,sizeof(swapped_i));
  HMAC_Final(&ctx,out,(unsigned int *)outlen);
  HMAC_CTX_cleanup(&ctx);
}

static void
pkcs5_subsequent_prf(unsigned char *p,
		     size_t plen,
		     unsigned char *v,
		     size_t vlen,
		     unsigned char *o,
		     size_t *olen){
  HMAC_CTX ctx;

  HMAC_CTX_init(&ctx);
  HMAC_Init(&ctx,p,plen,EVP_sha1());
  HMAC_Update(&ctx,v,vlen);
  HMAC_Final(&ctx,o,(unsigned int *)olen);
  HMAC_CTX_cleanup(&ctx);
}

static void
pkcs5_F(unsigned char *p,
	size_t plen,
	unsigned char *salt,
	size_t saltlen,
	size_t ic,
	size_t bix,
	unsigned char *out){
  size_t i,j,outlen;
  unsigned char ulast[PBKDF2_PRF_OUT_LEN];
  
  memset(out,0,PBKDF2_PRF_OUT_LEN);
  pkcs5_initial_prf(p,plen,salt,saltlen,bix,ulast,&outlen);
  for(i=1;i<=ic;++i){
    for(j=0;j<PBKDF2_PRF_OUT_LEN;j++)
      out[j] ^= ulast[j];
    pkcs5_subsequent_prf(p,plen,ulast,PBKDF2_PRF_OUT_LEN,ulast,&outlen);
  }

  for(j=0;j<PBKDF2_PRF_OUT_LEN;j++)
    out[j] ^= ulast[j];
}

static void 
spc_pbkdf2(unsigned char *pw, size_t pwlen, char *salt, uint64_t saltlen, uint32_t ic, unsigned char *dk,uint64_t dklen){
  uint32_t i,l,r;
  unsigned char final[PBKDF2_PRF_OUT_LEN]={0,};
  
  if (dklen > (((unsigned long long)1)<<32 -1) * PBKDF2_PRF_OUT_LEN){
    abort();
  }
  l=dklen / PBKDF2_PRF_OUT_LEN;
  r=dklen % PBKDF2_PRF_OUT_LEN;
  for(i=1;i<=l;++i)
    pkcs5_F(pw,pwlen,salt,saltlen,ic,i,dk + (i-1) * PBKDF2_PRF_OUT_LEN);
  if (r) {
    pkcs5_F(pw,pwlen,salt,saltlen,ic,i,final);
    for(l=0;l<r;++l)
      *(dk + (i-1) * PBKDF2_PRF_OUT_LEN + l) = final[l];
  }
}


int
pbkdf2_encoded_passwd_configured(const char *enc_pass) {

  if (enc_pass == NULL)
    return -EINVAL;
  dbg_out("check:%s, and %s\n", enc_pass, PBKDF2_PREFIX);
  if ( (enc_pass[0] == '\0') || (strncmp(enc_pass, PBKDF2_PREFIX, PBKDF2_PREFIX_LEN)) )
    return -ENOENT;

  return 0;
}

int
pbkdf2_encrypt(const char *key, const char *salt, char **enc_pass) {
  int rc;
  int i;
  size_t num_len;
  unsigned char *raw_salt=NULL;
  unsigned char *base64_salt=NULL;
  unsigned char *base64_out=NULL;
  unsigned int iterations,tmp_num;
  size_t key_len;
  size_t salt_len,encoded_salt_len;
  size_t result_len;
  char *output=NULL;
  char *ret_str;
  char *salt_end=NULL,*endptr;
  unsigned long tmp_ulong;

  if ( (!key) || (!enc_pass) )
    return -EINVAL;

  /*
   * Saltとくり返し回数を設定
   */
  if (!salt) {
    raw_salt = g_malloc(PBKDF2_SALT_LEN);
    if (!raw_salt)
      return -ENOMEM;

    rc = generate_rand(raw_salt, PBKDF2_SALT_LEN);
    if (rc)
      goto free_raw_salt;

    rc = base64_encode(raw_salt, PBKDF2_SALT_LEN,(gchar **)&base64_salt);
    if (rc)
      goto free_raw_salt;

    iterations=PBKDF2_ITER_CNT;
    salt_len=PBKDF2_SALT_LEN;
  } else {
    if (strncmp(salt,PBKDF2_PREFIX,PBKDF2_PREFIX_LEN))
      return -EINVAL;
    salt_end = strchr(salt + PBKDF2_PREFIX_LEN,'$');
    if (!salt_end)
      return -EINVAL;
    encoded_salt_len=salt_end - (salt + PBKDF2_PREFIX_LEN) + 1;
    base64_salt=g_malloc(encoded_salt_len+1);
    if (!base64_salt)
      return -ENOMEM;
    memcpy(base64_salt,salt + PBKDF2_PREFIX_LEN,encoded_salt_len);
    base64_salt[encoded_salt_len-1]='\0';
    dbg_out("base64 salt:%s\n",base64_salt);
    errno=0;
    endptr=NULL;
    tmp_ulong=strtoul(salt_end + 1,&endptr,10);
    if ( (tmp_ulong == ULONG_MAX && errno == ERANGE) ||
	 (tmp_ulong > UINT_MAX)                      ||
	 (!endptr)                                   ||
	 (*endptr != '$') ) {
      g_free(base64_salt);
      return -EINVAL;
    }
    iterations=(unsigned long)tmp_ulong;
    rc=base64_decode(base64_salt,&salt_len,(gchar **)&raw_salt);
    if (rc) {
      g_free(base64_salt);
      return rc;
    }
  }

  output=g_malloc(PBKDF2_KEY_LEN);
  if (!output)
    goto free_base64_encoded_salt;
  key_len=strlen(key);

  spc_pbkdf2((unsigned char *)key,key_len,raw_salt,salt_len,iterations,output,PBKDF2_KEY_LEN);
  rc=base64_encode(key,key_len+1,(gchar **)&base64_out);
  if (rc)
    goto free_output;

  for(num_len=1,tmp_num=iterations;tmp_num;++num_len,tmp_num/=10);

  result_len=strlen(base64_out)+strlen(base64_salt)+num_len+PBKDF2_FORMAT_ADDED_LEN+1;
  dbg_out("result_len:%d\n",result_len);
  ret_str=g_malloc(result_len);

  rc=-ENOMEM;
  if (!ret_str)
    goto free_base64_encoded_out;
  snprintf(ret_str,result_len,PBKDF2_FORMAT,base64_salt,iterations,base64_out);
  ret_str[result_len-1]='\0';
  dbg_out("ret_str:%s\n",ret_str);
  *enc_pass=ret_str;  
  rc=0;

 free_base64_encoded_out:
  if (base64_out)
    g_free(base64_out);
 free_output:
  if (output)
    g_free(output);
 free_base64_encoded_salt:
  if (base64_salt)
    g_free(base64_salt);
 free_raw_salt:
  if (raw_salt)
    g_free(raw_salt);

  return rc;
}

int
pbkdf2_verify(const char *plain_passwd,const char *crypt_passwd){
  int rc;
  int match = -EPERM;
  char *pbkdf2_result = NULL;

  if ( (!plain_passwd) || (!crypt_passwd) )
    return -EINVAL;

  rc = pbkdf2_encrypt(plain_passwd,crypt_passwd,&pbkdf2_result);
  if (rc != 0)
    goto error_out;

  match = strcmp(pbkdf2_result,crypt_passwd);

 error_out:
  if (pbkdf2_result != NULL)
    g_free(pbkdf2_result);

  dbg_out("comp:%d\n",match);

  return match;
}

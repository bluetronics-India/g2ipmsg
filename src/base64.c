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

#if !GLIB_CHECK_VERSION(2,12,0)

static char base64_encode_table[64] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static const unsigned char base64_decode_table[256] = {
  255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
  255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
  255,255,255,255,255,255,255,255,255,255,255, 62,255,255,255, 63,
   52, 53, 54, 55, 56, 57, 58, 59, 60, 61,255,255,255,  0,255,255,
  255,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
   15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25,255,255,255,255,255,
  255, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
   41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51,255,255,255,255,255,
  255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
  255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
  255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
  255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
  255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
  255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
  255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
  255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
};

static gchar *
compat_base64_encode(const guchar *data,gsize len){
  unsigned char *output=NULL;
  unsigned char *out_ptr;
  size_t i,allocated_size;
  unsigned char buff[4];
  unsigned char in_ch;
  unsigned int  upper,lower;
  unsigned int  mod,pad_len;
  unsigned int  offset;
  unsigned int  count;
  g_assert(data);

  mod=len % 3;
  pad_len=(3 - mod);

  allocated_size=(len / 3 + 1) * 4 + 1; /* NULL Terminate */

  output = (unsigned char *)g_malloc(allocated_size);
  if (!output)
    return NULL;
  memset(output,0,allocated_size);

  count=0;

  for(i=0,out_ptr=output;i<len;++i) {
    
    offset=i % 3;
    in_ch=data[i];
    switch(offset) {
    case 0:
      buff[0]=(in_ch >> 2);
      buff[1]=( (in_ch & 0x3) << 4);
      *out_ptr++ = base64_encode_table[buff[0]];
      ++count;
      break;
    case 1:
      buff[1] |= ( (in_ch) >> 4);
      buff[2] =  ( (in_ch) & 0xf)<<2;
      *out_ptr++ = base64_encode_table[buff[1]];
      ++count;
      break;
    case 2:
      buff[2] |= ( (in_ch) >> 6);
      buff[3] =  ( (in_ch) & 0x3f);
      *out_ptr++ =base64_encode_table[buff[2]];
      *out_ptr++ =base64_encode_table[buff[3]];
      count +=2;
      break;
    }
  }

  /*
   * 余り分の処理
   */
  switch(pad_len) {
  case 2:
    *out_ptr++ = base64_encode_table[buff[1]];
    *out_ptr++ = '=';
    *out_ptr++ = '=';
      count +=3;
    break;
  case 1:
    *out_ptr++ = base64_encode_table[buff[2]];
    *out_ptr++ = '=';
      count +=2;
    break;
  default:
    break;
  }
  *out_ptr='\0';
  ++count;

  g_assert(output);
  return output;
 error_out:
  g_free(output);
  return NULL;
}

static guchar *
compat_base64_decode(unsigned char *in, size_t *out_len){
  char buff[2];
  char *in_ptr,*out_ptr;
  char *output;
  size_t len;
  int i,offset;
  int in_ch,out_ch;
  u_int32_t val;

  if ( (!in) || (!out_len) )
    return NULL;
  len=strlen(in);
  output=g_malloc(len*3/4);
  if (!output)
    return NULL;

  in_ptr=in;
  memset(buff,0,2);
  out_ptr=output;
  for(i=0,val=0,offset=0;i<len;++i) {
    in_ch=in_ptr[i];
    ++offset;
    out_ch=base64_decode_table[in_ch];
    if (out_ch==255) {
      ipmsg_err_dialog("%s:%d,%c\n", _("Invalid char"), in_ch, in_ch);
      goto free_out;
    }
    buff[1]=buff[0];
    buff[0]=in_ch;
    val = (val << 6)|(out_ch);
    if (offset==4) {
      offset=0;
      *out_ptr=(val>>16);
      ++out_ptr;
      if (buff[1] != '=') {
	*out_ptr=(val>>8);
	++out_ptr;
      }
      if (buff[0] != '=') {
	*out_ptr=val;
	++out_ptr;
      }
    }
  }
  *out_len=out_ptr - output;
  return output;
 free_out:
  g_free(output);
  return NULL;  
}
#endif  /*   !GLIB_CHECK_VERSION(2,12,0)  */

/*
 * Interfaces 
 */
int
base64_encode(const guchar *data,size_t len,gchar **output){
  gchar *ret_str=NULL;

  if ( (!output) || (!data) )
    return -EINVAL;

#if GLIB_CHECK_VERSION(2,12,0)
    ret_str=g_base64_encode(data,len);
#else
    ret_str=compat_base64_encode(data,len);
#endif  /*  GLIB_CHECK_VERSION(2,12,0)  */

  if (!ret_str)
    return -ENOMEM;

  *output=ret_str;
  return 0;
}
int
base64_decode(const guchar *data,size_t *len,gchar **output){
  gchar *ret_str=NULL;

  if ( (!output) || (!data) || (!len) )
    return -EINVAL;

#if GLIB_CHECK_VERSION(2,12,0)
    ret_str=g_base64_decode(data,len);
#else
    ret_str=compat_base64_decode(data,len);
#endif  /*  GLIB_CHECK_VERSION(2,12,0)  */

  if (!ret_str)
    return -ENOMEM;

  *output=ret_str;
  return 0;
}

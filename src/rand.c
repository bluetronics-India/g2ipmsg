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

static int
check_random_seed(void){
  int rc;
  unsigned short seed_in;
  int retry;



#if defined(HAVE_RAND_POLL)
  RAND_poll();
#endif  /*  HAVE_RAND_POLL  */

  for (rc=RAND_status(),retry=CRYPT_RND_MAX_RETRY;(!rc)&&(retry);rc=RAND_status(),--retry) {
    seed_in = rand() & 0xffff;      
    RAND_seed(&seed_in, sizeof(seed_in));
  }

  rc = (rc)?(0):(-EBUSY);

  return rc;
}

int 
add_timing_entropy(void) {
  /*  FIXME add implementation */
}

int
generate_rand(unsigned char *buf,size_t len){
  int      rc = -EAGAIN;
  int rand_ok = 0;
  int   retry = CRYPT_RND_MAX_RETRY;

  if (check_random_seed()) {
    err_out("Can not seed random\n");
    g_assert_not_reached();
  }

  do{
    memset(buf,0,len);    
    rand_ok = RAND_bytes(buf,len);
    if (rand_ok == 1) {
      rc = 0;
      break;
    }
  }while( (rand_ok != 1) && (--retry) );

  return rc;
}

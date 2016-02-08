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

#if !defined(MSGOUT_H)
#define MSGOUT_H

//#define DEBUG 1
#if defined(DEBUG)
#define dbg_out(fmt,arg...) do{                                         \
    fprintf(stderr, "[file:%s function: %s line:%d ] " fmt,__FILE__,__FUNCTION__,__LINE__,##arg); \
    syslog(LOG_INFO|LOG_USER, "[file:%s function: %s line:%d ] " fmt,__FILE__,__FUNCTION__,__LINE__,##arg); \
    fflush(stderr);              \
  }while(0)
#else
#define dbg_out(fmt,arg...) do{         \
    if (hostinfo_refer_debug_state()){	\
      fprintf(stderr, "[file:%s function: %s line:%d ] " fmt,__FILE__,__FUNCTION__,__LINE__,##arg); \
      syslog(LOG_INFO|LOG_USER, "[file:%s function: %s line:%d ] " fmt,__FILE__,__FUNCTION__,__LINE__,##arg); \
      fflush(stderr);							\
    }                            \
  }while(0)      
#endif
#define war_out(fmt,arg...) do{                                                                                  \
    if (hostinfo_refer_debug_state()){					                                         \
      fprintf(stderr, "Warning : [file:%s line:%d ] " fmt,__FILE__,__LINE__,##arg);                              \
      fflush(stderr);							                                         \
      syslog(LOG_WARNING|LOG_USER, "[file:%s function: %s line:%d ] " fmt,__FILE__,__FUNCTION__,__LINE__,##arg); \
    }                                                                                                            \
  }while(0)

#define err_out(fmt,arg...) do{                                                        \
      fprintf(stderr, "Error : [file:%s line:%d ] " fmt,__FILE__,__LINE__,##arg);      \
      fflush(stderr);							               \
    if (hostinfo_refer_debug_state()){	                                               \
      syslog(LOG_ERR|LOG_USER, "[file:%s function: %s line:%d ] " fmt,__FILE__,__FUNCTION__,__LINE__,##arg); \
    }                                                                                 \
  }while(0)

#define _assert(cond) do{                                                   \
  if (!(cond)) {                                                            \
     fprintf(stderr, "Assertion : [file:%s line:%d ]\n",__FILE__,__LINE__); \
     fflush(stderr);                                                        \
     abort();								    \
    }                                                                       \
  }while(0)

#endif

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

#if !defined(LOGFILE_H)
#define LOGFILE_H

#define LOGFILE_START_HEADER "====================================="
#define LOGFILE_END_HEADER   "-------------------------------------"
#define LOGFILE_NEW_LINE     "\n"
#define LOGFILE_MAX_LINE_LEN 0x1000
#define LOGFILE_FMT             " %s %s %s (%s/%s%s)"
#define LOGFILE_TO_STR "To:"
#define LOGFILE_FROM_STR "From:"
#define LOGFILE_SND_HLIST_STR "SEND HOST LIST:"

int logfile_init_logfile(void);
int logfile_reopen_logfile(const char *filepath);
int logfile_send_log(const char *ipaddr, const ipmsg_send_flags_t flags, const char *message);
int logfile_recv_log(const char *ipaddr, const ipmsg_send_flags_t flags, const char *message);
int logfile_shutdown_logfile(void);

#endif  /*  LOGFILE_H  */

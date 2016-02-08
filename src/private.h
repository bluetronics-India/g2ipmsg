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

#if !defined(PRIVATE_H)
#define PRIVATE_H
#define IPMSG_PRIVATE_DATA_MAGIC 0x19731219
#define IPMSG_PRIVATE_NO_TYPE 0
#define IPMSG_PRIVATE_RECVMSG 1
#define IPMSG_PRIVATE_AFCB    2
#define IPMSG_PRIVATE_PASSWD  3

typedef struct _ipmsg_private_data{
  int magic;
  int type;
  void *data;
}ipmsg_private_data_t;

typedef struct _ipmsg_recvmsg_private{
  char    *ipaddr;
  pktno_t   pktno;
  int       flags;
  char  *ext_part;
}ipmsg_recvmsg_private_t;

#define IPMSG_ASSERT_PRIVATE(private,ptype) do{			 \
    g_assert((private));					 \
    g_assert((private)->magic == IPMSG_PRIVATE_DATA_MAGIC);	 \
    g_assert((private)->type == (ptype));			 \
  }while(0)

#define IPMSG_HOOKUP_DATA(component,data_p,name) \
  g_object_set_data_full (G_OBJECT (component), name, \
    data_p, (GDestroyNotify) destroy_ipmsg_private)

int init_ipmsg_private(ipmsg_private_data_t **priv,const int type);
int init_recvmsg_private(const char *ipaddr,int flags,int pktno, ipmsg_private_data_t **priv);
void destroy_ipmsg_private(gpointer data);

#endif

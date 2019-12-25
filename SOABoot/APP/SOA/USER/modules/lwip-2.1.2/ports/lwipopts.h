/*
 * Copyright (c) 2001-2003 Swedish Institute of Computer Science.
 * All rights reserved. 
 * 
 * Redistribution and use in source and binary forms, with or without modification, 
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED 
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF 
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT 
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT 
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING 
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY 
 * OF SUCH DAMAGE.
 *
 * This file is part of the lwIP TCP/IP stack.
 * 
 * Author: Simon Goldschmidt
 *
 */
#ifndef LWIP_HDR_LWIPOPTS_H
#define LWIP_HDR_LWIPOPTS_H

#define LWIP_TESTMODE                   0

#define LWIP_IPV6                       0

#define LWIP_CHECKSUM_ON_COPY           1
#define TCP_CHECKSUM_ON_COPY_SANITY_CHECK 1
#define TCP_CHECKSUM_ON_COPY_SANITY_CHECK_FAIL(printfmsg) LWIP_ASSERT("TCP_CHECKSUM_ON_COPY_SANITY_CHECK_FAIL", 0)

/* We link to special sys_arch.c (for basic non-waiting API layers unit tests) */
#define NO_SYS                          0
#define SYS_LIGHTWEIGHT_PROT            0
#define LWIP_NETCONN                    !NO_SYS
#define LWIP_SOCKET                     !NO_SYS
#define LWIP_NETCONN_FULLDUPLEX         LWIP_SOCKET
#define LWIP_NETBUF_RECVINFO            1
#define LWIP_HAVE_LOOPIF                1
#define TCPIP_THREAD_TEST

/* Enable DHCP to test it, disable UDP checksum to easier inject packets */
#define LWIP_DHCP                       0
#define DHCP_DEBUG                      LWIP_DBG_OFF
/* Minimal changes to opt.h required for tcp unit tests: */
#define MEM_SIZE                        1024 * 2   /*内存堆的大小 mem_alloc和mem_free使用 */
#define TCP_SND_QUEUELEN                10
#define MEMP_NUM_TCP_SEG                TCP_SND_QUEUELEN
#define TCP_SND_BUF                     (2 * TCP_MSS)
#define TCP_WND                         (2 * TCP_MSS)
#define LWIP_WND_SCALE                  1
#define TCP_RCV_SCALE                   0
#define PBUF_POOL_SIZE                  20 /* 缓存池的个数 */
/*#define PBUF_POOL_BUFSIZE               LWIP_MEM_ALIGN_SIZE(TCP_MSS+40+PBUF_LINK_ENCAPSULATION_HLEN+PBUF_LINK_HLEN)*/
	 
#define LWIP_TCP						1
#define TCP_TTL							255
#define LWIP_UDP						1
#define UDP_TTL							255
#define LWIP_DEBUG                    	LWIP_DBG_ON
/* Enable IGMP and MDNS for MDNS tests */
#define LWIP_IGMP                       1
#define LWIP_MDNS_RESPONDER             1
#define LWIP_NUM_NETIF_CLIENT_DATA      (LWIP_MDNS_RESPONDER)

/* Minimal changes to opt.h required for etharp unit tests: */
#define ETHARP_SUPPORT_STATIC_ENTRIES   1

#define MEMP_NUM_SYS_TIMEOUT            (LWIP_NUM_SYS_TIMEOUT_INTERNAL + 8)

/* MIB2 stats are required to check IPv4 reassembly results */
#define MIB2_STATS                      1

/* netif tests want to test this, so enable: */
#define LWIP_NETIF_EXT_STATUS_CALLBACK  1

/* Check lwip_stats.mem.illegal instead of asserting */
#define LWIP_MEM_ILLEGAL_FREE(msg)      /* to nothing */

/* add */
#define LWIP_PROVIDE_ERRNO				1
#define LWIP_TCPIP_CORE_LOCKING         0

#define TCPIP_MBOX_SIZE                 20
#define TCPIP_THREAD_STACKSIZE          512
#define TCPIP_THREAD_PRIO               1
#define LWIP_TCPIP_TIMEOUT              1
#define LWIP_SO_RCVTIMEO                1 
#define LWIP_SO_SNDTIMEO                0

#define DEFAULT_UDP_RECVMBOX_SIZE       100
#define DEFAULT_TCP_RECVMBOX_SIZE       100
#endif /* LWIP_HDR_LWIPOPTS_H */

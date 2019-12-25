/*
 * Copyright (c) 2017 Simon Goldschmidt
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
 * Author: Simon Goldschmdit <goldsimon@gmx.de>
 *
 */
#ifndef LWIP_ARCH_SYS_ARCH_H
#define LWIP_ARCH_SYS_ARCH_H

#include "lwip/opt.h"
#include "lwip/arch.h"
#include "os.h"
#include "lib_mem.h"

#define SYS_ARCH_DECL_PROTECT(lev)	u32_t lev
#define SYS_ARCH_PROTECT(lev)		lev = CPU_SR_Save(CPU_CFG_KA_IPL_BOUNDARY << (8u - CPU_CFG_NVIC_PRIO_BITS))
#define SYS_ARCH_UNPROTECT(lev)		CPU_SR_Restore(lev)

void sys_arch_msleep(u32_t delay_ms);
#define sys_msleep(ms) sys_arch_msleep(ms)


#if SYS_LIGHTWEIGHT_PROT
typedef u32_t sys_prot_t;
#endif
/*
#if !LWIP_COMPAT_MUTEX
struct _sys_mut {
  void *mut;
};
typedef struct _sys_mut sys_mutex_t;
#define sys_mutex_valid_val(mutex)   ((mutex).mut != NULL)
#define sys_mutex_valid(mutex)       (((mutex) != NULL) && sys_mutex_valid_val(*(mutex)))
#define sys_mutex_set_invalid(mutex) ((mutex)->mut = NULL)
#endif

struct _sys_sem {
  void *sem;
};
typedef struct _sys_sem sys_sem_t;
#define sys_sem_valid_val(sema)   ((sema).sem != NULL)
#define sys_sem_valid(sema)       (((sema) != NULL) && sys_sem_valid_val(*(sema)))
#define sys_sem_set_invalid(sema) ((sema)->sem = NULL)

struct _sys_mbox {
  void *mbx;
};
typedef struct _sys_mbox sys_mbox_t;
#define sys_mbox_valid_val(mbox)   ((mbox).mbx != NULL)
#define sys_mbox_valid(mbox)       (((mbox) != NULL) && sys_mbox_valid_val(*(mbox)))
#define sys_mbox_set_invalid(mbox) ((mbox)->mbx = NULL)

struct _sys_thread {
  void *thread_handle;
};
typedef struct _sys_thread sys_thread_t;

#if LWIP_NETCONN_SEM_PER_THREAD
sys_sem_t* sys_arch_netconn_sem_get(void);
void sys_arch_netconn_sem_alloc(void);
void sys_arch_netconn_sem_free(void);
#define LWIP_NETCONN_THREAD_SEM_GET()   sys_arch_netconn_sem_get()
#define LWIP_NETCONN_THREAD_SEM_ALLOC() sys_arch_netconn_sem_alloc()
#define LWIP_NETCONN_THREAD_SEM_FREE()  sys_arch_netconn_sem_free()
#endif
*/

/* The user can change this priority level. 
 * It is important that there was no crossing with other levels.
 */

#define SYS_MBOX_NULL   (void*)0 
#define SYS_SEM_NULL    (void*)0 
   
/** struct of LwIP mailbox */
typedef OS_SEM     sys_sem_t;
typedef OS_MUTEX   sys_mutex_t;
typedef OS_Q       sys_mbox_t;
typedef CPU_INT08U sys_thread_t;


#endif /* LWIP_ARCH_SYS_ARCH_H */

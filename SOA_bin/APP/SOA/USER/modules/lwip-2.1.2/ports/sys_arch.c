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
 * Author: Adam Dunkels <adam@sics.se>
 *
 */

/* lwIP includes. */
#include "lwip/debug.h"
#include "lwip/def.h"
#include "lwip/sys.h"
#include "lwip/mem.h"
#include "lwip/stats.h"
#include <string.h>
#if !NO_SYS

#define LWIP_ARCH_TICK_PER_MS       1000/OS_CFG_TICK_RATE_HZ


//当消息指针为空时,指向一个常量pvNullPointer所指向的值.
//在UCOS中如果OSQPost()中的msg==NULL会返回一条OS_ERR_POST_NULL
//错误,而在lwip中会调用sys_mbox_post(mbox,NULL)发送一条空消息,我们
//在本函数中把NULL变成一个常量指针0xFFFFFFFF
const void * const pvNullPointer = (mem_ptr_t*)0xFFFFFFFF;

#pragma pack(4)
CPU_STK       LwIP_Task_Stk[TCPIP_THREAD_STACKSIZE];
OS_TCB        LwIP_Task_TCB;

/*-----------------------------------------------------------------------------------*/
//  Creates an empty mailbox.
err_t sys_mbox_new(sys_mbox_t *mbox, int size)
{
	OS_ERR       ucErr;
      
	OSQCreate(mbox,"lwip queue", size, &ucErr); 
	LWIP_ASSERT( "OSQCreate ", ucErr == OS_ERR_NONE );
  
	if( ucErr == OS_ERR_NONE)
	{ 
		return 0; 
	}
	return -1;
}

/*-----------------------------------------------------------------------------------*/
/*
  Deallocates a mailbox. If there are messages still present in the
  mailbox when the mailbox is deallocated, it is an indication of a
  programming error in lwIP and the developer should be notified.
*/
void sys_mbox_free(sys_mbox_t *mbox)
{
    OS_ERR     ucErr;
    LWIP_ASSERT( "sys_mbox_free ", mbox != SYS_MBOX_NULL );      
        
    OSQFlush(mbox,& ucErr);
    
    OSQDel(mbox, OS_OPT_DEL_ALWAYS, &ucErr);
    LWIP_ASSERT( "OSQDel ", ucErr == OS_ERR_NONE );
}

/*-----------------------------------------------------------------------------------*/
//   Posts the "msg" to the mailbox.
void sys_mbox_post(sys_mbox_t *mbox, void *msg)
{
	OS_ERR     ucErr;
	if( msg == NULL ) 
		msg = (void*)&pvNullPointer;
	while(1)
	{
		OSQPost(mbox, msg,0,OS_OPT_POST_ALL,&ucErr);
		if(ucErr == OS_ERR_NONE)
			break;
		OSTimeDly(5,OS_OPT_TIME_DLY,&ucErr);
	}
}


/*-----------------------------------------------------------------------------------*/
//   Try to post the "msg" to the mailbox.
err_t sys_mbox_trypost(sys_mbox_t *mbox, void *msg)
{
	OS_ERR     ucErr;
	if(msg == NULL )
		msg = (void*)&pvNullPointer;  
	OSQPost(mbox, msg,0,OS_OPT_POST_ALL,&ucErr);    
  	if(ucErr != OS_ERR_NONE)
	{
    	return ERR_MEM;
  	}
  	return ERR_OK;	
}

/*-----------------------------------------------------------------------------------*/
/*
  Blocks the thread until a message arrives in the mailbox, but does
  not block the thread longer than "timeout" milliseconds (similar to
  the sys_arch_sem_wait() function). The "msg" argument is a result
  parameter that is set by the function (i.e., by doing "*msg =
  ptr"). The "msg" parameter maybe NULL to indicate that the message
  should be dropped.

  The return values are the same as for the sys_arch_sem_wait() function:
  Number of milliseconds spent waiting or SYS_ARCH_TIMEOUT if there was a
  timeout.

  Note that a function with a similar name, sys_mbox_fetch(), is
  implemented by lwIP.
*/
u32_t sys_arch_mbox_fetch(sys_mbox_t *mbox, void **msg, u32_t timeout)
{
  	OS_ERR	ucErr;
  	OS_MSG_SIZE   msg_size;
  	CPU_TS        ucos_timeout , timeout_now , timeout_new;  
  	CPU_TS        in_timeout = timeout/LWIP_ARCH_TICK_PER_MS;
  	if((timeout != 0) && (in_timeout == 0))
    	in_timeout = 1;
	timeout_now = OSTimeGet(&ucErr);
  	*msg  = OSQPend (mbox,in_timeout,OS_OPT_PEND_BLOCKING,&msg_size, &ucos_timeout,&ucErr);
  	if ( ucErr == OS_ERR_TIMEOUT )
      	timeout_new = SYS_ARCH_TIMEOUT;  
  	else
  	{
	  	timeout_new = OSTimeGet(&ucErr);
		if (timeout_new > timeout_now) 
			timeout_new = timeout_new - timeout;
		else 
			timeout_new = 0xffffffff - timeout_now + timeout_new; 
	  	timeout_new = timeout_new * LWIP_ARCH_TICK_PER_MS + 1; 
  	}
  	return timeout_new; 
}

/*-----------------------------------------------------------------------------------*/
/*
  Similar to sys_arch_mbox_fetch, but if message is not ready immediately, we'll
  return with SYS_MBOX_EMPTY.  On success, 0 is returned.
*/
u32_t sys_arch_mbox_tryfetch(sys_mbox_t *mbox, void **msg)
{
	return sys_arch_mbox_fetch(mbox,msg,1);//尝试获取一个消息
}
/*----------------------------------------------------------------------------------*/
int sys_mbox_valid(sys_mbox_t *mbox)
{
	if(mbox->NamePtr)  
		return (strcmp(mbox->NamePtr,"?Q"))? 1:0;
	else
		return 0;
}
/*-----------------------------------------------------------------------------------*/
void sys_mbox_set_invalid(sys_mbox_t *mbox)
{
	if(sys_mbox_valid(mbox))
		sys_mbox_free(mbox);
}

/*-----------------------------------------------------------------------------------*/
//  Creates a new semaphore. The "count" argument specifies
//  the initial state of the semaphore.
err_t sys_sem_new(sys_sem_t *sem, u8_t count)
{
  	OS_ERR	ucErr;
  	OSSemCreate (sem,"LWIP Sem",count,&ucErr);
  	if(ucErr != OS_ERR_NONE )
	{
    	LWIP_ASSERT("OSSemCreate ",ucErr == OS_ERR_NONE );
    	return -1;    
	}
  	return 0;
}

/*-----------------------------------------------------------------------------------*/
/*
  Blocks the thread while waiting for the semaphore to be
  signaled. If the "timeout" argument is non-zero, the thread should
  only be blocked for the specified time (measured in
  milliseconds).

  If the timeout argument is non-zero, the return value is the number of
  milliseconds spent waiting for the semaphore to be signaled. If the
  semaphore wasn't signaled within the specified time, the return value is
  SYS_ARCH_TIMEOUT. If the thread didn't have to wait for the semaphore
  (i.e., it was already signaled), the function may return zero.

  Notice that lwIP implements a function with a similar name,
  sys_sem_wait(), that uses the sys_arch_sem_wait() function.
*/
u32_t sys_arch_sem_wait(sys_sem_t *sem, u32_t timeout)
{
	OS_ERR	ucErr;
	CPU_TS        ucos_timeout , timeout_now , timeout_new;
	CPU_TS        in_timeout = timeout / LWIP_ARCH_TICK_PER_MS;
	if((timeout != 0) && (in_timeout == 0))
		in_timeout = 1; 
	timeout_now = OSTimeGet(&ucErr);
	OSSemPend (sem,in_timeout,OS_OPT_PEND_BLOCKING,&ucos_timeout,&ucErr);
	
    /*  only when timeout! */
	if(ucErr == OS_ERR_TIMEOUT)
		timeout_new = SYS_ARCH_TIMEOUT;	
	else
	{
	  	timeout_new = OSTimeGet(&ucErr);
		if (timeout_new > timeout_now) 
			timeout_new = timeout_new - timeout;
		else 
			timeout_new = 0xffffffff - timeout_now + timeout_new; 
	  	timeout_new = timeout_new * LWIP_ARCH_TICK_PER_MS + 1; 		
	}
  return timeout_new;
}

/*-----------------------------------------------------------------------------------*/
// Signals a semaphore
void sys_sem_signal(sys_sem_t *sem)
{
	OS_ERR	ucErr;  
	OSSemPost(sem,OS_OPT_POST_ALL,&ucErr);
	LWIP_ASSERT("OSSemPost ",ucErr == OS_ERR_NONE );
}

/*-----------------------------------------------------------------------------------*/
// Deallocates a semaphore
void sys_sem_free(sys_sem_t *sem)
{
    OS_ERR     ucErr;
    OSSemDel(sem, OS_OPT_DEL_ALWAYS, &ucErr );
    LWIP_ASSERT( "OSSemDel ", ucErr == OS_ERR_NONE );
}
/*-----------------------------------------------------------------------------------*/
int sys_sem_valid(sys_sem_t *sem)
{
	if(sem->NamePtr)
		return (strcmp(sem->NamePtr,"?SEM"))? 1:0;
	else
		return 0;
}

/*-----------------------------------------------------------------------------------*/
void sys_sem_set_invalid(sys_sem_t *sem)
{
	if(sys_sem_valid(sem))
		sys_sem_free(sem);
}

/*-----------------------------------------------------------------------------------*/
// Initialize sys arch
void sys_init(void)
{

}
/*-----------------------------------------------------------------------------------*/
                                      /* Mutexes*/
/*-----------------------------------------------------------------------------------*/
/*-----------------------------------------------------------------------------------*/
#if LWIP_COMPAT_MUTEX == 0
/* Create a new mutex*/
err_t sys_mutex_new(sys_mutex_t *mutex) 
{
  	OS_ERR	ucErr;
  	OSMutexCreate (mutex,"LWIP Mutex",&ucErr);
  	if(ucErr != OS_ERR_NONE )
	{
    	LWIP_ASSERT("OSMutexCreate ",ucErr == OS_ERR_NONE );
    	return -1;
	}
  	return ERR_OK;
}
/*-----------------------------------------------------------------------------------*/
/* Deallocate a mutex*/
void sys_mutex_free(sys_mutex_t *mutex)
{
    OS_ERR     ucErr;
    OSMutexDel(mutex, OS_OPT_DEL_ALWAYS, &ucErr );
    LWIP_ASSERT( "OSMutexDel ", ucErr == OS_ERR_NONE );
}
/*-----------------------------------------------------------------------------------*/
/* Lock a mutex*/
void sys_mutex_lock(sys_mutex_t *mutex)
{
	OS_ERR     ucErr;
	OSMutexPend (mutex,0,OS_OPT_PEND_BLOCKING,NULL,&ucErr);
	LWIP_ASSERT("OSMutexPend ",ucErr == OS_ERR_NONE );
}
/*-----------------------------------------------------------------------------------*/
/* Unlock a mutex*/
void sys_mutex_unlock(sys_mutex_t *mutex)
{
	OS_ERR     ucErr;
	OSMutexPost (mutex,OS_OPT_POST_NONE,&ucErr);
	LWIP_ASSERT("OSMutexPost ",ucErr == OS_ERR_NONE );
}
#endif /*LWIP_COMPAT_MUTEX*/
/*-----------------------------------------------------------------------------------*/
// TODO
/*-----------------------------------------------------------------------------------*/
/*
  Starts a new thread with priority "prio" that will begin its execution in the
  function "thread()". The "arg" argument will be passed as an argument to the
  thread() function. The id of the new thread is returned. Both the id and
  the priority are system dependent.
*/
sys_thread_t sys_thread_new(const char *name, lwip_thread_fn thread , void *arg, int stacksize, int prio)
{
    OS_ERR      ucErr;  
    OSTaskCreate((OS_TCB     *)&LwIP_Task_TCB,
                 (CPU_CHAR   *)name,
                 (OS_TASK_PTR )thread, 
                 (void       *)0,
                 (OS_PRIO     )prio,
                 (CPU_STK    *)&LwIP_Task_Stk[0],
                 (CPU_STK_SIZE)stacksize/10,
                 (CPU_STK_SIZE)stacksize,
                 (OS_MSG_QTY  )0,
                 (OS_TICK     )0,
                 (void       *)0,
                 (OS_OPT      )(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
                 (OS_ERR     *)&ucErr);  
	
	LWIP_ASSERT("sys_thread_new ",ucErr == OS_ERR_NONE );						
    return ucErr;
}


void sys_msleep(u32_t ms)
{
  OS_ERR      ucErr;  
  OSTimeDly(ms,OS_OPT_TIME_DLY,&ucErr);  
}
#endif /* !NO_SYS */

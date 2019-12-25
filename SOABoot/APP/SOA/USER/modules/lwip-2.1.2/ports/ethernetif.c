/**
  ******************************************************************************
  * File Name          : ethernetif.c
  * Description        : This file provides code for the configuration
  *                      of the ethernetif.c MiddleWare.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2019 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "lwip/opt.h"

#include "lwip/timeouts.h"
#include "netif/ethernet.h"
#include "netif/etharp.h"
#include "lwip/ethip6.h"
#include "ethernetif.h"

#include <string.h>
#include <assert.h>

#include "stm32f1xx_hal.h"
#include "dm9000.h"
#include "lwip.h"

/* Network interface name */
#define IFNAME0 's'
#define IFNAME1 't'




/*该任务用于中断同步后从mac层接收数据*/
#define INTERFACE_THREAD_STACK_SIZE ( 256 )
#define INTERFACE_THREAD_PRIO		( 2 )
static  OS_TCB   lwip_interface_tcb;
static  CPU_STK  lwip_interface_stk[INTERFACE_THREAD_STACK_SIZE];


OS_SEM  LwipSyncSem;
OS_MUTEX  dm9000Mutex;
/* Private functions ---------------------------------------------------------*/


/*******************************************************************************
                       LL Driver Interface ( LwIP stack --> ETH) 
*******************************************************************************/
/**
 * In this function, the hardware should be initialized.
 * Called from ethernetif_init().
 *
 * @param netif the already initialized lwip network interface structure
 *        for this ethernetif
 */
static void low_level_init(struct netif *netif)
{ 
#if LWIP_ARP || LWIP_ETHERNET 

  /* set MAC hardware address length */
  netif->hwaddr_len = ETH_HWADDR_LEN;
  
  uint32_t temp = *(uint32_t*)(0x1FFFF7E8);	
  /* set MAC hardware address */
  netif->hwaddr[0] =  2;
  netif->hwaddr[1] =  0;
  netif->hwaddr[2] =  0;
  netif->hwaddr[3] =  (temp >> 16) & 0xFF;
  netif->hwaddr[4] =  (temp >> 8 ) & 0xFF;
  netif->hwaddr[5] =  (temp >> 0 ) & 0xFF;
  
  /* maximum transfer unit */
  netif->mtu = 1500;
  
  /* Accept broadcast address and ARP traffic */
  /* don't set NETIF_FLAG_ETHARP if this device is not an ethernet one */
  #if LWIP_ARP
    netif->flags |= NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_LINK_UP;
  #else 
    netif->flags |= NETIF_FLAG_BROADCAST;
  #endif /* LWIP_ARP */
  
/* create a binary semaphore used for informing ethernetif of frame reception */
  OS_ERR err;
  OSSemCreate (&LwipSyncSem,"lwip sync sem",0,&err);
  assert(err == OS_ERR_NONE);
  
  OSMutexCreate (&dm9000Mutex,"dm9000 mutex",&err);
  assert(err == OS_ERR_NONE);
/* create the task that handles the ETH_MAC */
  OSTaskCreate((OS_TCB     *)&lwip_interface_tcb,                               /* Create the startup task                              */
               (CPU_CHAR   *)"ethernetif_thread",
               (OS_TASK_PTR )ethernetif_input,
               (void       *)0u,
               (OS_PRIO     )INTERFACE_THREAD_PRIO,
               (CPU_STK    *)&lwip_interface_stk[0u],
               (CPU_STK_SIZE)INTERFACE_THREAD_STACK_SIZE / 10u,
               (CPU_STK_SIZE)INTERFACE_THREAD_STACK_SIZE,
               (OS_MSG_QTY  )0u,
               (OS_TICK     )0u,
               (void       *)0u,
               (OS_OPT      )(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
               (OS_ERR     *)&err);
  assert(err == OS_ERR_NONE);
  /* Enable MAC and DMA transmission and reception */

	if(DM9000_Init())
		printf("DM9000_Init fail!\r\n");
#endif /* LWIP_ARP || LWIP_ETHERNET */
}

/**
 * This function should do the actual transmission of the packet. The packet is
 * contained in the pbuf that is passed to the function. This pbuf
 * might be chained.
 *
 * @param netif the lwip network interface structure for this ethernetif
 * @param p the MAC packet to send (e.g. IP packet including MAC addresses and type)
 * @return ERR_OK if the packet could be sent
 *         an err_t value if the packet couldn't be sent
 *
 * @note Returning ERR_MEM here if a DMA queue of your MAC is full can lead to
 *       strange results. You might consider waiting for space in the DMA queue
 *       to become availale since the stack doesn't retry to send a packet
 *       dropped because of memory failure (except for the TCP timers).
 */

static err_t low_level_output(struct netif *netif, struct pbuf *p)
{
	(void)netif;
	DM9000_SendPacket(p);
	return ERR_OK;
}

/**
 * Should allocate a pbuf and transfer the bytes of the incoming
 * packet from the interface into the pbuf.
 *
 * @param netif the lwip network interface structure for this ethernetif
 * @return a pbuf filled with the received packet (including MAC header)
 *         NULL on memory error
   */
static struct pbuf * low_level_input(struct netif *netif)
{
	struct pbuf *buf = NULL;
	buf = DM9000_Receive_Packet();
	return buf;
}

/**
 * This function should be called when a packet is ready to be read
 * from the interface. It uses the function low_level_input() that
 * should handle the actual reception of bytes from the network
 * interface. Then the type of the received packet is determined and
 * the appropriate input function is called.
 *
 * @param netif the lwip network interface structure for this ethernetif
 */
void ethernetif_input( void * argument )
{
	struct pbuf *p = NULL;
	(void)argument;
	struct netif *netif = (struct netif *) &gnetif;
	OS_ERR err;
	//uint32_t count = 0;
	while (DEF_TRUE)
  	{
    	OSSemPend (&LwipSyncSem,0,OS_OPT_PEND_BLOCKING,NULL,&err);
    	if (err == OS_ERR_NONE)
    	{
      		do
      		{
        		p = low_level_input( netif );				
        		if(p != NULL)
        		{
          			if (netif->input( p, netif) != ERR_OK )
          			{
            			pbuf_free(p);
						p = NULL;
          			}
        		}
      		} while(p!=NULL);
    	}
  	}
}

#if !LWIP_ARP
/**
 * This function has to be completed by user in case of ARP OFF.
 *
 * @param netif the lwip network interface structure for this ethernetif
 * @return ERR_OK if ...
 */
static err_t low_level_output_arp_off(struct netif *netif, struct pbuf *q, const ip4_addr_t *ipaddr)
{  
  err_t errval;
  errval = ERR_OK;
    
    
  return errval;
  
}
#endif /* LWIP_ARP */ 

/**
 * Should be called at the beginning of the program to set up the
 * network interface. It calls the function low_level_init() to do the
 * actual setup of the hardware.
 *
 * This function should be passed as a parameter to netif_add().
 *
 * @param netif the lwip network interface structure for this ethernetif
 * @return ERR_OK if the loopif is initialized
 *         ERR_MEM if private data couldn't be allocated
 *         any other err_t on error
 */
err_t ethernetif_init(struct netif *netif)
{
  LWIP_ASSERT("netif != NULL", (netif != NULL));
  
#if LWIP_NETIF_HOSTNAME
  /* Initialize interface hostname */
  netif->hostname = "lwip";
#endif /* LWIP_NETIF_HOSTNAME */

  netif->name[0] = IFNAME0;
  netif->name[1] = IFNAME1;
  /* We directly use etharp_output() here to save a function call.
   * You can instead declare your own function an call etharp_output()
   * from it if you have to do some checks before sending (e.g. if link
   * is available...) */

#if LWIP_IPV4
#if LWIP_ARP || LWIP_ETHERNET
#if LWIP_ARP
  netif->output = etharp_output;
#else
  /* The user should write ist own code in low_level_output_arp_off function */
  netif->output = low_level_output_arp_off;
#endif /* LWIP_ARP */
#endif /* LWIP_ARP || LWIP_ETHERNET */
#endif /* LWIP_IPV4 */
 
#if LWIP_IPV6
  netif->output_ip6 = ethip6_output;
#endif /* LWIP_IPV6 */

  netif->linkoutput = low_level_output;

  /* initialize the hardware */
  low_level_init(netif);

  return ERR_OK;
}



/**
* @brief  Returns the current time in milliseconds
*         when LWIP_TIMERS == 1 and NO_SYS == 1
* @param  None
* @retval Time
*/
u32_t sys_jiffies(void)
{
  return HAL_GetTick();
}

/**
* @brief  Returns the current time in milliseconds
*         when LWIP_TIMERS == 1 and NO_SYS == 1
* @param  None
* @retval Time
*/
u32_t sys_now(void)
{
  return HAL_GetTick();
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/


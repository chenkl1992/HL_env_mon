/**
 ******************************************************************************
  * File Name          : LWIP.c
  * Description        : This file provides initialization code for LWIP
  *                      middleWare.
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
#include "lwip.h"
#include "lwip/init.h"
#include "lwip/netif.h"
#include <assert.h>
#include "sds.h"

	  
lwip_dev_t lwip_dev = {
 .flag = 0,
 .remoteip[0] = 192,
 .remoteip[1] = 168,
 .remoteip[2] = 10,
 .remoteip[3] = 10,
 .remoteport = 6000,
 .ip[0] = 192,
 .ip[1] = 168,
 .ip[2] = 10,
 .ip[3] = 53,
 .netmask[0] = 255,
 .netmask[1] = 255,
 .netmask[2] = 255,
 .netmask[3] = 0,
 .gateway[0] = 192,
 .gateway[1] = 168,
 .gateway[2] = 1,
 .gateway[3] = 1,
};

struct netif gnetif;
ip4_addr_t ipaddr;
ip4_addr_t netmask;
ip4_addr_t gw;

/* USER CODE BEGIN 2 */
static void lwipdev_edit(void)
{
	SDS_Edit(&lwip_dev);
}

static void lwipdev_save(void)
{
	SDS_Save(&lwip_dev);
}

void set_lwip_remote_para(uint8_t* ip , uint16_t port)
{
	SYS_ARCH_DECL_PROTECT(lev);
	SYS_ARCH_PROTECT(lev);
	lwipdev_edit();
	lwip_dev.remoteip[0] = ip[0];	
	lwip_dev.remoteip[1] = ip[1];
	lwip_dev.remoteip[2] = ip[2];
	lwip_dev.remoteip[3] = ip[3];
	lwip_dev.remoteport = port;
	lwipdev_save();
	SYS_ARCH_UNPROTECT(lev);
}

void set_lwip_local_para(uint8_t* ip , uint8_t* netmask , uint8_t* gateway)
{
	
	SYS_ARCH_DECL_PROTECT(lev);
	SYS_ARCH_PROTECT(lev);
	
	lwipdev_edit();
	lwip_dev.ip[0] = ip[0];	
	lwip_dev.ip[1] = ip[1];
	lwip_dev.ip[2] = ip[2];
	lwip_dev.ip[3] = ip[3];	

	lwip_dev.netmask[0] = netmask[0];	
	lwip_dev.netmask[1] = netmask[1];
	lwip_dev.netmask[2] = netmask[2];
	lwip_dev.netmask[3] = netmask[3];	

	lwip_dev.gateway[0] = gateway[0];	
	lwip_dev.gateway[1] = gateway[1];
	lwip_dev.gateway[2] = gateway[2];
	lwip_dev.gateway[3] = gateway[3];
	lwipdev_save();
	SYS_ARCH_UNPROTECT(lev);
}

void get_lwip_dev(lwip_dev_t* lwipx)
{
	SYS_ARCH_DECL_PROTECT(lev);
	SYS_ARCH_PROTECT(lev);	
	*lwipx = lwip_dev;
	SYS_ARCH_UNPROTECT(lev);
}

/* USER CODE END 2 */

/**
  * LwIP initialization function
  */
void lwip_init_l(void)
{	
	/* 读取IP地址，如果没有，则使用默认ip地址 */
	lwip_dev_t ip_dev = {0};
	get_lwip_dev(&ip_dev);
  	/* Initilialize the LwIP stack with RTOS */
  	tcpip_init( NULL, NULL );


  	IP4_ADDR(&ipaddr,ip_dev.ip[0],ip_dev.ip[1],ip_dev.ip[2],ip_dev.ip[3]);
  	IP4_ADDR(&netmask,ip_dev.netmask[0],ip_dev.netmask[1] ,ip_dev.netmask[2],ip_dev.netmask[3]);
  	IP4_ADDR(&gw,ip_dev.gateway[0],ip_dev.gateway[1],ip_dev.gateway[2],ip_dev.gateway[3]);

  	/* add the network interface (IPv4/IPv6) with RTOS */
  	struct netif * stat = netif_add(&gnetif, &ipaddr, &netmask, &gw, NULL, &ethernetif_init, &tcpip_input);

  	if(stat != NULL)
	{
    	/* Registers the default network interface */
		netif_set_default(&gnetif);	
		netif_set_up(&gnetif);
  	}
}
/**
  ******************************************************************************
  * File Name          : LWIP.h
  * Description        : This file provides code for the configuration
  *                      of the LWIP.
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
  *************************************************************************  

  */
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __mx_lwip_H
#define __mx_lwip_H
#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "lwip/opt.h"
#include "lwip/mem.h"
#include "lwip/memp.h"
#include "netif/etharp.h"
#include "lwip/dhcp.h"
#include "lwip/netif.h"
#include "lwip/timeouts.h"
#include "ethernetif.h"
#include "stm32f1xx_hal.h"
#include "lwipopts.h"

/* Includes for RTOS ---------------------------------------------------------*/
#if !NO_SYS
#include "lwip/tcpip.h"
#endif /* WITH_RTOS */

typedef struct  
{
        uint8_t         flag;
	uint8_t 	remoteip[4];
	uint16_t 	remoteport;
	uint8_t 	ip[4];
	uint8_t 	netmask[4];
	uint8_t 	gateway[4];
}lwip_dev_t;

extern lwip_dev_t lwip_dev;
extern struct netif gnetif;

void set_lwip_remote_para(uint8_t* ip , uint16_t port);
void set_lwip_local_para(uint8_t* ip , uint8_t* netmask , uint8_t* gateway);
void get_lwip_dev(lwip_dev_t* lwipx);
	
void lwip_init_l(void);


#ifdef __cplusplus
}
#endif
#endif /*__ mx_lwip_H */

/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

#ifndef __SFLASH_PORT_H__
#define __SFLASH_PORT_H__

#include "sFlash.h"
#include "stm32f1xx_hal.h"

typedef struct
{
	GPIO_TypeDef   *port;
	uint32_t        pin;
} sFlash_Pin_t;

typedef struct
{
	sFlash_Pin_t   CS;
	sFlash_Pin_t   CLK;
	sFlash_Pin_t   MOSI;
	sFlash_Pin_t   MISO;
	sFlash_Pin_t   WP;
} sFlash_PinInfo_t;


#endif /* __SFLASH_PORT_H__ */

#ifndef _DM9000AEP_H
#define _DM9000AEP_H

#include "lwip/pbuf.h"


#define DM9000_RST		PBout(12)			//DM9000复位引脚
#define DM9000_INT		PBin(13)			//DM9000中断引脚



/***************************************************************************************
2^26 =0X0400 0000 = 64MB,每个 BANK 有4*64MB = 256MB
64MB:FSMC_Bank1_NORSRAM1:0x6000 0000 ~ 0x63FF FFFF
64MB:FSMC_Bank1_NORSRAM2:0x6400 0000 ~ 0x67FF FFFF
64MB:FSMC_Bank1_NORSRAM3:0x6800 0000 ~ 0x6BFF FFFF
64MB:FSMC_Bank1_NORSRAM4:0x6C00 0000 ~ 0x6FFF FFFF

选择BANK1-BORSRAM1 连接 DM9000，地址范围为0x6000 0000 ~ 0x63FF FFFF
FSMC_A16 接DM9000的CMD(寄存器/数据选择)脚
寄存器基地址 = 0x60000000
RAM基地址 = 0x60020000 = 0x60000000 + (1 << 17) = 0x60000000 + 0x20000 = 0x60020000
当选择不同的地址线时，地址要重新计算  
****************************************************************************************/
		    

#define DM9000_DAT 		(*((volatile uint16_t  *) (0x60020000|0x000000FE)))
#define DM9000_CMD 		(*((volatile uint16_t  *) (0x60000000|0x000000FE)))


#define DM9000_ID			0X90000A46	//DM9000 ID
#define DM9000_PKT_MAX		1536		//DM9000最大接收包长度

#define DM9000_PHY			0x40		//DM9000 PHY寄存器访问标志
//DM9000寄存器
#define DM9000_NCR			0x00
#define DM9000_NSR			0x01
#define DM9000_TCR			0x02
#define DM9000_TSRI			0x03
#define DM9000_TSRII		0x04
#define DM9000_RCR			0x05
#define DM9000_RSR			0x06
#define DM9000_ROCR			0x07	
#define DM9000_BPTR			0x08
#define DM9000_FCTR			0x09
#define DM9000_FCR			0x0A
#define DM9000_EPCR			0x0B
#define DM9000_EPAR			0x0C
#define DM9000_EPDRL		0x0D	
#define DM9000_EPDRH		0x0E
#define DM9000_WCR			0x0F
#define DM9000_PAR			0x10		//物理地址0X10~0X15
#define DM9000_MAR			0x16		//多播地址0X16~0X1D
#define DM9000_GPCR			0x1E
#define DM9000_GPR			0x1F
#define DM9000_TRPAL		0x22
#define DM9000_TRPAH		0x23
#define DM9000_RWPAL		0x24
#define DM9000_RWPAH		0x25

#define DM9000_VIDL			0x28
#define DM9000_VIDH			0x29
#define DM9000_PIDL			0x2A
#define DM9000_PIDH			0x2B

#define DM9000_CHIPR		0x2C
#define DM9000_TCR2			0x2D
#define DM9000_OCR			0x2E
#define DM9000_SMCR			0x2F
#define DM9000_ETXCSR		0x30
#define DM9000_TCSCR		0x31
#define DM9000_RCSCSR		0x32
#define DM9000_MRCMDX		0xF0
#define DM9000_MRCMDX1		0xF1
#define DM9000_MRCMD		0xF2
#define DM9000_MRRL			0xF4
#define DM9000_MRRH			0xF5
#define DM9000_MWCMDX		0xF6
#define DM9000_MWCMD		0xF8
#define DM9000_MWRL			0xFA
#define DM9000_MWRH			0xFB
#define DM9000_TXPLL		0xFC
#define DM9000_TXPLH		0xFD
#define DM9000_ISR			0xFE
#define DM9000_IMR			0xFF

#define NCR_RST             0x01
#define NSR_SPEED           0x80
#define NSR_LINKST         	0x40
#define NSR_WAKEST          0x20
#define NSR_TX2END          0x08
#define NSR_TX1END          0x04
#define NSR_RXOV            0x02

#define RCR_DIS_LONG        0x20
#define RCR_DIS_CRC         0x10
#define RCR_ALL             0x08
#define RCR_RXEN            0x01

#define IMR_PAR             0x80
#define IMR_LNK				0x20
#define IMR_ROOI            0x08	
#define IMR_POI             0x04		//使能接收溢出中断
#define IMR_PTI             0x02		//使能发送中断
#define IMR_PRI             0x01		//使能接收中断

#define ISR_LNKCHGS         (1<<5)
#define ISR_ROOS            (1<<3)
#define ISR_ROS             (1<<2)
#define ISR_PTS             (1<<1)
#define ISR_PRS             (1<<0)
#define ISR_CLR_STATUS      (ISR_ROOS | ISR_ROS | ISR_PTS | ISR_PRS)

//DM9000内部PHY寄存器
#define DM9000_PHY_BMCR		0x00
#define DM9000_PHY_BMSR		0x01
#define DM9000_PHY_PHYID1	0x02
#define DM9000_PHY_PHYID2	0x03
#define DM9000_PHY_ANAR		0x04
#define DM9000_PHY_ANLPAR	0x05
#define DM9000_PHY_ANER		0x06
#define DM9000_PHY_DSCR		0x10
#define DM9000_PHY_DSCSR	0x11
#define DM9000_PHY_10BTCSR	0x12
#define DM9000_PHY_PWDOR	0x13
#define DM9000_PHY_SCR		0x14

//DM9000工作模式定义
typedef enum
{
	DM9000_10MHD 	= 	0, 					//10M半双工
	DM9000_100MHD 	= 	1,					//100M半双工	
	DM9000_10MFD 	= 	4, 					//10M全双工
	DM9000_100MFD 	= 	5,					//100M全双工
	DM9000_AUTO  	= 	8, 					//自动协商
}DM9000_MODE_t;

//DM9000配置结构体
typedef struct
{
	DM9000_MODE_t  	mode;								//工作模式
	uint8_t   		imr_all;							//中断类型 
	uint16_t  		queue_packet_len;					//每个数据包大小
	uint8_t   		mac_addr[6];						//MAC地址
	uint8_t   		multicase_addr[8];					//组播地址
}dm9000_config_t;

extern dm9000_config_t dm9000cfg;		//dm9000配置结构体


uint8_t   	DM9000_Init(void);
uint16_t  	DM9000_ReadReg(uint16_t reg);
void 		DM9000_WriteReg(uint16_t reg,uint16_t data);
uint16_t  	DM9000_PHY_ReadReg(uint16_t reg);
void 		DM9000_PHY_WriteReg(uint16_t reg,uint16_t data);
uint32_t  	DM9000_Get_DeiviceID(void);
uint8_t   	DM9000_Get_SpeedAndDuplex(void);	
void 		DM9000_Set_PHYMode(uint8_t mode);
void 		DM9000_Set_MACAddress(uint8_t *macaddr);
void 		DM9000_Set_Multicast(uint8_t *multicastaddr);
void 		DM9000_Reset(void);
void 		DM9000_SendPacket(struct pbuf *p);
struct pbuf *DM9000_Receive_Packet(void);
void 		DMA9000_ISRHandler(void);
#endif
#include "dm9000.h"
#include "stm32f1xx_hal.h"
#include "delay.h"
#include "BitBand.h"
#include "os.h"
#include <assert.h>
#include "lwip.h"
#include  "data_service.h"
#include  "tcp_service.h"
#include  "log.h"

static SRAM_HandleTypeDef hsram1;

dm9000_config_t dm9000cfg;				//DM9000配置结构体
extern OS_MUTEX  dm9000Mutex;
extern OS_SEM  	LwipSyncSem;
static uint32_t FSMC_Initialized = 0;


static void MutexPend(void)
{
	OS_ERR err;
	OSMutexPend (&dm9000Mutex,0,OS_OPT_PEND_BLOCKING,NULL,&err)	;
}

static void MutexPost(void)
{
	OS_ERR err;
	OSMutexPost (&dm9000Mutex,OS_OPT_POST_NONE,&err);
}

void HAL_SRAM_MspInit(SRAM_HandleTypeDef* hsram)
{
	GPIO_InitTypeDef GPIO_InitStruct;
	if (FSMC_Initialized) 
	return;

	FSMC_Initialized = 1;
	/* Peripheral clock enable */
	__HAL_RCC_FSMC_CLK_ENABLE();
	__HAL_RCC_GPIOB_CLK_ENABLE();
	__HAL_RCC_GPIOE_CLK_ENABLE();
	__HAL_RCC_GPIOD_CLK_ENABLE();
	/** FSMC GPIO Configuration  
	PE7   ------> FSMC_D4
	PE8   ------> FSMC_D5
	PE9   ------> FSMC_D6
	PE10   ------> FSMC_D7
	PE11   ------> FSMC_D8
	PE12   ------> FSMC_D9
	PE13   ------> FSMC_D10
	PE14   ------> FSMC_D11
	PE15   ------> FSMC_D12
	PD8   ------> FSMC_D13
	PD9   ------> FSMC_D14
	PD10   ------> FSMC_D15
	PD11   ------> FSMC_A16
	PD14   ------> FSMC_D0
	PD15   ------> FSMC_D1
	PD0   ------> FSMC_D2
	PD1   ------> FSMC_D3
	PD4   ------> FSMC_NOE
	PD5   ------> FSMC_NWE
	PD7   ------> FSMC_NE1
	*/
	GPIO_InitStruct.Pin = GPIO_PIN_7|GPIO_PIN_8|GPIO_PIN_9|GPIO_PIN_10 
						  |GPIO_PIN_11|GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14 
						  |GPIO_PIN_15;
	GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

	GPIO_InitStruct.Pin = GPIO_PIN_8|GPIO_PIN_9|GPIO_PIN_10|GPIO_PIN_11 
						  |GPIO_PIN_14|GPIO_PIN_15|GPIO_PIN_0|GPIO_PIN_1 
						  |GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_7;
	GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);
	
	/* 
	PB12   ------> RST
	*/
	GPIO_InitStruct.Pin = GPIO_PIN_12; 
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

	/*
	PB13   ------> INT
	*/	
	GPIO_InitStruct.Pin = GPIO_PIN_13; 
	GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
	GPIO_InitStruct.Pull = GPIO_PULLUP;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
	
	HAL_NVIC_SetPriority(EXTI15_10_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
}

//初始化DM9000
//返回值:
//0,初始化成功
//1,DM9000A ID读取错误
uint8_t DM9000_Init(void)
{
	__HAL_RCC_AFIO_CLK_ENABLE();
	FSMC_NORSRAM_TimingTypeDef Timing;

	/** Perform the SRAM1 memory initialization sequence
	*/
	hsram1.Instance = FSMC_NORSRAM_DEVICE;
	hsram1.Extended = FSMC_NORSRAM_EXTENDED_DEVICE;
	/* hsram1.Init */
	hsram1.Init.NSBank = FSMC_NORSRAM_BANK1;
	hsram1.Init.DataAddressMux = FSMC_DATA_ADDRESS_MUX_DISABLE;
	hsram1.Init.MemoryType = FSMC_MEMORY_TYPE_SRAM;
	hsram1.Init.MemoryDataWidth = FSMC_NORSRAM_MEM_BUS_WIDTH_16;
	hsram1.Init.BurstAccessMode = FSMC_BURST_ACCESS_MODE_DISABLE;
	hsram1.Init.WaitSignalPolarity = FSMC_WAIT_SIGNAL_POLARITY_LOW;
	hsram1.Init.WrapMode = FSMC_WRAP_MODE_DISABLE;
	hsram1.Init.WaitSignalActive = FSMC_WAIT_TIMING_BEFORE_WS;
	hsram1.Init.WriteOperation = FSMC_WRITE_OPERATION_ENABLE;
	hsram1.Init.WaitSignal = FSMC_WAIT_SIGNAL_DISABLE;
	hsram1.Init.ExtendedMode = FSMC_EXTENDED_MODE_DISABLE;
	hsram1.Init.AsynchronousWait = FSMC_ASYNCHRONOUS_WAIT_DISABLE;
	hsram1.Init.WriteBurst = FSMC_WRITE_BURST_DISABLE;
	/* Timing */
	Timing.AddressSetupTime = 0;
	Timing.AddressHoldTime = 0;
	Timing.DataSetupTime = 3;
	Timing.BusTurnAroundDuration = 0;
	Timing.CLKDivision = 0;
	Timing.DataLatency = 0;
	Timing.AccessMode = FSMC_ACCESS_MODE_A;
	/* ExtTiming */

	if (HAL_SRAM_Init(&hsram1, &Timing, NULL) != HAL_OK)
	{
		assert(0);
	}

	/** Disconnect NADV
	*/
	__HAL_AFIO_FSMCNADV_DISCONNECTED();
		
	dm9000cfg.mode = DM9000_AUTO;	
 	dm9000cfg.queue_packet_len = 0;
	//DM9000的SRAM的发送和接收指针自动返回到开始地址，并且开启接收中断
	dm9000cfg.imr_all = IMR_PAR | IMR_PRI | IMR_LNK; 
	
	//初始化MAC地址
	uint32_t sn = *(uint32_t*)(0x1FFFF7E8);
	dm9000cfg.mac_addr[0] = 2;
	dm9000cfg.mac_addr[1] = 0;
	dm9000cfg.mac_addr[2] = 0;
	dm9000cfg.mac_addr[3] = (sn>>16)&0xFF;
	dm9000cfg.mac_addr[4] = (sn>>8)&0xFFF;
	dm9000cfg.mac_addr[5] = sn&0xFF;
	//初始化组播地址
	dm9000cfg.multicase_addr[0]=0xFF;
	dm9000cfg.multicase_addr[1]=0xFF;
	dm9000cfg.multicase_addr[2]=0xFF;
	dm9000cfg.multicase_addr[3]=0xFF;
	dm9000cfg.multicase_addr[4]=0xFF;
	dm9000cfg.multicase_addr[5]=0xFF;
	dm9000cfg.multicase_addr[6]=0xFF;
	dm9000cfg.multicase_addr[7]=0xFF; 
	
	DM9000_Reset();							//复位DM9000
	
	delay_ms(100);
	uint32_t temp = DM9000_Get_DeiviceID();			//获取DM9000ID  0X90000A46
	loginf("DM9000 ID:%x\r\n",temp);
	if(temp!=DM9000_ID) return 1; 			//读取ID错误
	DM9000_Set_PHYMode(dm9000cfg.mode);		//设置PHY工作模式
	
	DM9000_WriteReg(DM9000_NCR,0x00);
	DM9000_WriteReg(DM9000_TCR,0x00);		//发送控制寄存器清零
	DM9000_WriteReg(DM9000_BPTR,0x3F);	
	DM9000_WriteReg(DM9000_FCTR,0x38);
	DM9000_WriteReg(DM9000_FCR,0x00);
	DM9000_WriteReg(DM9000_SMCR,0x00);		//特殊模式
	DM9000_WriteReg(DM9000_NSR,NSR_WAKEST|NSR_TX2END|NSR_TX1END);//清除发送状态
	DM9000_WriteReg(DM9000_ISR,0x3F);		//清除中断状态
	DM9000_WriteReg(DM9000_TCR2,0x80);		//切换LED到mode1 	
	//设置MAC地址和组播地址
	DM9000_Set_MACAddress(dm9000cfg.mac_addr);		//设置MAC地址
	DM9000_Set_Multicast(dm9000cfg.multicase_addr);	//设置组播地址
	DM9000_WriteReg(DM9000_RCR,RCR_DIS_LONG|RCR_DIS_CRC|RCR_RXEN);
	DM9000_WriteReg(DM9000_IMR,IMR_PAR); 
	
	if(DM9000_Get_SpeedAndDuplex() != 0xFF)
        {//连接成功，通过串口显示连接速度和双工状态
          loginf("DM9000 Speed:%dMbps,Duplex:%s duplex mode\r\n",(temp&0x02)?10:100,(temp&0x01)?"Full":"Half");
        }
	else 
        {
          logwar("DM9000 Establish Link Failed!\r\n");
        }
	
	DM9000_WriteReg(DM9000_IMR,dm9000cfg.imr_all);	//设置中断
	
	return 0;		
} 



//读取DM9000指定寄存器的值
//reg:寄存器地址
//返回值：DM9000指定寄存器的值
uint16_t DM9000_ReadReg(uint16_t reg)
{
	delay_us(20);
	DM9000_CMD = reg;
	delay_us(20);
	return DM9000_DAT;
}
//向DM9000指定寄存器中写入指定值
//reg:要写入的寄存器
//data:要写入的值
void DM9000_WriteReg(uint16_t reg,uint16_t data)
{
	delay_us(20);
	DM9000_CMD = reg;

	delay_us(20);
	DM9000_DAT = data;
	delay_us(20);	
}
//读取DM9000的PHY的指定寄存器
//reg:要读的PHY寄存器
//返回值:读取到的PHY寄存器值
uint16_t DM9000_PHY_ReadReg(uint16_t reg)
{
	uint16_t temp;
	DM9000_WriteReg(DM9000_EPAR , DM9000_PHY | reg);
	DM9000_WriteReg(DM9000_EPCR , 0x0C);				//选中PHY，发送读命令
	delay_ms(10);
	DM9000_WriteReg(DM9000_EPCR , 0x00);				//清除读命令
	temp=(DM9000_ReadReg(DM9000_EPDRH)<<8)|(DM9000_ReadReg(DM9000_EPDRL));
	return temp;
}
//向DM9000的PHY寄存器写入指定值
//reg:PHY寄存器
//data:要写入的值
void DM9000_PHY_WriteReg(uint16_t reg,uint16_t data)
{
	DM9000_WriteReg(DM9000_EPAR , DM9000_PHY | reg);
	DM9000_WriteReg(DM9000_EPDRL, (data&0xFF));		//写入低字节
	DM9000_WriteReg(DM9000_EPDRH,((data>>8)&0xFF));	//写入高字节
	DM9000_WriteReg(DM9000_EPCR,0x0A);				//选中PHY,发送写命令
	delay_ms(50);
	DM9000_WriteReg(DM9000_EPCR,0x00);				//清除写命令	
} 
//获取DM9000的芯片ID
//返回值：DM9000的芯片ID值
uint32_t DM9000_Get_DeiviceID(void)
{
	uint32_t value;
	value = DM9000_ReadReg(DM9000_VIDL);  //0x28
	value |= DM9000_ReadReg(DM9000_VIDH) << 8;
	value |= DM9000_ReadReg(DM9000_PIDL) << 16;
	value |= DM9000_ReadReg(DM9000_PIDH) << 24;
	return value;
}

//获取DM9000的连接速度和双工模式
//返回值：	0,100M半双工
//			1,100M全双工
//			2,10M半双工
//			3,10M全双工
//			0xFF,连接失败！
uint8_t DM9000_Get_SpeedAndDuplex(void)
{
	uint8_t temp;
	uint8_t i=0;	
	if(dm9000cfg.mode == DM9000_AUTO)					//如果开启了自动协商模式一定要等待协商完成
	{
		while(!(DM9000_PHY_ReadReg(0x01)&0x0020))	//等待自动协商完成 超时时间5s
		{
			delay_ms(100);					
			i++;
			if(i>50)
			{
				logwar("DM9000 Auto-Negotiation Fail! Can Not Get Speed And Duplex!\r\n");
				return 0xFF;					//自动协商失败
			}
		}	
	}
	while(!(DM9000_ReadReg(DM9000_NSR)&0x40))	//等待连接成功 超时时间5s
	{
		delay_ms(100);
		i++;
		if(i>50)
		{
			logwar("DM9000 Link Fail!\r\n");
			return 0xFF;//连接失败
		}								
	}
	
	temp =((DM9000_ReadReg(DM9000_NSR)>>6)&0x02);	//获取DM9000的连接速度
	temp|=((DM9000_ReadReg(DM9000_NCR)>>3)&0x01);	//获取DM9000的双工状态
	return temp;
}

//设置DM9000的PHY工作模式
//mode:PHY模式
void DM9000_Set_PHYMode(uint8_t mode)
{
	uint16_t BMCR_Value,ANAR_Value;	
	switch(mode)
	{
		case DM9000_10MHD:		//10M半双工
			BMCR_Value=0x0000;
			ANAR_Value=0x21;
			break;
		case DM9000_10MFD:		//10M全双工
			BMCR_Value=0x0100;
			ANAR_Value=0x41;
			break;
		case DM9000_100MHD:		//100M半双工
			BMCR_Value=0x2000;
			ANAR_Value=0x81;
			break;
		case DM9000_100MFD:		//100M全双工
			BMCR_Value=0x2100;
			ANAR_Value=0x101;
			break;
		case DM9000_AUTO:		//自动协商模式
			BMCR_Value=0x1000;
			ANAR_Value=0x01E1;
			break;		
	}
	DM9000_PHY_WriteReg(DM9000_PHY_BMCR,BMCR_Value);
	DM9000_PHY_WriteReg(DM9000_PHY_ANAR,ANAR_Value);
 	DM9000_WriteReg(DM9000_GPR,0x00);	//使能PHY
}
//设置DM9000的MAC地址
//macaddr:指向MAC地址
void DM9000_Set_MACAddress(uint8_t *macaddr)
{
	uint8_t i;
	for(i=0;i<6;i++)
	{
		DM9000_WriteReg(DM9000_PAR+i,macaddr[i]);
	}
}
//设置DM9000的组播地址
//multicastaddr:指向多播地址
void DM9000_Set_Multicast(uint8_t *multicastaddr)
{
	uint8_t i;
	for(i=0;i<8;i++)
	{
		DM9000_WriteReg(DM9000_MAR+i,multicastaddr[i]);
	}
}  
//复位DM9000
void DM9000_Reset(void)
{
	//复位DM9000,复位步骤参考<DM9000 Application Notes V1.22>手册29页
	DM9000_RST = 0;								//DM9000硬件复位
	delay_ms(10);
	DM9000_RST = 1; 							//DM9000硬件复位结束
	delay_ms(100);								//一定要有这个延时，让DM9000准备就绪！
 	DM9000_WriteReg(DM9000_GPCR,0x01);			//第一步:设置GPCR寄存器(0X1E)的bit0为1 
	DM9000_WriteReg(DM9000_GPR,0);				//第二步:设置GPR寄存器(0X1F)的bit1为0，DM9000内部的PHY上电
 	DM9000_WriteReg(DM9000_NCR,(0x02|NCR_RST));	//第三步:软件复位DM9000 
	do 
	{
		delay_ms(25); 	
	}while(DM9000_ReadReg(DM9000_NCR)&1);		//等待DM9000软复位完成
	DM9000_WriteReg(DM9000_NCR,0);
	DM9000_WriteReg(DM9000_NCR,(0x02|NCR_RST));	//DM9000第二次软复位
	do 
	{
		delay_ms(25);	
	}while (DM9000_ReadReg(DM9000_NCR)&1);
}  

//通过DM9000发送数据包
//p:pbuf结构体指针
void DM9000_SendPacket(struct pbuf *p)
{
	struct pbuf *qbuf;
	uint16_t pbuf_index = 0;
	uint8_t word[2], word_index = 0;
 	//printf("send len:%d\r\n",p->tot_len);
	MutexPend();			//请求互斥信号量,锁定DM9000 
	DM9000_WriteReg(DM9000_IMR,IMR_PAR);		//关闭网卡中断 
	DM9000_CMD = DM9000_MWCMD;					//发送此命令后就可以将要发送的数据搬到DM9000 TX SRAM中	
	qbuf = p;
	//向DM9000的TX SRAM中写入数据，一次写入两个字节数据
	//当要发送的数据长度为奇数的时候，我们需要将最后一个字节单独写入DM9000的TX SRAM中
 	while(qbuf)
	{
		if (pbuf_index < qbuf->len)
		{
			word[word_index++] = ((uint8_t*)qbuf->payload)[pbuf_index++];
			if (word_index == 2)
			{
				DM9000_DAT = ((uint16_t)word[1]<<8)|word[0];
				word_index = 0;
			}
		}
		else
		{
			qbuf = qbuf->next;
			pbuf_index = 0;
		}
	}
	//还有一个字节未写入TX SRAM
	if(word_index==1)DM9000_DAT = word[0];
	//向DM9000写入发送长度
	DM9000_WriteReg(DM9000_TXPLL,p->tot_len&0xFF);
	DM9000_WriteReg(DM9000_TXPLH,(p->tot_len>>8)&0xFF);		//设置要发送数据的数据长度
	DM9000_WriteReg(DM9000_TCR,0x01);						//启动发送 
	while((DM9000_ReadReg(DM9000_NSR)&0x0C)==0);			//等待发送完成 
	DM9000_WriteReg(DM9000_NSR,0x0C);						//清除状态寄存器，由于发送数据没有设置中断，因此不必处理中断标志位
 	DM9000_WriteReg(DM9000_IMR,dm9000cfg.imr_all);			//DM9000网卡接收中断使能
	MutexPost();											//发送互斥信号量,解锁DM9000
}
/*
	DM9000接收数据包
	接收到的数据包存放在DM9000的RX FIFO中，地址为0x0C00~0x3FFF
	接收到的数据包的前四个字节并不是真实的数据，而是有特定含义的
	byte1:表明是否接收到数据，为0x00或者0x01，如果两个都不是的话一定要软件复位DM9000
			0x01，接收到数据
			0x00，未接收到数据
	byte2:第二个字节表示一些状态信息，和DM9000的RSR(0x06)寄存器一致的
	byte3:本帧数据长度的低字节
	byte4:本帧数据长度的高字节
	返回值：pbuf格式的接收到的数据包*/
struct pbuf *DM9000_Receive_Packet(void)
{
	struct pbuf* p_buf = NULL;
	struct pbuf* pp_buf = NULL;
    uint32_t rxbyte = 0;
	volatile uint16_t rx_status, rx_length;
    uint16_t* data = NULL;
	uint16_t dummy = 0; 
	int len;

	MutexPend(); 				//请求互斥信号量,锁定DM9000
error_retry:	
	DM9000_ReadReg(DM9000_MRCMDX);					//假读
	rxbyte=(uint8_t)DM9000_DAT;						//进行第二次读取 
	if(rxbyte)										//接收到数据
	{
		if(rxbyte>1)								//rxbyte大于1，接收到的数据错误,挂了
		{
            logerr("dm9000 rx: rx error, stop device\r\n");
			DM9000_WriteReg(DM9000_RCR,0x00);
			DM9000_WriteReg(DM9000_ISR,0x80);
			MutexPost();
			return p_buf;
		}
		DM9000_CMD = DM9000_MRCMD;
		rx_status = DM9000_DAT;
        rx_length = DM9000_DAT;  
		//if(rx_length>512)printf("rxlen:%d\r\n",rx_length);
        p_buf = pbuf_alloc(PBUF_RAW,rx_length,PBUF_POOL);	//pbufs内存池分配pbuf
		if(p_buf != NULL)									//内存申请成功
        {
            for(pp_buf = p_buf ;pp_buf != NULL ; pp_buf = pp_buf->next)
            {
                data = (uint16_t*)pp_buf->payload;
                len = pp_buf->len;
                while( len > 0)
                {
					*data = DM9000_DAT;
                    data++;
                    len-= 2;
                }
            }
        }
		else										//内存申请失败
		{
			logerr("pbuf malloc fail !:%d\r\n",rx_length);
            data = &dummy;
			len = rx_length;
			while(len > 0)
			{
				*data = DM9000_DAT;
				len -= 2;
			}
        }	
		//根据rx_status判断接收数据是否出现如下错误：FIFO溢出、CRC错误
		//对齐错误、物理层错误，如果有任何一个出现的话丢弃该数据帧，
		//当rx_length小于64或者大于最大数据长度的时候也丢弃该数据帧
		if((rx_status & 0xBF00) || (rx_length < 0x40) || (rx_length > DM9000_PKT_MAX))
		{
			loginf("rx_status:%#x\r\n",rx_status);
			if (rx_status & 0x100)logerr("rx fifo error\r\n");
            if (rx_status & 0x200)logerr("rx crc error\r\n");
            if (rx_status & 0x8000)logerr("rx length error\r\n");
            if (rx_length > DM9000_PKT_MAX)
			{
				logwar("rx length too big\r\n");
				DM9000_WriteReg(DM9000_NCR, NCR_RST); 	//复位DM9000
				delay_ms(5);
			}
			if(p_buf!=NULL)pbuf_free((struct pbuf*)p_buf);		//释放内存
			p_buf = NULL;
			goto error_retry;
		}
	}
	else
    {
		/*
        DM9000_WriteReg(DM9000_ISR,ISR_CLR_STATUS);							//清除所有中断标志位
        //dm9000cfg.imr_all = IMR_PAR | IMR_PRI | IMR_LNK;				//重新接收中断 和 状态变化中断
        DM9000_WriteReg(DM9000_IMR, dm9000cfg.imr_all);
		*/
    } 
	MutexPost();														//发送互斥信号量,解锁DM9000
	return (struct pbuf*)p_buf; 
}


//中断处理函数
void DMA9000_ISRHandler(void)
{
	uint16_t int_status;
	uint16_t last_io; 
	last_io = DM9000_CMD;
	int_status = DM9000_ReadReg(DM9000_ISR); 
	DM9000_WriteReg(DM9000_ISR,int_status);								//清除中断标志位，DM9000的ISR寄存器的bit0~bit5写1清零
	if(int_status & ISR_ROS)
		loginf("Receive Overflow \r\n");
    if(int_status & ISR_ROOS)
		loginf("Receive Overflow Counter Overflow \r\n");	
	if(int_status & ISR_PRS)											//接收中断
	{
		OS_ERR err;
 		OSSemPost (&LwipSyncSem,OS_OPT_POST_ALL,&err);
		assert(OS_ERR_NONE == err);
	} 
//	if(int_status & ISR_PTS)											//发送中断
//	{ 
//		printf("Transmit Under-run \r\n");
//	}
	if(int_status & ISR_LNKCHGS)  										//数据链路发生变化
	{
          if( DM9000_ReadReg(DM9000_NSR) & 0x40 )
          {
            loginf("Link Status Change : Create Connect! \r\n");
          }
          else
          {
            loginf("Link Status Change : Lost Connect!\r\n");
          }
	}
	DM9000_CMD = last_io;	
}

void EXTI15_10_IRQHandler(void)
{
	CPU_SR_ALLOC();


    CPU_CRITICAL_ENTER();
    OSIntEnter();                                               /* Tell uC/OS-III that we are starting an ISR           */
    CPU_CRITICAL_EXIT();
	
	if (__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_13) != RESET)
	{
		__HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_13);
		while(DM9000_INT == 0)
		{
			DMA9000_ISRHandler();
		}
	}	
    OSIntExit();                                                /* Tell uC/OS-III that we are leaving the ISR           */	
}


































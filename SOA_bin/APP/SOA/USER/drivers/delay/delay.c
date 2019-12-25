#include "delay.h"
#include "os.h"
#include "stm32f1xx_hal.h"

#define delay_osrunning		OSRunning			//OS是否运行标记,0,不运行;1,在运行
#define delay_ostickspersec	OSCfg_TickRate_Hz	//OS时钟节拍,即每秒调度次数
#define delay_osintnesting 	OSIntNestingCtr		//中断嵌套级别,即中断嵌套次数

static uint8_t  fac_us = 72;							//us延时倍乘数			   
static uint16_t fac_ms = 1;							//ms延时倍乘数,在ucos下,代表每个节拍的ms数

static void delay_osschedlock(void)
{
	OS_ERR err; 
	OSSchedLock(&err);
}


static void delay_osschedunlock(void)
{	
	OS_ERR err; 
	OSSchedUnlock(&err);
}

static void delay_ostimedly(uint32_t ticks)
{
	OS_ERR err; 
	OSTimeDly(ticks,OS_OPT_TIME_PERIODIC,&err);
}

void delay_us(uint32_t nus)
{		
	uint32_t ticks;
	uint32_t told,tnow,tcnt=0;
	uint32_t reload = SysTick->LOAD;
	ticks= nus * fac_us;
	tcnt = 0;
	delay_osschedlock();
	told = SysTick->VAL;
	while(1)
	{
		tnow = SysTick->VAL;	
		if(tnow != told)
		{	    
			if(tnow<told) 
				tcnt += (told - tnow);					//这里注意一下SYSTICK是一个递减的计数器就可以了.
			else 
				tcnt += (reload - tnow + told);	    
			told = tnow;
			if(tcnt>=ticks)
				break;									//时间超过/等于要延迟的时间,则退出.
		}  
	};
	delay_osschedunlock();								//恢复OS调度									    
}
//延时nms
//nms:要延时的ms数
void delay_ms(uint16_t nms)
{	
	if(delay_osrunning && (delay_osintnesting == 0))	//如果OS已经在跑了,并且不是在中断里面(中断里面不能任务调度)	    
	{		 
		if(nms >= fac_ms)								//延时的时间大于OS的最少时间周期 
		{ 
   			delay_ostimedly(nms/fac_ms);		    	//OS延时
		}
		nms %= fac_ms;									//OS已经无法提供这么小的延时了,采用普通方式延时    
	}
	delay_us((uint32_t)(nms*1000));						//普通方式延时  
}
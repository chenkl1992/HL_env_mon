#ifndef __SDS_H__
#define __SDS_H__

/*******************************************************************************************************
            SDS Module(Stable Data Storage)
*******************************************************************************************************/
#ifdef __cplusplus 
extern "C" {
#endif

#include "stdint.h"
#include "string.h"
#include "SDS_port.h"

#define SDS_ERR_NONE                     0
#define SDS_ERR_PORT                     1
#define SDS_ERR_UNFOUND                  2
#define SDS_ERR_MEMORY                   3
#define SDS_ERR_CHECKSUM                 4
#define SDS_ERR_DIFF                     5
#define SDS_ERR_UNINIT                   6
#define SDS_ERR_UNEDIT                   7
#define SDS_ERR_SAVING                   8
#define SDS_ERR_LOAD_DFT                 9
#define SDS_ERR_OUT_OF_MEM               10
#define SDS_ERR_INVALID      	         11
	
#define SDS_ERR_TEST_FAILED             12


typedef struct
{
	void*        ram_addr;
	uint16_t     size;
	const void*  p_default;
} SDS_Item_t;


/*
************************************************************************************************************************
*                                                     初始化
*
* Description: 初始化 SDS 模块(自动加载所有数据)
*
* Arguments  : none
*
* Returns    : SDS_ERR_NONE         无错误
*              SDS_ERR_PORT         port 初始化失败 
*
* Note(s)    : none
************************************************************************************************************************
*/

uint8_t SDS_Init(void);


/*
************************************************************************************************************************
*                                                 获取数据有效性
*
* Description: 获取数据指定数据的有效性(判断依据:CRC)
*
* Arguments  : p_data               数据项的指针
*              p_is_valid           接收有效性的指针(0:无效, 1:有效)
*
* Returns    : SDS_ERR_NONE         无错误
*              SDS_ERR_UNFOUND      未找到数据项
*              SDS_ERR_UNINIT       未初始化
*
* Note(s)    : none
************************************************************************************************************************
*/

uint8_t SDS_IsValid(void* p_data, uint8_t* p_is_valid);


/*
************************************************************************************************************************
*                                                    开始编辑
*
* Description: 开始编辑在 SDS_table 中定义的一项数据
*
* Arguments  : p_data               需保存的数据项的指针
*
* Returns    : SDS_ERR_NONE         无错误
*              SDS_ERR_UNFOUND      未找到数据项
*              SDS_ERR_UNINIT       未初始化
*
* Note(s)    : none
************************************************************************************************************************
*/

uint8_t SDS_Edit(void* p_data);


/*
************************************************************************************************************************
*                                                    数据保存
*
* Description: 保存在 SDS_table 中定义的一项数据(需保存的数据项 必须在调用 SDS_Edit 之后, 才能执行此函数保存数据)
*
* Arguments  : p_data               需保存的数据项的指针
*
* Returns    : SDS_ERR_NONE         无错误
*              SDS_ERR_UNFOUND      未找到数据项
*              SDS_ERR_UNINIT       未初始化
*              SDS_ERR_UNEDIT       未进入编辑模式
*
* Note(s)    : 最终的数据存储操作将在 SDS_Poll 中执行
************************************************************************************************************************
*/

uint8_t SDS_Save(void* p_data);


/*
************************************************************************************************************************
*                                                     轮询
*
* Description: 扫描并存储需要存储的数据
*
* Arguments  : p_data               需保存的数据项的指针
*
* Returns    : SDS_ERR_NONE         无错误
*              SDS_ERR_PORT         port层错误
*
* Note(s)    : none
************************************************************************************************************************
*/

uint8_t SDS_Poll(void);


/*
************************************************************************************************************************
*                                                   重新加载数据
*
* Description: 加载在 SDS_table 中定义的一项数据
*
* Arguments  : p_data               需加载的数据项的指针
*
* Returns    : SDS_ERR_NONE         无错误
*              SDS_ERR_UNFOUND      未找到数据项
*              SDS_ERR_UNINIT       未初始化
*
* Note(s)    : 可能丢失未保存的数据
************************************************************************************************************************
*/

uint8_t SDS_Reload(void* p_data);


//uint8_t SDS_Test(void);



#ifdef __cplusplus
}
#endif

#endif /* __SDS_H__ */
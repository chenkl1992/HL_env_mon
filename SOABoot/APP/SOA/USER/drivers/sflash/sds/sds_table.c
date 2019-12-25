#include "sds.h"
#include "sds_table.h"

// 在此处 包含 所有需要使用 sds 模块存储数据的 变量 的头文件
lwip_dev_t  ipdev_dft = 
{	
	.remoteip[0] = 192,
	.remoteip[1] = 168,
	.remoteip[2] = 1,
	.remoteip[3] = 149,
	
	.remoteport = 6000,

	.netmask[0] = 255,
	.netmask[1] = 255,
	.netmask[2] = 255,
	.netmask[3] = 0,

	.gateway[0] = 192,
	.gateway[1] = 168,
	.gateway[2] = 1,
	.gateway[3] = 1,		

	.ip[0] = 192,
	.ip[1] = 168,
	.ip[2] = 1,
	.ip[3] = 49,
};

SysConf_t  sconfig_dft = 
{	
	.soft_version 		= 0x00010000,
	.report_interval 	= 30,
};

SysConf_t sysconfig = {0};
lwip_dev_t lwip_dev = {0};
/*当前使用14个扇区 ，共64K 16个扇区*/
const SDS_Item_t SDS_Table[] = 
{
	{&sysconfig , sizeof(SysConf_t) , &sconfig_dft},
	{&lwip_dev , sizeof(lwip_dev_t) , &ipdev_dft},
};
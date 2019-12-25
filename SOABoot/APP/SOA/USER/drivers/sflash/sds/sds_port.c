#include "SDS.h"
#include "SDS_cfg.h"
#include "string.h"
#include "stdio.h"
#include "stdlib.h"
#include "stdint.h"
#include "sFlash.h"


uint8_t  _SDS_Init(void)
{	
	return SDS_ERR_NONE;
}

uint8_t _SDS_Erase(uint32_t addr, uint16_t size)
{
	//(void)addr;
	//(void)size;
    sFlash_Erase(SDS_MEM_ADDR + addr, size);
	return SDS_ERR_NONE;
}

uint8_t _SDS_Write(void* p_buf, uint32_t addr, uint16_t size)
{
//	if (addr + size > SDS_MEM_SIZE) /* address range check, never remove this */
//		return SDS_ERR_OUT_OF_MEM;
//	uint8_t err = EEPROM_Write(SDS_MEM_ADDR + addr, p_buf, size);
//	return (err == EEPROM_ERR_NONE) ? SDS_ERR_NONE : SDS_ERR_MEMORY;
    
	if (addr + size > SDS_MEM_SIZE) /* address range check, never remove this */
		return SDS_ERR_OUT_OF_MEM;
	uint8_t err = sFlash_WriteBytes(SDS_MEM_ADDR + addr, p_buf, size);
	return (err == SFLASH_ERR_NONE) ? SDS_ERR_NONE : SDS_ERR_MEMORY;

}

uint8_t _SDS_Read(void* p_buf, uint32_t addr, uint16_t size)
{
//	if (addr + size > SDS_MEM_SIZE) /* address range check, never remove this */
//		return SDS_ERR_OUT_OF_MEM;
//	uint8_t err = EEPROM_Read(SDS_MEM_ADDR + addr, p_buf, size);
//	return (err == EEPROM_ERR_NONE) ? SDS_ERR_NONE : SDS_ERR_MEMORY;
    
	if (addr + size > SDS_MEM_SIZE) /* address range check, never remove this */
		return SDS_ERR_OUT_OF_MEM;
	uint8_t err = sFlash_ReadBytes(SDS_MEM_ADDR + addr, p_buf, size);
	return (err == SFLASH_ERR_NONE) ? SDS_ERR_NONE : SDS_ERR_MEMORY;

}

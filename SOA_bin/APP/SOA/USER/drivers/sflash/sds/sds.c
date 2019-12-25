#include "sds.h"
#include "sds_cfg.h"



extern uint8_t  _SDS_Init(void);
extern uint8_t  _SDS_Erase(uint32_t addr, uint16_t size);
extern uint8_t  _SDS_Write(void* p_buf, uint32_t addr, uint16_t size);
extern uint8_t  _SDS_Read(void* p_buf, uint32_t addr, uint16_t size);

typedef enum { UNINIT = 0, IDLE, EDIT, NEED_SAVE, SAVING , DATA_CHANGED_ERROR } Item_State_t;

static Item_State_t _Items_State[SDS_ITEMS_NB];
static uint8_t      _Items_Valid[SDS_ITEMS_NB];
static uint16_t     _Items_CRC16[SDS_ITEMS_NB];

static uint8_t temp_buf[SDS_TEMP_BUF_SIZE * 2];

static uint16_t crc16_modbus(unsigned char *arr_buff, int len)
{
    uint16_t crc = 0xFFFF;
	
    for (int j = 0; j < len ; j++)
    {
        crc = crc ^ arr_buff[j];
        for (int i = 0; i < 8; i++)
        {
            if ((crc & 0x0001) >0)
            {
                crc = crc >> 1;
                crc = crc ^ 0xa001;
            }
            else
                crc = crc >> 1;
        }
    }
    return (crc);
}

static uint8_t _cmp(uint32_t addr1, uint32_t addr2, uint16_t size)
{
	size += 2; // CRC
	
	uint8_t err;
	uint8_t len;
	for (uint16_t i = 0; i < size; i += len)
	{
		len = SDS_TEMP_BUF_SIZE;
		if (size - i < len)
		{
			len = size - i;
		}
		err = _SDS_Read(&temp_buf[0], addr1 + i, len);
		if (err != SDS_ERR_NONE)
		{
			return SDS_ERR_MEMORY;
		}
		err = _SDS_Read(&temp_buf[SDS_TEMP_BUF_SIZE], addr2 + i, len);
		if (err != SDS_ERR_NONE)
		{
			return SDS_ERR_MEMORY;
		}
		for (uint8_t k = 0; k < len; k++)
		{
			if (temp_buf[k] != temp_buf[SDS_TEMP_BUF_SIZE + k])
			{
				return SDS_ERR_DIFF;
			}
		}
	}

	return SDS_ERR_NONE;
}

static uint8_t _cpy(uint32_t addr_dest, uint32_t addr_src, uint16_t size)
{
	size += 2; // CRC
	
	uint8_t err;

#if SDS_STORAGE_ALIGN > 1
	err = _SDS_Erase(addr_dest, (size + SDS_STORAGE_ALIGN - 1) / SDS_STORAGE_ALIGN * SDS_STORAGE_ALIGN);
	if (err != SDS_ERR_NONE)
	{
		return err;
	}
#endif 

	uint8_t len;
	for (uint16_t i = 0; i < size; i += len)
	{
		len = sizeof(temp_buf);
		if (size - i < len)
		{
			len = size - i;
		}
		err = _SDS_Read(temp_buf, addr_src + i, len);
		if (err != SDS_ERR_NONE)
		{
			return err;
		}		
		err = _SDS_Write(temp_buf, addr_dest + i, len);
		if (err != SDS_ERR_NONE)
		{
			return err;
		}
	}

	return SDS_ERR_NONE;
}


static uint8_t _get_addr_and_size(void* p_data, uint32_t* p_dft, uint32_t* p_bkp, uint16_t* p_size, uint8_t* p_i, const void** pp_default)
{
	uint32_t addr = 0;
	uint32_t mod;
	uint32_t hold_size;
	for (uint8_t i = 0; i < SDS_ITEMS_NB; i++)
	{
		void* p_data_search = SDS_Table[i].ram_addr;
		if (p_data_search == p_data)
		{
			*p_i = i;
			*p_size = SDS_Table[i].size;
			*p_dft = addr;
			*pp_default = SDS_Table[i].p_default;
			hold_size = *p_size + 2; // 2 bytes checksum
			mod = hold_size % SDS_STORAGE_ALIGN;
			if (mod > 0)
			{
				hold_size += SDS_STORAGE_ALIGN - mod;
			}
			*p_bkp = addr + hold_size;
			return SDS_ERR_NONE;
		}
		hold_size = SDS_Table[i].size + 2; // 2 bytes checksum
		mod = hold_size % SDS_STORAGE_ALIGN;
		if (mod > 0)
		{
			hold_size += SDS_STORAGE_ALIGN - mod;
		}
		addr += hold_size << 1; // 2 part (default + backup)
	}

	return SDS_ERR_UNFOUND;
}

static uint8_t _read(void* p_buf, uint32_t addr, uint16_t size)
{
	uint16_t checksum_val;
	uint8_t err = _SDS_Read(p_buf, addr, size);
	if (err != SDS_ERR_NONE) return err;
	err = _SDS_Read(&checksum_val, addr + size, 2);
	if (err != SDS_ERR_NONE) return err;
	if (crc16_modbus(p_buf, size) == checksum_val)
	{
		return SDS_ERR_NONE;
	}
	else
	{
		memset(p_buf, 0xFF, size);
		return SDS_ERR_CHECKSUM;
	}
}

static uint8_t _write(void* p_buf, uint32_t addr, uint16_t size)
{
	uint16_t checksum_val = crc16_modbus(p_buf, size);
	uint8_t err;
#if SDS_STORAGE_ALIGN > 1
	err = _SDS_Erase(addr, ((size + 2 + SDS_STORAGE_ALIGN - 1) / SDS_STORAGE_ALIGN) * SDS_STORAGE_ALIGN);
	if (err != SDS_ERR_NONE) return err;
#endif 
	err = _SDS_Write(p_buf, addr, size);
	if (err != SDS_ERR_NONE) return err;
	err = _SDS_Write(&checksum_val, addr + size, 2);
	return err;
}

static uint8_t _load(void* p_data, uint32_t dft, uint32_t bkp, uint16_t size, const void* p_default)
{
	// read default
	uint8_t err = _read(p_data, dft, size);
	if (err == SDS_ERR_NONE)
	{
		if (_cmp(dft, bkp, size) != 0)
		{
			_cpy(bkp, dft, size);
		}
	}
	else
	{
		// read backup
		err = _read(p_data, bkp, size);
		if (err == SDS_ERR_NONE)
		{
			_cpy(dft, bkp, size);
		}
	}
	if (err == SDS_ERR_CHECKSUM && p_default != NULL)
	{
		memcpy(p_data, p_default, size);
		return SDS_ERR_LOAD_DFT;
	}
	return err;
}



// load all items, ignore error
uint8_t SDS_Init(void)
{
	uint8_t err = _SDS_Init();
	if (err != SDS_ERR_NONE)
		return err;

	for (uint8_t i = 0; i < SDS_ITEMS_NB; i++)
	{
		uint32_t dft;
		uint32_t bkp;
		uint16_t size;
		uint8_t  i2;
		const void* p_default;
		void* p_data = SDS_Table[i].ram_addr;
		_get_addr_and_size(p_data, &dft, &bkp, &size, &i2, &p_default);
		err = _load(p_data, dft, bkp, size, p_default);
		if (err == SDS_ERR_OUT_OF_MEM)
			return err;
		_Items_Valid[i] = (err == SDS_ERR_NONE) ? 1 : 0;
		_Items_State[i] = IDLE;
	}
	return SDS_ERR_NONE;
}

uint8_t SDS_IsValid(void* p_data, uint8_t* p_is_valid)
{
	uint32_t dft;
	uint32_t bkp;
	uint16_t size;
	uint8_t  i;
	const void* p_default;
	uint8_t err = _get_addr_and_size(p_data, &dft, &bkp, &size, &i, &p_default);
	if (err != SDS_ERR_NONE)
	{
		return err;
	}	

	if (_Items_State[i] == UNINIT)
	{	
		return SDS_ERR_UNINIT;
	}

	*p_is_valid = _Items_Valid[i];

	return SDS_ERR_NONE;
}

uint8_t SDS_Edit(void* p_data)
{
	uint32_t dft;
	uint32_t bkp;
	uint16_t size;
	uint8_t  i;
	const void* p_default;
	uint8_t err = _get_addr_and_size(p_data, &dft, &bkp, &size, &i, &p_default);
	if (err != SDS_ERR_NONE)
	{
		return err;
	}

	if (_Items_State[i] == UNINIT)
	{	
		return SDS_ERR_UNINIT;
	}

	if(_Items_State[i] == SAVING)//add
	{
		return SDS_ERR_SAVING;
	}
	
	_Items_State[i] = EDIT;

	return SDS_ERR_NONE;
}

uint8_t SDS_Save(void * p_data)
{
	uint32_t dft;
	uint32_t bkp;
	uint16_t size;
	uint8_t  i;
	const void* p_default;
	uint8_t err = _get_addr_and_size(p_data, &dft, &bkp, &size, &i, &p_default);
	if (err != SDS_ERR_NONE)
	{
		return err;
	}

	if (_Items_State[i] == UNINIT)
	{	
		return SDS_ERR_UNINIT;
	}

	if (_Items_State[i] != EDIT)
	{
		return SDS_ERR_UNEDIT;
	}
	
	_Items_CRC16[i] = crc16_modbus(p_data, size);
	_Items_State[i] = NEED_SAVE;

	return SDS_ERR_NONE;
}

uint8_t SDS_Poll(void)
{
	uint8_t err_result = SDS_ERR_NONE;
	for (uint8_t i = 0; i < SDS_ITEMS_NB; i++)
	{
		if (_Items_State[i] != NEED_SAVE)
		{
			continue;
		}
		//_Items_Valid[i] = 0;
		_Items_State[i] = SAVING;   //add
		void* p_data = SDS_Table[i].ram_addr;
		uint32_t dft;
		uint32_t bkp;
		uint16_t size;
		uint8_t  i2;
		const void* p_default;
		uint8_t err = _get_addr_and_size(p_data, &dft, &bkp, &size, &i2, &p_default);
		if (err != SDS_ERR_NONE)
		{
			err_result = err;
			continue;
		}
		// write default
		if (_Items_CRC16[i] != crc16_modbus(p_data, size))
		{
			_Items_State[i] = DATA_CHANGED_ERROR;
			continue;
		}		
		err = _write(p_data, dft, size);
		if (err != SDS_ERR_NONE)
		{
			err_result = err;
			continue;
		}
		// write backup
		if (_Items_CRC16[i] != crc16_modbus(p_data, size))
		{
			_Items_State[i] = DATA_CHANGED_ERROR;
			continue;
		}
		err = _write(p_data, bkp, size);
		if (err != SDS_ERR_NONE)
		{
			err_result = err;
			continue;
		}
		if (_Items_CRC16[i] != crc16_modbus(p_data, size))
		{
			_Items_State[i] = DATA_CHANGED_ERROR;
			continue;
		}
		_Items_Valid[i] = 1;
		_Items_State[i] = IDLE;
	}
	return err_result;
}

uint8_t SDS_Reload(void * p_data)
{
	uint32_t dft;
	uint32_t bkp;
	uint16_t size;
	uint8_t  i;
	const void*    p_default;
	uint8_t err = _get_addr_and_size(p_data, &dft, &bkp, &size, &i, &p_default);
	if (err != SDS_ERR_NONE)
	{
		return err;
	}

	if (_Items_State[i] == UNINIT)
	{	
		return SDS_ERR_UNINIT;
	}

	err = _load(p_data, dft, bkp, size, p_default);
	_Items_Valid[i] = (err == SDS_ERR_NONE) ? 1 : 0;
	if (err != SDS_ERR_NONE)
	{
		return err;
	}

	_Items_State[i] = IDLE;

	return SDS_ERR_NONE;
}


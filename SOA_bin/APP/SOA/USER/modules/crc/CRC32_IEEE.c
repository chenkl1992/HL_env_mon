#include "stdint.h"

#define QUOTIENT 0x04c11db7

uint32_t CRC32_IEEE_Calc2(const void* buf, uint32_t len, uint32_t init_val)
{
	uint32_t value = init_val;
	uint8_t* p = (uint8_t*)buf;
	for (uint32_t i = 0; i < len; i++)
	{
		uint8_t octet = *(p+i);
		for (uint8_t j = 0; j < 8; j++)
		{
			if ((octet >> 7) ^ (value >> 31))
			{
				value = (value << 1) ^ QUOTIENT;
			}
			else
			{
				value = (value << 1);
			}
			octet <<= 1;
		}
	}
	return value;
}

uint32_t CRC32_IEEE_Calc(const void* buf, uint32_t len)
{
	return CRC32_IEEE_Calc2(buf, len, UINT32_MAX);
}

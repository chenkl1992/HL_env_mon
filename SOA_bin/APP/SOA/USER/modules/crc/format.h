#ifndef __FORMAT_H__
#define __FORMAT_H__

#include "stdint.h"
#include <stdlib.h>
#include "string.h"

#define HI_UINT16(a) (((a) >> 8) & 0xFF)
#define LO_UINT16(a) ((a) & 0xFF)

uint16_t string2Hexs(char *ascs,uint8_t * hexs,uint16_t length);
uint16_t  string2Dig(char *ascs,uint8_t * Dig,uint16_t length);
void hex2String(const uint8_t *source, uint16_t sourceLen, char *dest);
uint8_t hex_to_dig(uint8_t hex);
#endif
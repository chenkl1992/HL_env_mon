#include "stdint.h"
#include <stdlib.h>
#include "string.h"
#include "format.h"

//16进制转10进制
uint8_t hex_to_dig(uint8_t hex)
{
    uint8_t d;
    d = hex/10*16 + hex%10;
    return d;
}

void hex2String(const uint8_t *source, uint16_t sourceLen, char *dest)  
{  
    short i;  
    unsigned char highByte, lowByte;  
  
    for (i = 0; i < sourceLen; i++)  
    {  
        highByte = source[i] >> 4;  
        lowByte = source[i] & 0x0f ;  
  
        highByte += 0x30;  
  
        if (highByte > 0x39)  
                dest[i * 2] = highByte + 0x07;  
        else  
                dest[i * 2] = highByte;  
  
        lowByte += 0x30;  
        if (lowByte > 0x39)  
            dest[i * 2 + 1] = lowByte + 0x07;  
        else  
            dest[i * 2 + 1] = lowByte;  
    }  
    return;  
}

uint8_t charsToDig(char *asc)
{
  uint8_t Dig=0;
  if((asc[0]>='0')&&(asc[0]<='9'))
  {
    Dig=asc[0]-0x30;
  }
  return Dig;
}


uint8_t charsToHex(char *asc)
{
  
  uint8_t hex=0;
  if((asc[0]>='0')&&(asc[0]<='9')){
    hex=asc[0]-0x30;
  }
  else if((asc[0]>='a')&&(asc[0]<='f')){
    hex=asc[0]-'a'+0xa;
  }
  else if((asc[0]>='A')&&(asc[0]<='F')){
    hex=asc[0]-'A'+0xa;
  }
  
  hex=hex<<4;
  
  if((asc[1]>='0')&&(asc[1]<='9')){
    hex+=(asc[1]-0x30);
  }
  else if((asc[1]>='a')&&(asc[1]<='f')){
    hex+=(asc[1]-'a'+0xa);
  }
  else if((asc[1]>='A')&&(asc[1]<='F')){
    hex+=(asc[1]-'A'+0xa);
  } 
  
  return hex;
}

uint16_t  string2Hexs(char *ascs,uint8_t * hexs,uint16_t length)
{
  uint16_t j=0;
  for(uint16_t i=0;i<length;i+=2){
    
    hexs[j++]=charsToHex(ascs+i);
  } 
  return j;
}

uint16_t  string2Dig(char *ascs,uint8_t * Dig,uint16_t length)
{
  uint16_t j=0;
  for(uint16_t i=0;i<length;i++){
    
    Dig[j++]=charsToDig(ascs+i);
  } 
  return j;
}



/*
	Copyright 2001, 2002 Georges Menie (www.menie.org)
	stdarg version contributed by Christian Ettinger

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

/*
	putchar is the only external dependency for this file,
	if you have a working putchar, leave it commented out.
	If not, uncomment the define below and
	replace outbyte(c) by your own function call.

*/
#include <stdarg.h>
#include "stm32f1xx_hal.h"
#include "printf-stdarg.h"
//#include "usart.h"
uint8_t mybuf[200];

#define putchar(c)    HAL_UART_Transmit(&huart4, (uint8_t *)&c, 1, 0xFFFF)
#define get_char()     HAL_UART_Receive(&huart4, (uint8_t *)&mybuf, 1, 0xFFFF)


UART_HandleTypeDef huart4;

void debug_init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct;
  
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_UART4_CLK_ENABLE();
  
  
  huart4.Instance = UART4;
  huart4.Init.BaudRate = 115200;
  huart4.Init.WordLength = UART_WORDLENGTH_8B;
  huart4.Init.StopBits = UART_STOPBITS_1;
  huart4.Init.Parity = UART_PARITY_NONE;
  huart4.Init.Mode = UART_MODE_TX_RX;
  huart4.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart4.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart4) != HAL_OK)
  {
    assert(0);
  }
  
  /**UART4 GPIO Configuration    
  PC10     ------> UART4_TX
  PC11     ------> UART4_RX 
  */
  GPIO_InitStruct.Pin = GPIO_PIN_10;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
  
  GPIO_InitStruct.Pin = GPIO_PIN_11;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
  
  __HAL_UART_ENABLE_IT(&huart4, UART_IT_RXNE);
  
  //HAL_NVIC_SetPriority(UART4_IRQn, 0, 0);
  //HAL_NVIC_EnableIRQ(UART4_IRQn);
  
}

static void printchar(char **str, int c)
{
  if (str) {
    **str = (char)c;
    ++(*str);
  }
  else
  { 
    (void)putchar(c);
  }
}

#define PAD_RIGHT 1
#define PAD_ZERO 2

static int prints(char **out, const char *string, int width, int pad)
{
  register int pc = 0, padchar = ' ';
  
  if (width > 0) {
    register int len = 0;
    register const char *ptr;
    for (ptr = string; *ptr; ++ptr) ++len;
    if (len >= width) width = 0;
    else width -= len;
    if (pad & PAD_ZERO) padchar = '0';
  }
  if (!(pad & PAD_RIGHT)) {
    for ( ; width > 0; --width) {
      printchar (out, padchar);
      ++pc;
    }
  }
  for ( ; *string ; ++string) {
    printchar (out, *string);
    ++pc;
  }
  for ( ; width > 0; --width) {
    printchar (out, padchar);
    ++pc;
  }
  
  return pc;
}

/* the following should be enough for 32 bit int */
#define PRINT_BUF_LEN 12

static int printi(char **out, int i, int b, int sg, int width, int pad, int letbase)
{
  char print_buf[PRINT_BUF_LEN];
  register char *s;
  register int t, neg = 0, pc = 0;
  register unsigned int u = (unsigned int)i;
  
  if (i == 0) {
    print_buf[0] = '0';
    print_buf[1] = '\0';
    return prints (out, print_buf, width, pad);
  }
  
  if (sg && b == 10 && i < 0) {
    neg = 1;
    u = (unsigned int)-i;
  }
  
  s = print_buf + PRINT_BUF_LEN-1;
  *s = '\0';
  
  while (u) {
    t = (unsigned int)u % b;
    if( t >= 10 )
      t += letbase - '0' - 10;
    *--s = (char)(t + '0');
    u /= b;
  }
  
  if (neg) {
    if( width && (pad & PAD_ZERO) ) {
      printchar (out, '-');
      ++pc;
      --width;
    }
    else {
      *--s = '-';
    }
  }
  
  return pc + prints (out, s, width, pad);
}

static int print( char **out, const char *format, va_list args )
{
  register int width, pad;
  register int pc = 0;
  char scr[2];
  
  for (; *format != 0; ++format) {
    if (*format == '%') {
      ++format;
      width = pad = 0;
      if (*format == '\0') break;
      if (*format == '%') goto out;
      if (*format == '-') {
        ++format;
        pad = PAD_RIGHT;
      }
      while (*format == '0') {
        ++format;
        pad |= PAD_ZERO;
      }
      for ( ; *format >= '0' && *format <= '9'; ++format) {
        width *= 10;
        width += *format - '0';
      }
      if( *format == 's' ) {
        register char *s = (char *)va_arg( args, int );
        pc += prints (out, s?s:"(null)", width, pad);
        continue;
      }
      if( *format == 'd' ) {
        pc += printi (out, va_arg( args, int ), 10, 1, width, pad, 'a');
        continue;
      }
      if( *format == 'x' ) {
        pc += printi (out, va_arg( args, int ), 16, 0, width, pad, 'a');
        continue;
      }
      if( *format == 'X' ) {
        pc += printi (out, va_arg( args, int ), 16, 0, width, pad, 'A');
        continue;
      }
      if( *format == 'u' ) {
        pc += printi (out, va_arg( args, int ), 10, 0, width, pad, 'a');
        continue;
      }
      if( *format == 'c' ) {
        /* char are converted to int then pushed on the stack */
        scr[0] = (char)va_arg( args, int );
        scr[1] = '\0';
        pc += prints (out, scr, width, pad);
        continue;
      }
    }
    else {
      out:
      printchar (out, *format);
      ++pc;
    }
  }
  if (out) **out = '\0';
  va_end( args );
  return pc;
}

int printf(const char *format, ...)
{
  va_list args;
  
  va_start( args, format );
  return print( 0, format, args );
}

int sprintf(char *out, const char *format, ...)
{
  va_list args;
  
  va_start( args, format );
  return print( &out, format, args );
}

int vsprintf( char *out, const char *fmt, va_list __vl )
{
  return print(&out, fmt, __vl);
}

int snprintf( char *buf, unsigned int count, const char *format, ... )
{
  va_list args;
  
  ( void ) count;
  
  va_start( args, format );
  return print( &buf, format, args );
}

int printk_va(char **out, const char *format, va_list args )
{
  register int width, pad;
  register int pc = 0;
  char scr[2];
  
  for (; *format != 0; ++format) {
    if (*format == '%') {
      ++format;
      width = pad = 0;
      if (*format == '\0') break;
      if (*format == '%') goto out;
      if (*format == '-') {
        ++format;
        pad = PAD_RIGHT;
      }
      while (*format == '0') {
        ++format;
        pad |= PAD_ZERO;
      }
      for ( ; *format >= '0' && *format <= '9'; ++format) {
        width *= 10;
        width += *format - '0';
      }
      if( *format == 's' ) {
        register char *s = (char *)va_arg( args, int );
        pc += prints (out, s?s:"(null)", width, pad);
        continue;
      }
      if( *format == 'd' ) {
        pc += printi (out, va_arg( args, int ), 10, 1, width, pad, 'a');
        continue;
      }
      if( *format == 'p' ) {
        pad = 8;
        pc += printi (out, va_arg( args, int ), 16, 0, width, pad, 'a');
        continue;
      }
      if( *format == 'x' ) {
        pc += printi (out, va_arg( args, int ), 16, 0, width, pad, 'a');
        continue;
      }
      if( *format == 'X' ) {
        pc += printi (out, va_arg( args, int ), 16, 0, width, pad, 'A');
        continue;
      }
      if( *format == 'u' ) {
        pc += printi (out, va_arg( args, int ), 10, 0, width, pad, 'a');
        continue;
      }
      if( *format == 'c' ) {
        /* char are converted to int then pushed on the stack */
        scr[0] = (char)va_arg( args, int );
        scr[1] = '\0';
        pc += prints (out, scr, width, pad);
        continue;
      }
    }
    else {
    out:
      printchar (out, *format);
      ++pc;
    }
  }
  if (out) **out = '\0';
  va_end( args );
  return pc;
}






#ifndef __SOFTI2C_PORT_H__
#define __SOFTI2C_PORT_H__

#include <stdint.h>


#define SOFTI2C_CH_NB                1        /* I2C 通道数                   */
#define SOFTI2C_SPEED                400      /* I2C 通信速率,单位:kbps       */
#define SOFTI2C_RETRY_TIMES_DEFAULT  3        /* I2C 默认重试次数                 */

void    _i2c_io_init(void);
void    _i2c_scl_0(uint8_t ch_index);
void    _i2c_scl_1(uint8_t ch_index);
void    _i2c_sda_0(uint8_t ch_index);
void    _i2c_sda_1(uint8_t ch_index);
uint8_t _i2c_scl_is_high(uint8_t ch_index);
uint8_t _i2c_sda_is_high(uint8_t ch_index);
void    _i2c_dly_scl(void);
void    _i2c_dly_ms(uint8_t ms);


#endif /* __SOFTI2C_PORT_H__ */

#include "stm32f1xx_hal.h"
#include "softi2c_port.h"

#define _I2C_0_SCL_PORT   GPIOB
#define _I2C_0_SDA_PORT   GPIOB


#define _I2C_0_SCL_PIN    GPIO_PIN_6
#define _I2C_0_SDA_PIN    GPIO_PIN_7


void _i2c_io_init(void)
{
	__HAL_RCC_GPIOB_CLK_ENABLE();
	
	GPIO_InitTypeDef  gpio_init;
	
    gpio_init.Pin   = _I2C_0_SCL_PIN;
    gpio_init.Mode  = GPIO_MODE_OUTPUT_OD;
    gpio_init.Pull  = GPIO_NOPULL;
    gpio_init.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(_I2C_0_SCL_PORT, &gpio_init);
	
    gpio_init.Pin   = _I2C_0_SDA_PIN;
    gpio_init.Mode  = GPIO_MODE_OUTPUT_OD;
    gpio_init.Pull  = GPIO_NOPULL;
    gpio_init.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(_I2C_0_SDA_PORT, &gpio_init);
}


void _i2c_scl_0(uint8_t ch_index)
{
	if (ch_index == 0)      HAL_GPIO_WritePin(_I2C_0_SCL_PORT, _I2C_0_SCL_PIN, GPIO_PIN_RESET);
}


void _i2c_scl_1(uint8_t ch_index)
{
	if (ch_index == 0)      HAL_GPIO_WritePin(_I2C_0_SCL_PORT, _I2C_0_SCL_PIN, GPIO_PIN_SET);
}

void _i2c_sda_0(uint8_t ch_index)
{
	if (ch_index == 0)      HAL_GPIO_WritePin(_I2C_0_SDA_PORT, _I2C_0_SDA_PIN, GPIO_PIN_RESET);
}

void _i2c_sda_1(uint8_t ch_index)
{
	if (ch_index == 0)      HAL_GPIO_WritePin(_I2C_0_SDA_PORT, _I2C_0_SDA_PIN, GPIO_PIN_SET);
}


uint8_t _i2c_scl_is_high(uint8_t ch_index)
{
	if (ch_index == 0)      return (HAL_GPIO_ReadPin(_I2C_0_SCL_PORT, _I2C_0_SCL_PIN) == GPIO_PIN_SET);
	
	else return 0;
}

uint8_t _i2c_sda_is_high(uint8_t ch_index)
{
	if (ch_index == 0)      return (HAL_GPIO_ReadPin(_I2C_0_SDA_PORT, _I2C_0_SDA_PIN) == GPIO_PIN_SET);
	
	else return 0;
}

/*
void _i2c_dly_scl(void)
{

	__IO uint32_t Delay = 2 * (SystemCoreClock / 8U / 1000000U);
	do 
	{
		__NOP();
	} 
	while (Delay --);
}



void _i2c_dly_ms(uint8_t ms)
{
	__IO uint32_t Delay = ms * (SystemCoreClock / 8U / 1000U);
	do 
	{
		__NOP();
	} 
	while (Delay --);
}
*/

void _i2c_dly_scl(void)
{
#if SOFTI2C_SPEED >= 400
	int16_t i = 5;
#elif SOFTI2C_SPEED >= 100
	int16_t i = 33;
#endif
	while (i--)
	{
		__NOP();
	}
}

void _i2c_dly_ms(uint8_t ms)
{

}


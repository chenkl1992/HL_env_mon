#include "softi2c.h"
#include "softi2c_port.h"
#include "os.h"

void softi2c_init(void)
{
	static uint8_t is_called = 0;
	if (is_called)
		return;
	is_called = 1;

	_i2c_io_init();
	
	for (uint8_t i = 0; i < SOFTI2C_CH_NB; i++)
	{
		_i2c_scl_1(i);
		_i2c_sda_1(i);
	}
}

static void _reset(uint8_t ch_index)
{
	_i2c_sda_1(ch_index);
	for (uint8_t i = 0; i < 10; i++)
	{
		_i2c_scl_0(ch_index);
		_i2c_dly_scl();
		_i2c_scl_1(ch_index);
		_i2c_dly_scl();
	}
}


static void _stop(uint8_t ch_index)
{
	_i2c_sda_0(ch_index);
	_i2c_dly_scl();
	_i2c_scl_1(ch_index);
	_i2c_dly_scl();
	_i2c_sda_1(ch_index);
	_i2c_dly_scl();
}

static uint8_t _start(uint8_t ch_index)
{
	_i2c_sda_1(ch_index);
	_i2c_scl_1(ch_index);
	_i2c_dly_scl();
	
	uint8_t retry = 10;
	while ((!_i2c_sda_is_high(ch_index) || !_i2c_scl_is_high(ch_index)) &&(--retry))
	{
		_stop(ch_index);
		_reset(ch_index);
	}
	if (retry > 0)
	{
		_i2c_sda_0(ch_index);
		_i2c_dly_scl();
		_i2c_scl_0(ch_index);
		_i2c_dly_scl();
		return 1;
	}
	else
	{
		return 0;
	}
}


static uint8_t _tx_byte(uint8_t ch_index, uint8_t b)
{	
	for (uint8_t i = 0; i < 8; i++)
	{
		if (b & 0x80)
			_i2c_sda_1(ch_index);
		else
			_i2c_sda_0(ch_index);
		b <<= 1;
		_i2c_scl_1(ch_index);
		_i2c_dly_scl();
		_i2c_scl_0(ch_index);
		_i2c_dly_scl();
	}
	_i2c_sda_1(ch_index);
	_i2c_dly_scl();
	_i2c_scl_1(ch_index);
	_i2c_dly_scl();
	uint8_t is_ack = !_i2c_sda_is_high(ch_index);
	_i2c_scl_0(ch_index);
	_i2c_dly_scl();

	return is_ack;
}


static void _ack(uint8_t ch_index)
{
	_i2c_sda_0(ch_index);
	_i2c_dly_scl();
	_i2c_scl_1(ch_index);
	_i2c_dly_scl();
	_i2c_scl_0(ch_index);
	_i2c_dly_scl();
}

static void _nack(uint8_t ch_index)
{
	_i2c_sda_1(ch_index);
	_i2c_dly_scl();
	_i2c_scl_1(ch_index);
	_i2c_dly_scl();
	_i2c_scl_0(ch_index);
	_i2c_dly_scl();
}

static uint8_t _rx_byte(uint8_t ch_index)
{
	uint8_t b = 0;
	_i2c_sda_1(ch_index);
	for (uint8_t i = 0; i < 8; i++)
	{
		_i2c_scl_1(ch_index);
		_i2c_dly_scl();
		b <<= 1;
		if (_i2c_sda_is_high(ch_index))
			b |= 0x01;
		_i2c_scl_0(ch_index);
		_i2c_dly_scl();
	}
	return b;
}



// 私有函数(单次传输)
static uint8_t _I2C_Trans(uint8_t ch_index, uint8_t addr, uint8_t* tx_buf, uint16_t tx_len, uint8_t* rx_buf, uint16_t rx_len)
{
	uint8_t is_success = 0;

	CPU_SR_ALLOC();	
	CPU_CRITICAL_ENTER();
	
	if ( tx_len > 0 )  
	{
		is_success = _start(ch_index);
		if (!is_success) return SOFTI2C_ERR_LINE;
		is_success = _tx_byte(ch_index, addr);
		if (!is_success) return SOFTI2C_ERR_LINE;        
        for (uint16_t i = 0; i < tx_len; i++)
        {
            is_success = _tx_byte(ch_index, tx_buf[i]);
            if (!is_success && rx_len > 0) return SOFTI2C_ERR_LINE;
        }
    }	
    if (rx_len > 0)
	{
		is_success = _start(ch_index);
		if (!is_success) return SOFTI2C_ERR_LINE;
		is_success = _tx_byte(ch_index, addr | 0x01);
		if (!is_success) return SOFTI2C_ERR_LINE;	
        for (uint16_t i = 0; i < rx_len; i++)
        {
            rx_buf[i] = _rx_byte(ch_index);
            if (i != (rx_len - 1))
                _ack(ch_index);
			else
				_nack(ch_index);
        }
    }
	_stop(ch_index);
	_i2c_dly_ms(10);
	CPU_CRITICAL_EXIT();	
	return SOFTI2C_ERR_NONE;
}

uint8_t softi2c_trans(uint8_t ch_index, uint8_t addr, uint8_t* tx_buf, uint16_t tx_len, uint8_t* rx_buf, uint16_t rx_len, uint8_t retry_times)
{
	uint8_t err = 0;
	uint8_t retry = SOFTI2C_RETRY_TIMES_DEFAULT;
	if (retry_times > 0 && retry_times < 100)
		retry = retry_times;
		
	while (retry--)
	{
		err = _I2C_Trans(ch_index, addr, tx_buf, tx_len, rx_buf, rx_len);
		if (err == SOFTI2C_ERR_LINE)
		{
			_i2c_dly_ms(10);
			_reset(ch_index);
		}
		else
		{
			break;
		}
	}
	return err;
}



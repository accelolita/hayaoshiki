/*
 * MCP3425.h
 *
 * Created: 2021/01/21 22:27:21
 *  Author: SIMPSONMAN
 */ 


#define	F_CPU	8000000UL		// CKSEL[1:0]=10, CKDIV8=0
//#define _BV(x) 1<<x
//#define loop_until_bit_is_set(sfr,bit) do { } while (!(sfr & _BV(bit)))

//#include "i2c.h"
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <util/twi.h>

#define MCP3425_address 0xD0
#define ADC_config 0b10010001//AD変換開始　連続モード　240SPS 12bit 2GAIN




void adc_init()
{
	I2C_SendOne(MCP3425_address,ADC_config);
}

uint16_t adc_read()
{
	char val[2]={1,1};
	I2C_Read_No_Red(MCP3425_address,2,&val[0]);
	return ((uint16_t)(val[0]))<<8 | (uint16_t)val[1];
}

char adc_IS_music_playing()
{
	for (int i=0;i<1;i++)
	{
		if (adc_read()==0)
		{
			return 0;
		}
	}
	return 1;
}


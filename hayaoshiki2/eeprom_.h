/*
 * eeprom_.h
 *
 * Created: 2022/01/30 4:31:38
 *  Author: SIMPSONMAN
 */ 

#define	F_CPU	8000000UL		// CKSEL[1:0]=10, CKDIV8=0



#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>

/*
0x0000 volume
0x0001 Player1 point
|
0x000A Player10 point


*/


char eep_read_byte(unsigned int addr)
{
	eeprom_busy_wait();//�ǂݏ����\�܂ő҂�
	return eeprom_read_byte((uint8_t *)addr);
}

void eep_write_byte(unsigned int addr,char data)
{
	eeprom_busy_wait();//�ǂݏ����\�܂ő҂�
	eeprom_write_byte((uint8_t *)addr,data);
}

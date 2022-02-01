/*
 * LCD_I2C_2004.h
 *
 * Created: 2021/01/17 22:16:47
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


#define LCD_ADR 0x4E
#define LCD_RS 0x01
#define LCD_E 0x04
#define LCD_BL 0x08

#define LCD_OFF		0x08 //display off, cursor off, blinking off
#define LCD_DISP	0x0C //display on, cursor off, blinking off
#define LCD_CUR		0x0E //display on, cursor on, blinking off
#define LCD_BLI		0x0D //display on, cursor off, blinking on
#define LCD_ALL		0x0F //display on, cursor on, blinking on

char back_light=0x00;

void lcd_out(char code,char rs);
void lcd_init();
void lcd_cmd(char cmd);
void lcd_data(char asci);
void lcd_data(char asci);
void lcd_pos(char line, char col);
void lcd_set_cg();

void lcd_out(char code, char rs)
{
	char _code = code & 0xF0;
	if (back_light)
	{
		_code |= LCD_BL;
	}
	
	if (rs == 0)
	{
		_code = _code & ~LCD_RS;
		I2C_SendOne(LCD_ADR,_code);
	}
	else
	{
		_code = _code | LCD_RS;
		I2C_SendOne(LCD_ADR,code | LCD_RS);
	}
	_delay_ms(1);
	I2C_SendOne(LCD_ADR,_code | LCD_E);
	_delay_ms(1);
	I2C_SendOne(LCD_ADR,_code & ~LCD_E);
}

void lcd_init()
{
	_delay_ms(15);
	lcd_out(0x30,0);//8bitモード指定
	_delay_ms(5);//4.1ms以上待つ
	lcd_out(0x30,0);//8bitモード指定もっかい
	_delay_ms(1);
	lcd_out(0x30,0);//8bitモード指定もっかいもっかい
	_delay_ms(1);
	lcd_out(0x20,0);//4bitモード指定
	_delay_ms(1);
	lcd_cmd(0x2C);//4bit mode, 2line, 5x8dot
	//lcd_cmd(0x08);//display off, cursor off, blinking off
	//lcd_cmd(0x0C);//display on, cursor off, blinking off
	lcd_cmd(0x0E);//display on, cursor on, blinking off
	//lcd_cmd(0x0D);//display on, cursor off, blinking on
	//lcd_cmd(0x0F);//display on, cursor on, blinking on
	lcd_cmd(0x06);//表示シフト インクリメント, 表示シフトなし
	lcd_cmd(0x02);//cursor home
	lcd_cmd(0x01);//表示を消してカーソルをホームに移動
	
}

void lcd_cmd(char cmd)
{
	lcd_out(cmd,0);
	lcd_out(cmd<<4,0);
	if (0xFC & cmd)
	{
		_delay_ms(2);
	}
	else
	{
		_delay_ms(0.05);
	}

}

void lcd_data(char asci)
{
	lcd_out(asci,1);
	lcd_out(asci<<4,1);
	_delay_ms(0.05);
	
}

void lcd_str(char *str)
{
	while (*str != '\0')
	{
		lcd_data(*str);
		str++;
	}
}

void lcd_pos(char line, char col)
{
	char row_offsets[] = { 0x80, 0xC0, 0x94, 0xD4 };
	lcd_cmd(col + row_offsets[(int)line]);
}

void lcd_set_cg()
{
	lcd_cmd(0x40);//CGRAM 最初のアドレス
	//1文字目 0x00 点
	lcd_data(0b00000100);
	lcd_data(0b00000110);
	lcd_data(0b00000100);	
	lcd_data(0b00011111);
	lcd_data(0b00010001);
	lcd_data(0b00011111);
	lcd_data(0b00001000);
	lcd_data(0b00010101);
	//2文字目 0x01 ↑
	lcd_data(0b00000000);
	lcd_data(0b00000100);
	lcd_data(0b00001110);
	lcd_data(0b00010101);
	lcd_data(0b00000100);
	lcd_data(0b00000100);
	lcd_data(0b00000000);
	lcd_data(0b00000000);
	//3文字目 0x02　↓
	lcd_data(0b00000000);
	lcd_data(0b00000100);
	lcd_data(0b00000100);
	lcd_data(0b00010101);
	lcd_data(0b00001110);
	lcd_data(0b00000100);
	lcd_data(0b00000000);
	lcd_data(0b00000000);
	//4文字目 0x03　目
	lcd_data(0b00011111);
	lcd_data(0b00010001);
	lcd_data(0b00011111);
	lcd_data(0b00010001);
	lcd_data(0b00011111);
	lcd_data(0b00010001);
	lcd_data(0b00011111);
	lcd_data(0b00000000);
	//5文字目 0x04　卍
	lcd_data(0b00000000);
	lcd_data(0b00011101);
	lcd_data(0b00000101);
	lcd_data(0b00011111);
	lcd_data(0b00010100);
	lcd_data(0b00010111);
	lcd_data(0b00000000);
	lcd_data(0b00000000);
	//6文字目 0x05　左回転風車
	lcd_data(0b00000000);
	lcd_data(0b00010011);
	lcd_data(0b00011010);
	lcd_data(0b00000100);
	lcd_data(0b00001011);
	lcd_data(0b00011001);
	lcd_data(0b00000000);
	lcd_data(0b00000000);
	//7文字目 0x06　♪
	lcd_data(0b00000100);
	lcd_data(0b00000110);
	lcd_data(0b00000101);
	lcd_data(0b00000101);
	lcd_data(0b00011101);
	lcd_data(0b00011100);
	lcd_data(0b00011100);
	lcd_data(0b00000000);
	//8文字目 0x07　10
	lcd_data(0b00010111);
	lcd_data(0b00010101);
	lcd_data(0b00010101);
	lcd_data(0b00010101);
	lcd_data(0b00010101);
	lcd_data(0b00010101);
	lcd_data(0b00010111);
	lcd_data(0b00000000);
}


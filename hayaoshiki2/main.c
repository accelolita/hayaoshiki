/*
 * hayaoshiki2.c
 *
 * Created: 2020/11/29 1:41:25
 * Author : SIMPSONMAN
 */ 

#define	F_CPU	8000000UL		// CKSEL[1:0]=10, CKDIV8=0
#define _BV(x) 1<<x
#define loop_until_bit_is_set(sfr,bit) do { } while (!(sfr & _BV(bit)))
#define loop_until_bit_is_clear(sfr,bit) do { } while ((sfr & _BV(bit)))
#define log2(x) 
#define WDT_reset() __asm__ __volatile__ ("wdr")
#define DEBUG_FLAG 0

#define PlaySound(x) dfCommand[6]=x;SendCommand(&dfCommand[0],(unsigned char)0x08);

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include "i2c.h"
#include "LCD_I2C_2004.h"
#include "Quiz_counter.h"
#include "MCP3425.h"
#include "eeprom_.h"




uint16_t hayaoshiPin=0;//0bFEDCBA9876543210
//|||||||||||||||||+-PINA0
//||||||||||||||||+--PINA1
//|||||||||||||||+---PINA2
//||||||||||||||+----PINA3
//|||||||||||||+-----PINA4
//||||||||||||+------PINA5
//|||||||||||+-------PINA6
//||||||||||+--------PINA7
//|||||||||+---------PINC6
//||||||||+----------PINC7
//|||||||+-----------ローが出たかどうか　A

uint16_t PINAC;//一時保存する奴
int tCounter=0;
unsigned char blinkingCounter=0;

ISR(TIMER0_COMPA_vect)  //タイマー割り込み　
{
	if (blinkingCounter<0xFF)
	{
		blinkingCounter++;
	}
	else
	{
		blinkingCounter=0;
	}
	while(tCounter < 10)
	{
		if ((~PINAC & hayaoshiPin) & 0x01<<tCounter)
		{	
			if(blinkingCounter<128) PORTB=tCounter;
			else PORTB=0b00001110;
			
			if (tCounter>=9)
			{
				tCounter=0;
			}
			else{
				tCounter++;
			}
			return ;
		}
		tCounter++;
	}
	tCounter=0;
}

ISR(TIMER0_COMPB_vect)  //タイマーB割り込み //全点灯
{
	PORTB=tCounter;			
	if (tCounter>=9)
	{
		tCounter=0;
	}
	else{
		tCounter++;
	}	
}

void SendCommand(char* str,unsigned char num){
	for (char i=0;i<num;i++)
	{
		while(!(UCSR0A & 0b00100000));
		UDR0=*str;
		str++;
	}
}


void ResetRespondent(char sound,char playerLed);
void ChangeVolume(char dir);
void SetRespondent(char sound);
void Show_adc_level();


int main(void)
{
	//タイマー
	TCCR0A = 0b00000010;//タイマー０
	TCCR0B = 0b00000101; // 1024分周
	OCR0A =13;  // 3.5ｍｓ
	OCR0B =13;	// 3.5ｍｓ タイマーB 13以外にすると動かない謎
	TIMSK0 = 0b0000010;	//タイマー0A割り込み許可
	//TIMSK0 = 0b0000100;	//タイマー0B割り込み許可
	
	//PORTCを使うためJTAG禁止
	
    //早押しボタン入力 //四極ステレオコントロール　出力//音量//リセット
	DDRA=0x00;
	PORTA=0xFF;
	DDRC= 0x20;//PC7,6 IN, PC5 OUT, PC4 青, PC3 黄色, PC2 赤
	PORTC=0xDC;
	
	
	//回答権LED出力
	DDRB=0x0F;//PB0,1,2,3
	PORTB=0b00001110;//下位4ビットが0x02以上なら10番目以降のシフトレジスタのピンがハイになるので光らない
	
	//正誤ボタン入力　ビジー入力　ステレオカット
	DDRD= 0b10000000;//PD7ステレオカット,　PD6,5正誤ボタン,PD4ビジー
	PORTD=0b11100000;//負論理　LOWでステレオカット,PD５６プルアップ、PD４プルアップ無し
	
	
	//シリアル
	UBRR0=51;//9615bps
	//UDR0	データレジスタ
	UCSR0A=0b00000000;	//5bit 送信可能フラグ
	UCSR0B=0b00011000;	//送受信機能しよう
	UCSR0C=0b00000110;	//データ8ビット　stop 1bit パリティなし 非同期
	
	char dfCommand[8]={0x7E,0xFF,0x06,0x03,0x00,0x00,0x01,0xEF};//ルートディレクトリの1番目の曲再生
		
	I2C_Init();
	lcd_init();
	adc_init();
	//volume 取得
	volume=eep_read_byte(0x00);
	if (volume<=0 || volume>30)
	{
		volume=20;
	}
	//配点取得
	Load_score();
	_delay_ms(500);

	lcd_set_cg();
	lcd_cmd(LCD_DISP); //display on, cursor off, blinking off
	//Load_score();
	Show_score();
	Write_volume(volume);		

	
	//7E FF 06 06 00 00 0F EF
	//7E FF 06 06 00 00 0F EF
	dfCommand[3]=0x06;
	dfCommand[6]=volume;
	SendCommand(&dfCommand[0],(unsigned char)0x08);//音量30
	dfCommand[3]=0x03;
	dfCommand[6]=0x01;	
	
	char settingFlag= 0b00000000;
					//0b76543210
					//|||||||||+-0 1:回答者待ちフラグ　
					//||||||||+--1 1:回答者いるフラグ
					//|||||||+---2
					//||||||+----3
					//|||||+-----4
					//||||+------5 数字読み上げ
					//|||+-------6 誤答マイナス
					//||+--------7

	
	//数字読み設定正解押しながら起動でオン
	if (~PIND & 0b00100000)
	{
		settingFlag|=0b00100000;
		PlaySound(0x01);
	}
	if (~PIND & 0b01000000)//誤答　マイナス
	{
		settingFlag|=0b01000000;
	}
	
	

	back_light=0x01;
	Write_status(0x0E);

    while(1) 
    {
		//回答ボタン読み取り
		if(settingFlag & 0x01) PINAC=(uint16_t)PINA | ((((uint16_t)PINC)<<2) & 0xFF00);
		//回答判定
		for (int i = 0; (i < 10) && (settingFlag & 0x01); i++)
		{
			if ((PINAC & 0x01<<i))
			{
				hayaoshiPin |= 0x01<<i;			
			}
			else if(hayaoshiPin & 0x01<<i)
			{
				settingFlag|=0b00000010;//回答者いるフラグ
				lcd_pos(3,13);
				lcd_str("PLAYER");
				if (i==9)
				{
					lcd_data(0x07);//10
				}
				else
				{
					lcd_data(i+0x31);
				}							
				SetRespondent(0x01);

				if (settingFlag & 0b00100000)//回答者番号
				{
					PlaySound(i+9);
				}
				Write_status(2);//正誤判定待ち
			}
		}
		if ((settingFlag & 0b00000011) == 0b00000011)
		{
			settingFlag&=0b11111100;//回答者フラグ
		}
		//正解判定
		if (~PIND & 0b00100000)
		{
			if (status==3 || status==4)//編集設定モード
			{
				lcd_cmd(LCD_DISP);//display on, cursor off, blinking off
				PlaySound(21);//操作音	
				Write_status(14);
				//設定保存
				eep_write_byte(0,volume);
				Save_score();
				loop_until_bit_is_set(PIND,5);
				goto reset;
			}
			if (status==2)//正解判定中に正解ボタン
			{
				//点数アップ
				for (int i=0;(i<10);i++)
				{
					if (~PINAC & 0x01<<i)
					{
						point[i]++;
						Write_score(i);
					}
				}
				Save_score();
				cli();//割込み禁止
				ResetRespondent(0x07,0b00001110);//スタンバイ				
			}
			else if (status== 0b00001111)//レディー
			{
				settingFlag&=0b11111100;//回答者フラグなし
				cli();//割込み禁止
				ResetRespondent(0x07,0b00001110);//スタンバイ
			}
			else if (status== 0b00001110)//スタンバイ
			{
				cli();//割込み禁止
				ResetRespondent(0x07,0b00001110);//スタンバイ
			}

		}
		else if(~PIND & 0b01000000)//不正解判定
		{
			if (status==3 || status==4)//編集設定モード
			{
				lcd_cmd(LCD_DISP);//display on, cursor off, blinking off
				PlaySound(21);//操作音
				Write_status(14);//スタンバイ
				//設定保存
				eep_write_byte(0,volume);	
				Save_score();
				loop_until_bit_is_set(PIND,6);
				goto reset;
			}
			//点数ダウン
			if (settingFlag & 0b01000000)
			{
				for (int i=0;(i<10) && (status==2);i++)
				{
					if (~PINAC & 0x01<<i)
					{
						point[i]--;
						Write_score(i);
					}
				}
			}	
			
			if (status== 2)
			{
				settingFlag|=0b00000001;//回答受付
				TIMSK0 = 0b0000100;	//タイマー0B割り込み許可
				sei();//割込み許可				
				ResetRespondent(0x02,0b00001111);//レディー
			}
			else if (status== 0b00001111)//レディー
			{
				settingFlag&=0b11111100;//回答者フラグなし
				cli();//割込み禁止
				ResetRespondent(0x02,0b00001110);//スタンバイ
			}
			else if (status== 0b00001110)//スタンバイ
			{
				cli();//割込み禁止
				ResetRespondent(0x02,0b00001110);//スタンバイ							
			}
					

		}
		reset://ぶっ飛びポイント
		//ボリューム設定
		//青下げる
		if (~PINC & 0b00010000)
		{
	
			if (status==3)//セレクトモード
			{
				PlaySound(21);//移動音、連続すると音量が大きくなるバグあり	
				if (setting_cursor<=0)
				{
					setting_cursor=10;
					lcd_pos(setting_cursor/3,(setting_cursor%3)*7+1);	
				}
				else
				{
					setting_cursor--;
					lcd_pos(setting_cursor/3,(setting_cursor%3)*7+1);
				}
				
			}
			else if (status==4)//編集モード
			{	
				PlaySound(20);		
				if (setting_cursor==10)//音量下げる
				{
					ChangeVolume(0);
					goto reset;
				}
				else//点数下げる
				{
					point[(int)setting_cursor]--;
					Write_score(setting_cursor);
					lcd_pos(setting_cursor/3,(setting_cursor%3)*7+5);//点にカーソルを移動
				}
			}
			else if(status!=15 && status!=2)//レディーとアンサー以外
			{
				PlaySound(21);//音量ダウンサウンド
				Write_status(3);//SETTING
				setting_cursor=10;//音量
				lcd_cmd(LCD_BLI);//display on, cursor off, blinking on
				lcd_pos(setting_cursor/3,(setting_cursor%3)*7+1);			
			}			
			_delay_ms(250);//ビジーがLOWになるまで待機
			loop_until_bit_is_set(PIND,PIND4);//ビジー消えるまで待機	
			
		}
		//赤あげる
		if (~PINC & 0b00000100)
		{

			if (status==3)//セレクトモード
			{
				PlaySound(21);//音量ダウンサウンド
				if (setting_cursor>=10)
				{
					setting_cursor=0;
					lcd_pos(setting_cursor/3,(setting_cursor%3)*7+1);
				}
				else
				{
					setting_cursor++;
					lcd_pos(setting_cursor/3,(setting_cursor%3)*7+1);
				}
			}
			else if (status==4)//編集モード
			{
				PlaySound(19);	//音量アップサウンド
				if (setting_cursor==10)//音量あげる
				{
					ChangeVolume(1);
					goto reset;
				}
				else//点数下げる
				{
					point[(int)setting_cursor]++;
					Write_score(setting_cursor);
					lcd_pos(setting_cursor/3,(setting_cursor%3)*7+5);//点にカーソルを移動
				}
			}
			else if(status==15)//レディの場合iponeコマンドリレーと連動
			{
				PORTC|=0x20;//i phone 再生停止		
			}
			else if(status!=15 && status!=2)//レディーとアンサー以外
			{
				PlaySound(21);//音量ダウンサウンド
				Write_status(3);//SETTING
				setting_cursor=0;//プレイヤー１
				lcd_cmd(LCD_BLI);//display on, cursor off, blinking on
				lcd_pos(setting_cursor/3,(setting_cursor%3)*7+1);
			}
			_delay_ms(250);//ビジーがLOWになるまで待機
			loop_until_bit_is_set(PIND,PIND4);//ビジー消えるまで待機
		}
		else if (status==15)//レディの場合iponeコマンドリレーと連動
		{
			PORTC&=0xDF;//i phone 再生停止
		}
		//リセット黄色
		if (~PINC & 0b00001000)
		{		
			
			if (status==3)//設定モード
			{
				PlaySound(21);//音量ダウンサウンド
				Write_status(4);//編集モード				
				lcd_pos(setting_cursor/3,(setting_cursor%3)*7+5);//点にカーソル移動
				_delay_ms(250);//ビジーがLOWになるまで待機
				loop_until_bit_is_set(PIND,PIND4);//ビジー消えるまで待機			
			}
			else if(status==4)//編集モード
			{
				PlaySound(21);//音量ダウンサウンド
				Write_status(3);//設定モード
				lcd_pos(setting_cursor/3,(setting_cursor%3)*7+1);
				_delay_ms(250);//ビジーがLOWになるまで待機
				loop_until_bit_is_set(PIND,PIND4);//ビジー消えるまで待機
			}
			else
			{
				settingFlag|=0b00000001;//回答受付
				TIMSK0 = 0b0000100;	//タイマー0B割り込み許可
				sei();//割込み許可					
				ResetRespondent(0x08,0b00001111);//レディー				
			}
			
			
			//点数リセット
			if (PINC & 0b00000100)
			{
				for (int i=0;i<10;i++)
				{
					point[i]=0;
				}
				Show_score();
			}

		
		}
    }
}

void SetRespondent(char sound)
{
		char dfCommand[8]={0x7E,0xFF,0x06,0x03,0x00,0x00,0x01,0xEF};//ルートディレクトリの1番目の曲再生
		PlaySound(sound);
		TIMSK0 = 0b0000010;	//タイマー0A割り込み許可
		sei();//割込み許可
		//PORTB=i;
		Write_status(2);//アンさー
		PORTD&=0x7F;//ステレオカット
		if (adc_IS_music_playing())
		{
			PORTC|=0x20;//i phone 再生停止
			_delay_ms(500);//ビジーがLOWになるまで待機
			PORTC&=0xDF;//i phone 再生停止			
		}	
		loop_until_bit_is_set(PIND,PIND4);//ビジー消えるまで待機	
}

void ResetRespondent(char sound,char playerLed)
{
	char dfCommand[8]={0x7E,0xFF,0x06,0x03,0x00,0x00,0x01,0xEF};//ルートディレクトリの1番目の曲再生
	PlaySound(sound);
	PORTB=playerLed;
	hayaoshiPin =0;
	PORTD|=0x80;//ステレオオン
	Write_status(playerLed);
	_delay_ms(500);//ビジーがLOWになるまで待機
	loop_until_bit_is_set(PIND,PIND4);//ビジー消えるまで待機
}

void ChangeVolume(char dir)
{
	char dfCommand[8]={0x7E,0xFF,0x06,0x06,0x00,0x00,0x01,0xEF};
		
	if (dir && volume<30)
	{
		volume++;
		dfCommand[6]=volume;
		SendCommand(&dfCommand[0],(unsigned char)0x08);//音量あげた
		dfCommand[3]=0x03;
		PlaySound(19);//音量アップサウンド
	}
	else if(!dir && volume>0)
	{
		volume--;
		dfCommand[6]=volume;
		SendCommand(&dfCommand[0],(unsigned char)0x08);//音量さげた
		dfCommand[3]=0x03;
		PlaySound(20);//音量ダウンサウンド	
	}
	else
	{
		if (volume>30)
		{
			volume=30;
			dfCommand[6]=volume;
			SendCommand(&dfCommand[0],(unsigned char)0x08);//音量
			dfCommand[3]=0x03;
		}
		if (volume<=0)
		{
			volume=1;
			dfCommand[6]=volume;
			SendCommand(&dfCommand[0],(unsigned char)0x08);//音量
			dfCommand[3]=0x03;
		}
	}
	Write_volume();
	_delay_ms(500);//ビジーがLOWになるまで待機
	loop_until_bit_is_set(PIND,PIND4);//ビジー消えるまで待機
}

void Show_adc_level()
{
	uint16_t val;
	char temp;
	lcd_pos(3,16);
	val=adc_IS_music_playing();
	val&=0x0FFF;
	temp=(char)(val/1000);
	lcd_data(temp+0x30);
	val-=1000*(int)temp;
	temp=(char)(val/100);
	lcd_data(temp+0x30);
	val-=100*(int)temp;
	temp=(char)(val/10);
	lcd_data(temp+0x30);
	lcd_data(val%10+0x30);
}
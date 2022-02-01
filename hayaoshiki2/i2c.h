/*
 * i2c.h
 *
 * Created: 2019/11/11 1:43:21
 *  Author: SIMPSONMAN
 */ 

#define	F_CPU	8000000UL		// CKSEL[1:0]=10, CKDIV8=0
//#define _BV(x) 1<<x
//#define loop_until_bit_is_set(sfr,bit) do { } while (!(sfr & _BV(bit)))
	
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <util/twi.h>


void I2C_Init();
void I2C_Set(char addres,char red,char num,char *data,char inc);
void I2C_Read(char addres,char red,char num,char *data);



void I2C_Init()
{
	// TWSR[TWPS] = 00(div1) & TWBR=2 -> 400kHz in 8MHz CPU
	TWSR = 0;
	TWBR = 2;//400kHz
}

void I2C_Set(char addres,char red,char num,char *data,char inc)//　incは自動インクリメントがありかなしか
{
	TWCR=_BV(TWINT)|_BV(TWSTA)|_BV(TWEN);//開始条件
	loop_until_bit_is_set(TWCR,TWINT);//フラグ待ち
	//SendByteASCII(TWSR&0xF8);
	#if DEBUG_FLAG
	if ((TWSR&0xF8)!=0x08)
	{
		SendByteASCII(TWSR&0xF8);
		goto I2C_Start_return;
	}
	#endif
	//スレーブアドレス指定　送信
	TWDR=addres & 0xFE;
	TWCR=_BV(TWINT)|_BV(TWEN);
	loop_until_bit_is_set(TWCR,TWINT);
	//SendByteASCII(TWSR&0xF8);
	#if DEBUG_FLAG
	if ((TWSR&0xF8)!=0x18)
	{
		SendByteASCII(TWSR&0xF8);
		goto I2C_Start_return;
	}
	#endif
	//自動インクリメントありRTCレジスタ指定
	if (inc==0)
	{
		TWDR=red;
		TWCR=_BV(TWINT)|_BV(TWEN);
		loop_until_bit_is_set(TWCR,TWINT);
		//SendByteASCII(TWSR&0xF8);
		#if DEBUG_FLAG
		if ((TWSR&0xF8)!=0x28)
		{
			SendByteASCII(TWSR&0xF8);
			goto I2C_Start_return;
		}
		#endif
	}
	//データセットループ
	for (int i=0;i<num;i++)
	{
		//自動インクリメントなしレジスタ指定
		if (inc)
		{
			TWDR=red;
			TWCR=_BV(TWINT)|_BV(TWEN);
			loop_until_bit_is_set(TWCR,TWINT);
			//SendByteASCII(TWSR&0xF8);
			#if DEBUG_FLAG
			if ((TWSR&0xF8)!=0x28)
			{
				SendByteASCII(TWSR&0xF8);
				goto I2C_Start_return;
			}
			#endif
		}
		//データをセット
		TWDR=*data;
		TWCR=_BV(TWINT)|_BV(TWEN);
		loop_until_bit_is_set(TWCR,TWINT);
		//SendByteASCII(TWSR&0xF8);
		#if DEBUG_FLAG
		if ((TWSR&0xF8)!=0x28)
		{
			SendByteASCII(TWSR&0xF8);
			goto I2C_Start_return;
		}
		#endif
		data++;
	}
	#if DEBUG_FLAG
	I2C_Start_return:
	#endif
	
	TWCR=_BV(TWINT)|_BV(TWSTO)|_BV(TWEN);//終了条件
}


void I2C_SetOne(char addres,char red,char data)
{
	I2C_Set(addres,red,1,&data,0);
}

void I2C_Read(char addres,char red,char num,char *data)
{
	cli();
	//開始条件
	TWCR=_BV(TWINT)|_BV(TWSTA)|_BV(TWEN);
	loop_until_bit_is_set(TWCR,TWINT);//フラグ待ち
	#if DEBUG_FLAG
	if ((TWSR&0xF8)!=0x08)
	{
		SendSerial("Start_R");
		SendByteASCII(TWSR&0xF8);
		goto I2C_Rev_return;
	}
	#endif
	//スレーブアドレス指定先ずは書き込みモード
	TWDR=addres;
	TWCR=_BV(TWINT)|_BV(TWEN);
	loop_until_bit_is_set(TWCR,TWINT);
	#if DEBUG_FLAG
	if ((TWSR&0xF8)!=0x18)
	{
		SendByteASCII(TWSR&0xF8);
		goto I2C_Rev_return;
	}
	#endif
	//読みだすRTCレジスタ指定
	TWDR=red;
	TWCR=_BV(TWINT)|_BV(TWEN);
	loop_until_bit_is_set(TWCR,TWINT);
	#if DEBUG_FLAG
	if ((TWSR&0xF8)!=0x28)
	{
		SendByteASCII(TWSR&0xF8);
		goto I2C_Rev_return;
	}
	#endif
	//読み出し開始条件
	TWCR=_BV(TWINT)|_BV(TWSTA)|_BV(TWEN);
	loop_until_bit_is_set(TWCR,TWINT);//フラグ待ち
	#if DEBUG_FLAG
	if ((TWSR&0xF8)!=0x10 )//0x10が返ってくる送信モードの終了条件を返さず受信モードで開始するから
	{
		SendByteASCII(TWSR&0xF8);
		goto I2C_Rev_return;
	}
	#endif
	//スレーブアドレス指定読み出しモード
	TWDR=addres | 0x01;
	TWCR=_BV(TWINT)|_BV(TWEN);
	loop_until_bit_is_set(TWCR,TWINT);
	#if DEBUG_FLAG
	if ((TWSR&0xF8)!=0x40)
	{
		SendByteASCII(TWSR&0xF8);
		goto I2C_Rev_return;
	}
	#endif

	//受信
	for (int i=0;i<num-1;i++)
	{
		TWCR=_BV(TWINT)|_BV(TWEA)|_BV(TWEN);//受信ackあり
		loop_until_bit_is_set(TWCR,TWINT);
		#if DEBUG_FLAG
		if ((TWSR&0xF8)!=0x50)
		{
			SendByteASCII(TWSR&0xF8);
			goto I2C_Rev_return;
		}
		#endif
		*data=TWDR;//データセット
		data++;		//データ送り
	}
	//受信最後
	TWCR=_BV(TWINT)|_BV(TWEN);
	loop_until_bit_is_set(TWCR,TWINT);
	#if DEBUG_FLAG
	if ((TWSR&0xF8)!=0x58)//0x58が返ってくる　最後だから　CPUがACK返してないから
	{
		SendByteASCII(TWSR&0xF8);
		goto I2C_Rev_return;
	}
	#endif
	*data=TWDR;//データセット
	
	#if DEBUG_FLAG
	I2C_Rev_return:
	#endif
	
	TWCR=_BV(TWINT)|_BV(TWSTO)|_BV(TWEN);//終了条件
	sei();
}

char I2C_ReadOne(char addres,char red)
{
	char data;
	I2C_Read(addres,red,1,&data);
	return data;
}

void I2C_SendOne(char addres,char data)
{
	TWCR=_BV(TWINT)|_BV(TWSTA)|_BV(TWEN);//開始条件
	loop_until_bit_is_set(TWCR,TWINT);//フラグ待ち
	//SendByteASCII(TWSR&0xF8);
	#if DEBUG_FLAG
	if ((TWSR&0xF8)!=0x08)
	{
		SendByteASCII(TWSR&0xF8);
		goto I2C_Start_return;
	}
	#endif
	
	//スレーブアドレス指定　送信
	TWDR=addres & 0xFE;
	TWCR=_BV(TWINT)|_BV(TWEN);
	loop_until_bit_is_set(TWCR,TWINT);
	//SendByteASCII(TWSR&0xF8);
	#if DEBUG_FLAG
	if ((TWSR&0xF8)!=0x18)
	{
		SendByteASCII(TWSR&0xF8);
		goto I2C_Start_return;
	}
	#endif
	
	//データをセット
	TWDR=data;
	TWCR=_BV(TWINT)|_BV(TWEN);
	loop_until_bit_is_set(TWCR,TWINT);
	//SendByteASCII(TWSR&0xF8);
	#if DEBUG_FLAG
	if ((TWSR&0xF8)!=0x28)
	{
		SendByteASCII(TWSR&0xF8);
		goto I2C_Start_return;
	}
	#endif
		
	#if DEBUG_FLAG
	I2C_Start_return:
	#endif
	
	TWCR=_BV(TWINT)|_BV(TWSTO)|_BV(TWEN);//終了条件
}

void I2C_Read_No_Red(char addres,char num,char *data)
{
	cli();
	//開始条件
	TWCR=_BV(TWINT)|_BV(TWSTA)|_BV(TWEN);
	loop_until_bit_is_set(TWCR,TWINT);//フラグ待ち
	#if DEBUG_FLAG
	if ((TWSR&0xF8)!=0x08)
	{
		SendSerial("Start_R");
		SendByteASCII(TWSR&0xF8);
		goto I2C_Rev_return;
	}
	#endif
	
	//スレーブアドレス指定先ずは書き込みモード
	TWDR=addres;
	TWCR=_BV(TWINT)|_BV(TWEN);
	loop_until_bit_is_set(TWCR,TWINT);
	#if DEBUG_FLAG
	if ((TWSR&0xF8)!=0x18)
	{
		SendByteASCII(TWSR&0xF8);
		goto I2C_Rev_return;
	}
	#endif
	
	//読み出し開始条件
	TWCR=_BV(TWINT)|_BV(TWSTA)|_BV(TWEN);
	loop_until_bit_is_set(TWCR,TWINT);//フラグ待ち
	#if DEBUG_FLAG
	if ((TWSR&0xF8)!=0x10 )//0x10が返ってくる送信モードの終了条件を返さず受信モードで開始するから
	{
		SendByteASCII(TWSR&0xF8);
		goto I2C_Rev_return;
	}
	#endif
	
	//スレーブアドレス指定読み出しモード
	TWDR=addres | 0x01;
	TWCR=_BV(TWINT)|_BV(TWEN);
	loop_until_bit_is_set(TWCR,TWINT);
	#if DEBUG_FLAG
	if ((TWSR&0xF8)!=0x40)
	{
		SendByteASCII(TWSR&0xF8);
		goto I2C_Rev_return;
	}
	#endif

	//受信
	for (int i=0;i<num-1;i++)
	{
		TWCR=_BV(TWINT)|_BV(TWEA)|_BV(TWEN);//受信ackあり
		loop_until_bit_is_set(TWCR,TWINT);
		#if DEBUG_FLAG
		if ((TWSR&0xF8)!=0x50)
		{
			SendByteASCII(TWSR&0xF8);
			goto I2C_Rev_return;
		}
		#endif
		*data=TWDR;//データセット
		data++;		//データ送り
	}
	//受信最後
	TWCR=_BV(TWINT)|_BV(TWEN);
	loop_until_bit_is_set(TWCR,TWINT);
	#if DEBUG_FLAG
	if ((TWSR&0xF8)!=0x58)//0x58が返ってくる　最後だから　CPUがACK返してないから
	{
		SendByteASCII(TWSR&0xF8);
		goto I2C_Rev_return;
	}
	#endif
	*data=TWDR;//データセット
	
	#if DEBUG_FLAG
	I2C_Rev_return:
	#endif
	
	TWCR=_BV(TWINT)|_BV(TWSTO)|_BV(TWEN);//終了条件
	sei();
}
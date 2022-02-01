/*
 * Quiz_counter.c
 *
 * Created: 2021/01/18 11:53:14
 *  Author: SIMPSONMAN
 */ 


signed char point[10]={0,0,0,0,0,0,0,0,0,0};
unsigned char volume;	//ボリューム
char status=0b00001110;//standby
char setting_cursor=0;


void Write_score(char num);
void Show_score();
void Write_volume();
void Write_status(char status);


void Show_score()
{
	for (char i=0;i<10;i++)
	{
		Write_score(i);
	}
}

void Write_score(char num)
{
	//書き込み位置
	lcd_pos(num/3,(num%3)*7);
	if (num!=9)
	{
		lcd_data(0x31+num);
	}
	else
	{
		lcd_data(0x07);
	}

	lcd_data('P');
	//ポイント書き込み
	if (point[(int)num]>=10)
	{
		if (point[(int)num]>99)
		{
			point[(int)num]=99;
		}
		lcd_data(':');
		lcd_data((point[(int)num])/10+0x30);
		lcd_data((point[(int)num])%10+0x30);				
	}
	else if(point[(int)num]>=0)
	{
		lcd_data(':');
		lcd_data(' ');
		lcd_data(point[(int)num]+0x30);
	}
	else if(point[(int)num]>-10)
	{
		lcd_data(':');
		lcd_data('-');
		lcd_data(-point[(int)num]+0x30);
	}	
	else
	{
		if (point[(int)num]<-99)
		{
			point[(int)num]=-99;
		}
		lcd_data('-');
		lcd_data((-point[(int)num])/10+0x30);
		lcd_data((-point[(int)num])%10+0x30);
	}
	lcd_data(0x00);//点
}

void Write_volume()
{
	lcd_pos(3,8);
	lcd_data(0x06);
	lcd_data(':');
	lcd_data(volume/10+0x30);
	lcd_data(volume%10+0x30);
	//lcd_pos(3,11);//カーソルを一桁目にする
}

void Write_status(char _status)//0:STANDBY
{
	lcd_pos(3,13);
	switch (_status)
	{
		case 0b00001110://14
		/* Your code here */
		lcd_str("STANDBY");		
		break;
		case 0b00001111://15
		/* Your code here */
		lcd_str(" READY ");
		break;
		case 2://正誤判定待ち
		/* Your code here */
		//lcd_str(" ANSWER");
		break;
		case 3:
		/* Your code here */
		lcd_str("SETTING");
		break;
		
		case 4:	
		lcd_str(" EDIT  ");
		break;	
		default:		
		/* Your code here */
		lcd_str(" ERROR ");
		break;
	}
	status=_status;
}
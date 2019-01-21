//此文件中定义了单片机的一些特殊功能寄存器
#include "reg52.h"			 
	
//对数据类型进行声明定义
typedef unsigned int u16;
typedef unsigned char u8;

//定义三个数码管A, B, C
sbit LSA=P2^2;
sbit LSB=P2^3;
sbit LSC=P2^4;

// 定义红外接受的引脚
// IRIN==1时确定收到的是正确信号
sbit IRIN=P3^2; 

u8 IrValue[6];
// [0]是用户正码 [1]用户反码 用来区别不同遥控器
// [2]按键正码 [3]按键反码 校验

u8 Time; 
u8 DisplayData[8];

//定义数码管0、1、2、3、4、5、6、7、8、9、A、b、C、d、E、F、H的显示码
u8 code smgduan[17]={
0x3f,0x06,0x5b,0x4f,0x66,0x6d,0x7d,0x07,
0x7f,0x6f,0x77,0x7c,0x39,0x5e,0x79,0x71,0X76};


// 延时函数，i=1时，大约延时10us 
void delay(u16 i)
{
	while(i--);	
}

// 数码管显示函数 
void DigDisplay()
{
	u8 i;
	for(i=0;i<3;i++)
	{
		//位选，选择点亮的数码管，
		switch(i)	 
		{
			//数码管=1时代表点亮, =0时代表不亮
			case(0):
				LSA=0;LSB=0;LSC=0; break;//显示第0位
			case(1):
				LSA=1;LSB=0;LSC=0; break;//显示第1位
			case(2):
				LSA=0;LSB=1;LSC=0; break;//显示第2位	
		}
		P0=DisplayData[i];//发送数据
		delay(100); //间隔一段时间扫描	
		P0=0x00;//消隐
	}		
}

// 初始化红外线接收 
void IrInit()
{
	IT0=1;//下降沿触发
	EX0=1;//打开中断0允许
	EA=1;	//打开总中断 
	IRIN=1;//初始化端口
}
 
void main()
{	
	IrInit();
	while(1)
	{	
		// IrValue[2]表示编码值
		// 高位用除法结果
		DisplayData[0] = smgduan[IrValue[2]/16];
		// 低位用余数
		DisplayData[1] = smgduan[IrValue[2]%16];
		// 永远是'H'表示十六进制
		DisplayData[2] = smgduan[16];
	    DigDisplay();		
	}		
}

// 红外解码原理
// 读取红外数值的中断函数(外部中断函数)

// 读取红外数值的中断函数(外部中断函数)
// NEC标准
void ReadIr() interrupt 0
{
	u8 j,k;
	u16 err;
	Time=0;					 
	delay(700);	//7ms
	if(IRIN==0)		//确认是否真的接收到正确的信号
	{	 
		
		err=1000;				//1000*10us=10ms,超过说明接收到错误的信号
		while((IRIN==0)&&(err>0))	//等待前面9ms的低电平过去  (起始码)		
		{			
			delay(1);
			err--;
		} 
		if(IRIN==1)			//如果正确等到9ms低电平
		{
			err=500;
			while((IRIN==1)&&(err>0))		 //等待4.5ms的起始高电平过去(引导码)
			{
				delay(1);
				err--;
			}
			for(k=0;k<4;k++)		//共有4组数据
			{				
				for(j=0;j<8;j++)	//接收一组数据
				{

					err=60;		
					while((IRIN==0)&&(err>0))//等待信号前面的560us低电平过去
					{
						delay(1);
						err--;
					}
					err=500;
					while((IRIN==1)&&(err>0))	 //计算高电平的时间长度。
					{
						delay(10);	 //0.1ms
						Time++;
						err--;
						if(Time>30)
						{
							return;
						}
					}
					IrValue[k]>>=1;	 //k表示第几组数据
					if(Time>=8)			//如果高电平出现大于565us，那么是1
					{
						IrValue[k]|=0x80;
					}
					Time=0;		//用完时间要重新赋值							
				}
			}
		}
		// 判断正码反码
		if(IrValue[2]!=~IrValue[3])
		{
			return;//有错误 跳出结束
		}
	}			
} 

// 以脉宽为0.565ms、间隔0.56ms、周期为1.125ms的组合表示二进制的“0”；
// 脉宽为0.565ms、间隔1.685ms、周期为2.25ms的组合表示二进制的“1”
// 起始码（9ms）和结束码（2.5ms） 确定连按
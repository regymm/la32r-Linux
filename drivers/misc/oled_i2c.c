#include "oled_i2c.h"
#include "bx_i2c.h"
#include "oledfont_i2c.h"

#include <linux/delay.h>

void init_i2c_oled(unsigned char i2c_num);
void OLED_Clear_i2c(void);
void OLED_On_i2c(void);
void OLED_Display_On_i2c(void);
void OLED_Display_Off_i2c(void);
void OLED_Set_Pos_i2c(unsigned char x, unsigned char y);
unsigned int oled_pow_i2c(unsigned char m,unsigned char n);
void OLED_ShowChar_i2c(unsigned char x,unsigned char y,unsigned char chr,unsigned char Char_Size);
void OLED_ShowNum_i2c(unsigned char x,unsigned char y,unsigned int num,unsigned char len,unsigned char size2);
void OLED_ShowString_i2c(unsigned char x,unsigned char y,unsigned char *chr,unsigned char Char_Size);

static void OLED_WR_Byte_i2c(unsigned char dat,unsigned char cmd)
{
        unsigned char send_buff;

	if(cmd)
	{
           send_buff = 0x40;
        }
        else
        {
           send_buff = 0x00;
        }
    
        i2c_send_start_and_addr(&i2c_info, slave_addr, BX_I2C_DIRECTION_WRITE);
        i2c_receive_ack(&i2c_info);
        i2c_send_data(&i2c_info, &send_buff, 1);
        i2c_send_data(&i2c_info, &dat, 1);
        i2c_send_stop(&i2c_info);
        mdelay(1);
}

void init_i2c_oled(unsigned char i2c_num)
{
     i2c_info.clock = 50*1000;       // 50kb/s
     if(0==i2c_num)
         i2c_info.I2Cx = BX_I2C_0;
     else if(1==i2c_num)
         i2c_info.I2Cx = BX_I2C_1;
     else if(2==i2c_num)
         i2c_info.I2Cx = BX_I2C_2;
     else
         i2c_info.I2Cx = BX_I2C_3;

     i2c_init(&i2c_info);

     mdelay(800);
        OLED_WR_Byte_i2c(0xAE,OLED_CMD);//--display off
	OLED_WR_Byte_i2c(0x00,OLED_CMD);//---set low column address
	OLED_WR_Byte_i2c(0x10,OLED_CMD);//---set high column address
	OLED_WR_Byte_i2c(0x40,OLED_CMD);//--set start line address  
	OLED_WR_Byte_i2c(0xB0,OLED_CMD);//--set page address
	OLED_WR_Byte_i2c(0x81,OLED_CMD); // contract control
	OLED_WR_Byte_i2c(0xFF,OLED_CMD);//--128   
	OLED_WR_Byte_i2c(0xA1,OLED_CMD);//set segment remap 
	OLED_WR_Byte_i2c(0xA6,OLED_CMD);//--normal / reverse
	OLED_WR_Byte_i2c(0xA8,OLED_CMD);//--set multiplex ratio(1 to 64)
	OLED_WR_Byte_i2c(0x3F,OLED_CMD);//--1/32 duty
	OLED_WR_Byte_i2c(0xC8,OLED_CMD);//Com scan direction
	OLED_WR_Byte_i2c(0xD3,OLED_CMD);//-set display offset
	OLED_WR_Byte_i2c(0x00,OLED_CMD);//
	
	OLED_WR_Byte_i2c(0xD5,OLED_CMD);//set osc division
	OLED_WR_Byte_i2c(0x80,OLED_CMD);//
	
	OLED_WR_Byte_i2c(0xD8,OLED_CMD);//set area color mode off
	OLED_WR_Byte_i2c(0x05,OLED_CMD);//
	
	OLED_WR_Byte_i2c(0xD9,OLED_CMD);//Set Pre-Charge Period
	OLED_WR_Byte_i2c(0xF1,OLED_CMD);//
	
	OLED_WR_Byte_i2c(0xDA,OLED_CMD);//set com pin configuartion
	OLED_WR_Byte_i2c(0x12,OLED_CMD);//
	
	OLED_WR_Byte_i2c(0xDB,OLED_CMD);//set Vcomh
	OLED_WR_Byte_i2c(0x30,OLED_CMD);//
	
	OLED_WR_Byte_i2c(0x8D,OLED_CMD);//set charge pump enable
	OLED_WR_Byte_i2c(0x14,OLED_CMD);//
	
	OLED_WR_Byte_i2c(0xAF,OLED_CMD);//--turn on oled panel

     return;
}

void OLED_Clear_i2c(void)  
{  
	unsigned char i,n;		    
	for(i=0;i<8;i++)  
	{  
		OLED_WR_Byte_i2c (0xb0+i,OLED_CMD);
		OLED_WR_Byte_i2c (0x02,OLED_CMD);
		OLED_WR_Byte_i2c (0x10,OLED_CMD); 
		for(n=0;n<128;n++)OLED_WR_Byte_i2c(0,OLED_DATA); 
	}
}

void OLED_On_i2c(void)  
{  
	unsigned char i,n;		    
	for(i=0;i<8;i++)  
	{  
		OLED_WR_Byte_i2c (0xb0+i,OLED_CMD);
		OLED_WR_Byte_i2c (0x00,OLED_CMD);
		OLED_WR_Byte_i2c (0x10,OLED_CMD);
		for(n=0;n<128;n++)OLED_WR_Byte_i2c(1,OLED_DATA); 
	}
}

//开启OLED显示    
void OLED_Display_On_i2c(void)
{
	OLED_WR_Byte_i2c(0X8D,OLED_CMD);  //SET DCDC命令
	OLED_WR_Byte_i2c(0X14,OLED_CMD);  //DCDC ON
	OLED_WR_Byte_i2c(0XAF,OLED_CMD);  //DISPLAY ON
}
//关闭OLED显示     
void OLED_Display_Off_i2c(void)
{
	OLED_WR_Byte_i2c(0X8D,OLED_CMD);  //SET DCDC命令
	OLED_WR_Byte_i2c(0X10,OLED_CMD);  //DCDC OFF
	OLED_WR_Byte_i2c(0XAE,OLED_CMD);  //DISPLAY OFF
}	

void OLED_Set_Pos_i2c(unsigned char x, unsigned char y) 
{ 	
     OLED_WR_Byte_i2c(0xb0+y,OLED_CMD);
     OLED_WR_Byte_i2c(((x&0xf0)>>4)|0x10,OLED_CMD);
     OLED_WR_Byte_i2c((x&0x0f),OLED_CMD); 
}

unsigned int oled_pow_i2c(unsigned char m,unsigned char n)
{
	unsigned int result=1;	 
	while(n--)result*=m;    
	return result;
}

void OLED_ShowChar_i2c(unsigned char x,unsigned char y,unsigned char chr,unsigned char Char_Size)
{      	
	unsigned char c=0,i=0;	
		c=chr-' ';			
		if(x>Max_Column-1){x=0;y=y+2;}
		if(Char_Size ==16)
		{
			OLED_Set_Pos_i2c(x,y);	
			for(i=0;i<8;i++)
			OLED_WR_Byte_i2c(F8X16_i2c[c*16+i],OLED_DATA);
			OLED_Set_Pos_i2c(x,y+1);
			for(i=0;i<8;i++)
			OLED_WR_Byte_i2c(F8X16_i2c[c*16+i+8],OLED_DATA);
		}
		else {	
			OLED_Set_Pos_i2c(x,y);
			for(i=0;i<6;i++)
			OLED_WR_Byte_i2c(F6x8_i2c[c][i],OLED_DATA);
				
		}
}

void OLED_ShowNum_i2c(unsigned char x,unsigned char y,unsigned int num,unsigned char len,unsigned char size2)
{         	
	unsigned char t,temp;
	unsigned char enshow=0;						   
	for(t=0;t<len;t++)
	{
		temp=(num/oled_pow_i2c(10,len-t-1))%10;
		if(enshow==0&&t<(len-1))
		{
			if(temp==0)
			{
				OLED_ShowChar_i2c(x+(size2/2)*t,y,' ',size2);
				continue;
			}else enshow=1; 
		 	 
		}
                if(0==t)
                {
                    OLED_ShowChar_i2c(x+(size2/2)*t-1,y,temp+'0',size2); 
                }
                else if(1==t)
                {
                    OLED_ShowChar_i2c(x+(size2/2)*t+1,y,temp+'0',size2); 
                }
                else if(2==t)
                {
                    OLED_ShowChar_i2c(x+(size2/2)*t+3,y,temp+'0',size2); 
                }
                else
                {
                    OLED_ShowChar_i2c(x+(size2/2)*t+5,y,temp+'0',size2); 
                }
             
	}
}

void OLED_ShowString_i2c(unsigned char x,unsigned char y,unsigned char *chr,unsigned char Char_Size)
{
	unsigned char j=0;
	while (chr[j]!='\0')
	{		
              OLED_ShowChar_i2c(x,y,chr[j],Char_Size);
			x+=8;
	      
              if(x>120){x=0;y+=2;}
			j++;
	}
}

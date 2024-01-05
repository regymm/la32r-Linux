#ifndef __oled_h__
#define __oled_h__

#define OLED_CMD  0	//写命令
#define OLED_DATA 1	//写数据

#define OLED_MODE 0
#define SIZE 8
#define XLevelL		0x00
#define XLevelH		0x10
#define Max_Column	128
#define Max_Row		64
#define	Brightness	0xFF 
#define X_WIDTH 	128
#define Y_WIDTH 	64

extern void init_i2c_oled(unsigned char i2c_num);
extern void OLED_Clear_i2c(void);
extern void OLED_On_i2c(void);
extern void OLED_Display_On_i2c(void);
extern void OLED_Display_Off_i2c(void);
extern void OLED_Set_Pos_i2c(unsigned char x, unsigned char y);
extern unsigned int oled_pow_i2c(unsigned char m,unsigned char n);
extern void OLED_ShowChar_i2c(unsigned char x,unsigned char y,unsigned char chr,unsigned char Char_Size);
extern void OLED_ShowNum_i2c(unsigned char x,unsigned char y,unsigned int num,unsigned char len,unsigned char size2);
extern void OLED_ShowString_i2c(unsigned char x,unsigned char y,unsigned char *chr,unsigned char Char_Size);

#endif

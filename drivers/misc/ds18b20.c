#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/gpio.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_gpio.h>
#include <linux/platform_device.h>
#include <linux/miscdevice.h>
#include <asm/uaccess.h>
#include <asm/io.h>

#define MYDEV_MINOR		203		/* 主设备号 */
#define MYDEV_NAME		"ds18b" 	/* 设备名字 */

#define GPIO_OE_BASE				(0x1FD010D0)
#define GPIO_IN_BASE				(0x1FD010E0)
#define GPIO_OUT_BASE				(0x1FD010F0)

static void __iomem *GPIO_OE_V;
static void __iomem *GPIO_IN_V;
static void __iomem *GPIO_OUT_V;

static void DS18B20_INPUT(void)  
{
      u8 val;
 
      val = readb(GPIO_OE_V);
      //val |= 0x40;
      val |= (1 << 6);
      writeb(val, GPIO_OE_V);
}

static void DS18B20_OUTPUT(void)  
{
      u8 val;
 
      val = readb(GPIO_OE_V);
      //val &= 0xbf;
      val &= ~(1 << 6);
      writeb(val, GPIO_OE_V);
}

/*
函数功能：等待DS18B20的回应
返回1:未检测到DS18B20的存在
返回0:存在
*/
unsigned char DS18B20_Check(void) 	   
{   
	unsigned char retry=0;
        u8 val;

	DS18B20_INPUT(); ///SET GPIO6 INPUT
        
        val = readb(GPIO_IN_V);
        while((val & (1 << 6)) && (retry<200))
	{
		retry++;
		udelay(1);
                val = readb(GPIO_IN_V);
	};

        //pr_info("DS18B20_Check===>val:%d,retry:%d\r\n",(val & (1 << 6)),retry);
	if(retry>=200) return 1;
	else retry=0;

        val = readb(GPIO_IN_V);
        while(!(val & (1 << 6)) && (retry<240))
	{
		retry++;
		udelay(1);
                val = readb(GPIO_IN_V);
	};
         
        //pr_info("DS18B20_Check2===>val:%d,retry:%d\r\n",(val & (1 << 6)),retry);
	if(retry>=240) return 1;	    
	return 0;
}

/*
从DS18B20读取一个位
返回值：1/0
*/
unsigned char DS18B20_Read_Bit(void) 			 // read one bit
{
    unsigned char data;
    u8 val;

    DS18B20_OUTPUT();

    val = readb(GPIO_OUT_V);
    val &= ~(1 << 6);
    writeb(val, GPIO_OUT_V);
	
     udelay(2);
     val = readb(GPIO_OUT_V);
     val |= (1 << 6);
     writeb(val, GPIO_OUT_V);
	
    DS18B20_INPUT();
    udelay(12);
    
    val = readb(GPIO_IN_V);
    if((val & (1 << 6)))
       data=1;
    else 
       data=0;	 
    
    udelay(50);           
    
    return data;
}

/*
从DS18B20读取一个字节
返回值：读到的数据
*/
unsigned char DS18B20_Read_Byte(void)    // read one byte
{        
    unsigned char i,j,dat;
    dat=0;
	
    for(i=1;i<=8;i++) 
    {
        j=DS18B20_Read_Bit();
	dat=dat>>1;
	if(j)        //主机对总线采样的数 判断-------读数据-1就是1，否则就是0 
	  dat|=0x80;   //先收低位数据--一步一步向低位移动>>
    }						    
    
    return dat;
}

/*
写一个字节到DS18B20
dat：要写入的字节
*/
void DS18B20_Write_Byte(unsigned char dat)     
 {             
    unsigned char j;
    unsigned char testb;
    u8 val;

    DS18B20_OUTPUT();
    for(j=1;j<=8;j++) 
    {
        testb = dat & 0x01;
        dat = dat>>1;
        if(testb) 
        {
            val = readb(GPIO_OUT_V);
            val &= ~(1 << 6);
            writeb(val, GPIO_OUT_V);  //输出0
            udelay(2); 
            val = readb(GPIO_OUT_V); 
            val |= (1 << 6);                          
            writeb(val, GPIO_OUT_V); //输出1
            udelay(60);             
        }
        else 
        {
            val = readb(GPIO_OUT_V);
            val &= ~(1 << 6);
            writeb(val, GPIO_OUT_V);  //输出0
            udelay(60);        
            val = readb(GPIO_OUT_V);     
            val |= (1 << 6);                          
            writeb(val, GPIO_OUT_V); //输出1
            udelay(2);                          
        }
    }
}

/*
从ds18b20得到温度值
精度：0.1C
返回值：温度值 （-550~1250） 
*/
short DS18B20_Get_Temp(void)
{
    unsigned short aaa;
    unsigned char temp;
    unsigned char TL,TH;
    u8 val,ret;

    DS18B20_OUTPUT(); 
    val = readb(GPIO_OUT_V);
    val &= ~(1 << 6);
    writeb(val, GPIO_OUT_V);  //输出0 //拉低DQ

    udelay(750);    //拉低750us
    val = readb(GPIO_OUT_V);
    val |= (1 << 6);                          
    writeb(val, GPIO_OUT_V); //输出1 //DQ=1 
    udelay(15);     //15US	  
	
    ret = DS18B20_Check();
    //pr_info("DS18B20_Check:%d\r\n",ret);
    if(0!=ret)
        return -2;	 
    DS18B20_Write_Byte(0xcc);
    DS18B20_Write_Byte(0x44);
	
    DS18B20_OUTPUT(); 
    val = readb(GPIO_OUT_V);
    val &= ~(1 << 6);
    writeb(val, GPIO_OUT_V);  //输出0 //拉低DQ

    udelay(750);    //拉低750us
    val = readb(GPIO_OUT_V);
    val |= (1 << 6);                          
    writeb(val, GPIO_OUT_V); //输出1 //DQ=1 
    udelay(15);     //15US
	
    ret = DS18B20_Check();	
    //pr_info("DS18B20_Check2:%d\r\n",ret);
    if(0!=ret)
        return -3; 
    DS18B20_Write_Byte(0xcc);// skip rom
    DS18B20_Write_Byte(0xbe);// convert	    
    TL=DS18B20_Read_Byte();  // LSB   
    TH=DS18B20_Read_Byte();  // MSB  
    //pr_info("DS18B20_Get_Temp===>TL:0x%x,TH:0x%x\r\n",TL,TH);
    aaa=((unsigned short)TH<<8)|TL;
	
    return aaa;
}

static int ds18b_open(struct inode *inode, struct file *filp)
{
	return 0;
}

static ssize_t ds18b_read(struct file *filp, char __user *buf, size_t cnt, loff_t *offt)
{
         /*读取温度信息*/
	short temp=DS18B20_Get_Temp();
	copy_to_user(buf,&temp,2);    //拷贝温度至应用层 
	
        return 0;
}

static ssize_t ds18b_write(struct file *filp, const char __user *buf, size_t cnt, loff_t *offt)
{
        return 0;
}

static int ds18b_release(struct inode *inode, struct file *filp)
{
	return 0;
}

/* 设备操作函数 */
static struct file_operations ds18b20_fops = {
	.owner = THIS_MODULE,
	.open = ds18b_open,
	.read = ds18b_read,
	.write = ds18b_write,
	.release = ds18b_release,
};

/* MISC设备结构体 */
static struct miscdevice ds18b20_miscdev = {
	.minor = MYDEV_MINOR,
	.name = MYDEV_NAME,
	.fops = &ds18b20_fops,
};

static int __init ds18b20_drv_init(void)
{
       int ret = 0;
       u8 val,val2;

       GPIO_OE_V = ioremap(GPIO_OE_BASE, 1);
       GPIO_IN_V = ioremap(GPIO_IN_BASE, 1);
       GPIO_OUT_V = ioremap(GPIO_OUT_BASE, 1);

       val &= 0xbf;
       writeb(val, GPIO_OE_V);  

       ret = misc_register(&ds18b20_miscdev);
       if(ret < 0){
	   pr_info("misc device register failed!\r\n");
	   return -EFAULT;
       }

       return 0;
}

static void __exit ds18b20_drv_exit(void)
{
       iounmap(GPIO_OE_V);
       iounmap(GPIO_IN_V);
       iounmap(GPIO_OUT_V);

       return;
}

module_init(ds18b20_drv_init);
module_exit(ds18b20_drv_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("wugang");


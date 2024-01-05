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
#include "bx_i2c.h"
#include "oled_i2c.h"

/*
 * I2C各个寄存器相对基地址的偏移
 * 发送数据寄存器和接收数据寄存器的偏移是相同的
 * 命令寄存器和状态寄存器的偏移是相同的，不同的是命令寄存器只写，状态寄存器只读
 */
#define BX_I2C_PRER_LOW_OFFSET            (0)     // 分频锁存器低字节寄存器偏移
#define BX_I2C_PRER_HIGH_OFFSET           (1)     // 分频锁存器高字节寄存器偏移
#define BX_I2C_CONTROL_OFFSET             (2)     // 控制寄存器偏移
#define BX_I2C_DATA_OFFSET                (3)     // 发送数据寄存器和接收数据寄存器的偏移是相同的
#define BX_I2C_CMD_OFFSET                 (4)     // 命令寄存器偏移，只写
#define BX_I2C_STATUS_OFFSET              (4)     // 状态寄存器偏移，只读


// 控制寄存器的位域
#define BX_I2C_CONTROL_EN                 (0x80)  // i2c模块使能
#define BX_I2C_CONTROL_IEN                (0x40)  // 中断使能

// 命令寄存器的位域
#define BX_I2C_CMD_START                  (0x90)  // 产生START信号
#define BX_I2C_CMD_STOP                   (0x40)  // 产生STOP信号
#define BX_I2C_CMD_READ                   (0x20)  // 产生读信号，即产生ACK信号
#define BX_I2C_CMD_WRITE                  (0x10)  // 产生写信号
#define BX_I2C_CMD_READ_ACK               (0x20)  // 产生ACK信号，与读信号相同
#define BX_I2C_CMD_READ_NACK              (0x28)  // 产生NACK信号
#define BX_I2C_CMD_IACK                   (0x00)  // 产生中断应答信号

// 状态寄存器的位域
#define BX_I2C_STATUS_IF                  (0x01)  // 中断标志位
#define BX_I2C_STATUS_TIP                 (0x02)  // 指示传输的过程。1，正在传输；0，传输完成
#define BX_I2C_STATUS_ARBLOST             (0x20)  // I2C核失去I2C总线的控制权
#define BX_I2C_STATUS_BUSY                (0x40)  // I2C总线忙标志
#define BX_I2C_STATUS_NACK                (0x80)  // 应答位标志。1，没收到应答位；0，收到应答位

#define MIN(a, b)           ((a) > (b) ? (b) : (a))

#define MY_I2C_MINOR		201		/* 主设备号 */
#define MY_I2C_NAME		"myi2c" 	/* 设备名字 */
#define BX_START_FREQ                     (0xbfe78030)
#define AHB_CLK                 (24000000)
#define APB_CLK                 (AHB_CLK)
#define DDR_RATE                (33000000)

#define  MAGIC_NUMBER    'l'
#define CMD_INIT_OLED _IO(MAGIC_NUMBER    ,1)
#define CMD_OLED_SHOWSTR _IO(MAGIC_NUMBER    ,2)
#define CMD_OLED_CLEAR _IO(MAGIC_NUMBER    ,3)

static void __iomem *I2C0_BASE_V;
static void __iomem *I2C1_BASE_V;
static void __iomem *I2C2_BASE_V;
static void __iomem *I2C3_BASE_V;
static void __iomem *SHUT_CTRL_BASE_V;

int slave_addr = 0x78 >> 1;
bx_i2c_info_t i2c_info;

void i2c_init(bx_i2c_info_t *i2c_info_p);
bx_i2c_ack_t i2c_receive_ack(bx_i2c_info_t *i2c_info_p);
bx_i2c_ret_t i2c_receive_data(bx_i2c_info_t *i2c_info_p, unsigned char *buf, int len);
bx_i2c_ret_t i2c_send_start_and_addr(bx_i2c_info_t *i2c_info_p, 
                                       unsigned char slave_addr,
                                       bx_i2c_direction_t direction);
bx_i2c_ret_t i2c_send_data(bx_i2c_info_t *i2c_info_p, unsigned char *data, int len);
void i2c_send_stop(bx_i2c_info_t *i2c_info_p);


static unsigned long clk_get_apb_rate(void)
{
    return DDR_RATE;
}
/*
 * 获取指定i2c模块的基地址
 * @I2Cx I2C模块的编号
 */
static void *i2c_get_base(bx_i2c_t I2Cx)
{
    void *base = NULL;
    
    switch (I2Cx)
    {
        case BX_I2C_0:
            base = (void *)I2C0_BASE_V;
            break;

        case BX_I2C_1:
            base = (void *)I2C1_BASE_V;
            break;

        case BX_I2C_2:
            base = (void *)I2C2_BASE_V;
            break;

        case BX_I2C_3:
            base = (void *)I2C3_BASE_V;
            break;

        default:
            base = NULL;
            break;
    }

    return base;
}

/*
 * 向命令寄存器写命令
 * @i2c_info_p i2c模块信息
 * @cmd 命令
 */
static void i2c_cmd(bx_i2c_info_t *i2c_info_p, unsigned char cmd)
{
    void *i2c_base = i2c_get_base(i2c_info_p->I2Cx);

    //reg_write_8(cmd, i2c_base + BX_I2C_CMD_OFFSET);
    writeb(cmd, i2c_base + BX_I2C_CMD_OFFSET);

    return ;
}

/*
 * 执行START命令，发送START信号
 * @i2c_info_p i2c模块信息
 */
static void i2c_cmd_start(bx_i2c_info_t *i2c_info_p)
{
    i2c_cmd(i2c_info_p, BX_I2C_CMD_START);
    return ;
}

/*
 * 执行STOP命令，发送STOP信号
 * @i2c_info_p i2c模块信息
 */
static void i2c_cmd_stop(bx_i2c_info_t *i2c_info_p)
{
    i2c_cmd(i2c_info_p, BX_I2C_CMD_STOP);
    return ;
}

/*
 * 执行写命令
 * @i2c_info_p i2c模块信息
 */
static void i2c_cmd_write(bx_i2c_info_t *i2c_info_p)
{
    i2c_cmd(i2c_info_p, BX_I2C_CMD_WRITE);
    return ;
}

/*
 * 执行读ack命令，发送读ack信号
 * @i2c_info_p i2c模块信息
 */
void i2c_cmd_read_ack(bx_i2c_info_t *i2c_info_p)
{
    i2c_cmd(i2c_info_p, BX_I2C_CMD_READ_ACK);
    return ;
}

/*
 * 执行读nack命令,发送读nack信号
 * @i2c_info_p i2c模块信息
 */
static void i2c_cmd_read_nack(bx_i2c_info_t *i2c_info_p)
{
    i2c_cmd(i2c_info_p, BX_I2C_CMD_READ_NACK);
    return ;
}

/*
 * 发送中断应答信号
 * @i2c_info_p i2c模块信息
 */
static void i2c_cmd_iack(bx_i2c_info_t *i2c_info_p)
{
    i2c_cmd(i2c_info_p, BX_I2C_CMD_IACK);
    return ;
}

/*
 * 获取状态寄存器的值
 * @i2c_info_p i2c模块信息
 * @ret 状态寄存器的值
 */
static unsigned char i2c_get_status(bx_i2c_info_t *i2c_info_p)
{
    void *i2c_base = i2c_get_base(i2c_info_p->I2Cx);

    //return reg_read_8(i2c_base + BX_I2C_STATUS_OFFSET);
    return readb(i2c_base + BX_I2C_STATUS_OFFSET);
}

/*
 * Poll the i2c status register until the specified bit is set.
 * Returns 0 if timed out (100 msec).
 * @i2c_info_p i2c模块信息
 * @bit 寄存器的某一位
 * @ret true or false
 */
static int i2c_poll_status(bx_i2c_info_t *i2c_info_p, unsigned long bit)
{
    int loop_cntr = 20000;

    do {
        //delay_us(1);
        udelay(1);
    } while ((i2c_get_status(i2c_info_p) & bit) && (0 < --loop_cntr));

    return (0 < loop_cntr);
}

/*
 * 初始化指定i2c模块
 * @i2c_info_p 某个i2c模块的信息
 */
void i2c_init(bx_i2c_info_t *i2c_info_p)
{
    void *i2c_base = i2c_get_base(i2c_info_p->I2Cx);
    unsigned long i2c_clock = i2c_info_p->clock;
    //unsigned char ctrl = reg_read_8(i2c_base + LS1C_I2C_CONTROL_OFFSET);
    unsigned char ctrl = readb(i2c_base + BX_I2C_CONTROL_OFFSET);
    unsigned long prescale = 0;

    /* make sure the device is disabled */
    ctrl = ctrl & ~(BX_I2C_CONTROL_EN | BX_I2C_CONTROL_IEN);
    //reg_write_8(ctrl, i2c_base + LS1C_I2C_CONTROL_OFFSET);
    writeb(ctrl, i2c_base + BX_I2C_CONTROL_OFFSET);

    // 设置时钟
    i2c_clock = MIN(i2c_clock, BX_I2C_CLOCK_MAX);     // 限制在最大允许范围内
    prescale = clk_get_apb_rate();
    prescale = (prescale / (5 * i2c_clock)) - 1;
    pr_info("i2c_init===>prescale:%d\r\n",prescale);
    //reg_write_8(prescale & 0xff, i2c_base + LS1C_I2C_PRER_LOW_OFFSET);
    //reg_write_8(prescale >> 8, i2c_base + LS1C_I2C_PRER_HIGH_OFFSET);
    writeb(prescale & 0xff, i2c_base + BX_I2C_PRER_LOW_OFFSET);
    writeb(prescale >> 8, i2c_base + BX_I2C_PRER_HIGH_OFFSET);
    
    // 使能
    i2c_cmd_iack(i2c_info_p);
    ctrl = ctrl | BX_I2C_CONTROL_EN;
    //reg_write_8(ctrl, i2c_base + LS1C_I2C_CONTROL_OFFSET);
    writeb(ctrl, i2c_base + BX_I2C_CONTROL_OFFSET);

    return ;
}

/*
 * (再发送一个字节数据之后)接收从机发送的ACK信号
 * @i2c_info_p i2c模块信息
 * @ret LS1C_I2C_ACK or LS1C_I2C_NACK
 */
bx_i2c_ack_t i2c_receive_ack(bx_i2c_info_t *i2c_info_p)
{
    bx_i2c_ack_t ret = BX_I2C_NACK;
    
    if (BX_I2C_STATUS_NACK & i2c_get_status(i2c_info_p))
    {
        ret = BX_I2C_NACK;
    }
    else
    {
        ret = BX_I2C_ACK;
    }

    return ret;
}

/*
 * 接收数据
 * @i2c_info_p i2c模块信息
 * @buf 数据缓存
 * @len 待接收数据的长度
 */
bx_i2c_ret_t i2c_receive_data(bx_i2c_info_t *i2c_info_p, unsigned char *buf, int len)
{
    void *i2c_base = i2c_get_base(i2c_info_p->I2Cx);
    int i = 0;

    for (i=0; i<len; i++)
    {
        // 开始接收
        if (i != (len - 1))
            i2c_cmd_read_ack(i2c_info_p);
        else 
            i2c_cmd_read_nack(i2c_info_p);

        // 等待，直到接收完成
        if (!i2c_poll_status(i2c_info_p, BX_I2C_STATUS_TIP))
            return BX_I2C_RET_TIMEOUT;

        // 读取数据，并保存
        //*buf++ = reg_read_8(i2c_base + LS1C_I2C_DATA_OFFSET);
        *buf++ = readb(i2c_base + BX_I2C_DATA_OFFSET);
    }

    return BX_I2C_RET_OK;
}

/*
 * 发送START信号和地址
 * @i2c_info_p i2c模块信息
 * @slave_addr 从机地址
 * @direction 数据传输方向(读、写)
 */
bx_i2c_ret_t i2c_send_start_and_addr(bx_i2c_info_t *i2c_info_p, 
                                       unsigned char slave_addr,
                                       bx_i2c_direction_t direction)
{
    void *i2c_base = i2c_get_base(i2c_info_p->I2Cx);
    unsigned char data = 0;
    
    // 等待i2c总线空闲
    if (!i2c_poll_status(i2c_info_p, BX_I2C_STATUS_BUSY))
        return BX_I2C_RET_TIMEOUT;

    // 填充地址到数据寄存器
    data = (slave_addr << 1) | ((BX_I2C_DIRECTION_READ == direction) ? 1 : 0);
    //reg_write_8(data , i2c_base + LS1C_I2C_DATA_OFFSET);
    writeb(data , i2c_base + BX_I2C_DATA_OFFSET);

    // 开始发送
    i2c_cmd_start(i2c_info_p);

    // 等待，直到发送完成
    if (!i2c_poll_status(i2c_info_p, BX_I2C_STATUS_TIP))
        return BX_I2C_RET_TIMEOUT;

    return BX_I2C_RET_OK;
}

/*
 * 发送数据
 * @i2c_info_p i2c模块信息
 * @data 待发送的数据
 * @len 待发送数据的长度
 */
bx_i2c_ret_t i2c_send_data(bx_i2c_info_t *i2c_info_p, unsigned char *data, int len)
{
    void *i2c_base = i2c_get_base(i2c_info_p->I2Cx);
    int i = 0;

    for (i=0; i<len; i++)
    {
        // 将一个字节数据写入数据寄存器
        //reg_write_8(*data++, i2c_base + LS1C_I2C_DATA_OFFSET);
        writeb(*data++, i2c_base + BX_I2C_DATA_OFFSET);

        // 开始发送
        //reg_write_8(LS1C_I2C_CMD_WRITE, i2c_base + LS1C_I2C_CMD_OFFSET);
        writeb(BX_I2C_CMD_WRITE, i2c_base + BX_I2C_CMD_OFFSET);

        // 等待，直到发送完成
        if (!i2c_poll_status(i2c_info_p, BX_I2C_STATUS_TIP))
            return BX_I2C_RET_TIMEOUT;

        // 读取应答信号
        if (BX_I2C_ACK != i2c_receive_ack(i2c_info_p))
            return len;
    }

    return BX_I2C_RET_OK;
}

/*
 * 发送STOP信号
 * @i2c_info_p i2c模块信息
 */
void i2c_send_stop(bx_i2c_info_t *i2c_info_p)
{
    i2c_cmd_stop(i2c_info_p);
    return ;
}

static int myi2c_open(struct inode *inode, struct file *filp)
{
	return 0;
}

static int myi2c_release(struct inode *inode, struct file *filp)
{
	return 0;
}

static long myi2c_ioctl(struct file *file, unsigned int cmd, unsigned long arg) 
{
       unsigned char databuf[20];
       int ret = -EINVAL,i;
       void __user *data = (void __user *)arg;
       unsigned char x,y,*pstr,i2c_num;

       memset(databuf,0,sizeof(databuf));
       if(cmd==CMD_OLED_SHOWSTR) {
           if(copy_from_user(&databuf, arg, sizeof(databuf))) {
		ret = -EFAULT;
		goto L;
	   }

           x = databuf[0];
           y = databuf[1];
           pstr = &databuf[2];

           pr_info("mygpio_ioctl===>cmd:%x,databuf:%x-%x-%s\r\n", cmd,databuf[0],databuf[1],pstr);
       }
       else if(cmd==CMD_INIT_OLED) {
           if(copy_from_user(&i2c_num, arg, 1)) {
		ret = -EFAULT;
		goto L;
	   }
           pr_info("mygpio_ioctl===>cmd:%x,i2c_num:%d\r\n", cmd,i2c_num);
       }
       else if(cmd==CMD_OLED_CLEAR) {
           pr_info("mygpio_ioctl===>cmd:%x\r\n", cmd);
       }

       switch(cmd) {
           case CMD_INIT_OLED:
                 init_i2c_oled(i2c_num);
                break;
           case CMD_OLED_SHOWSTR:
                 OLED_ShowString_i2c(x,y,pstr,8);
                break;
           case CMD_OLED_CLEAR:
                 OLED_Clear_i2c();
           default:
                break;
       };

L:
       return ret;
}


/* 设备操作函数 */
static struct file_operations myi2c_fops = {
	.owner = THIS_MODULE,
	.open = myi2c_open,
	.release = myi2c_release,
        .unlocked_ioctl = myi2c_ioctl,
};

/* MISC设备结构体 */
static struct miscdevice myi2c_miscdev = {
	.minor = MY_I2C_MINOR,
	.name = MY_I2C_NAME,
	.fops = &myi2c_fops,
};

static int __init i2c_drv_init(void)
{
       int ret = 0;
       u32 val;

       I2C0_BASE_V = ioremap(BX_I2C0_BASE, 4);
       I2C1_BASE_V = ioremap(BX_I2C1_BASE, 4);
       I2C2_BASE_V = ioremap(BX_I2C2_BASE, 4);
       I2C3_BASE_V = ioremap(BX_I2C3_BASE, 4);
       SHUT_CTRL_BASE_V = ioremap(SHUT_CTRL_BASE,4);
       pr_info("i2c_drv_init===>I2C1_BASE_V:0x%x,I2C2_BASE_V:0x%x\r\n",I2C1_BASE_V,I2C2_BASE_V);

       val = readb(SHUT_CTRL_BASE_V);
       pr_info("i2c_drv_init===>SHUT_CTRL_BASE_V:0x%x,SHUT_CTRL:0x%x\r\n",SHUT_CTRL_BASE_V,val);

       ret = misc_register(&myi2c_miscdev);
       if(ret < 0){
	   pr_info("misc device register failed!\r\n");
	   return -EFAULT;
       }

       return 0;
}

static void __exit i2c_drv_exit(void)
{
       iounmap(I2C0_BASE_V);
       iounmap(I2C1_BASE_V);
       iounmap(I2C2_BASE_V);
       iounmap(I2C3_BASE_V);

       return;
}

module_init(i2c_drv_init);
module_exit(i2c_drv_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("wugang");


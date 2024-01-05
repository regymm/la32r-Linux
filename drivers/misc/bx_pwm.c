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
#include "bx_pwm.h"

#define MY_PWM_MINOR		202		/* 主设备号 */
#define MY_PWM_NAME		"mypwm" 	/* 设备名字 */

#define MIN(a, b)           ((a) > (b) ? (b) : (a))

// pwm的最大周期
#define PWM_MAX_PERIOD                  (0xFFFFFF)      // 计数器的值
#define DDR_RATE                (33000000)

#define  MAGIC_NUMBER    'y'
#define CMD_PWM_PULSE _IO(MAGIC_NUMBER    ,1)

static void __iomem *PWM0_BASE_V;
static void __iomem *PWM1_BASE_V;
static void __iomem *PWM2_BASE_V;
static void __iomem *PWM3_BASE_V;
static void __iomem *PWM4_BASE_V;
static void __iomem *PWM5_BASE_V;
static void __iomem *PWM6_BASE_V;
static void __iomem *PWM7_BASE_V;
static void __iomem *PWM8_BASE_V;
static void __iomem *PWM9_BASE_V;

static unsigned long clk_get_apb_rate(void)
{
    return DDR_RATE;
}

static unsigned int pwm_get_reg_base(unsigned int pwm_num)
{
    unsigned int reg_base = 0;
    
    switch (pwm_num)
    {
        case 0:
            reg_base = PWM0_BASE_V;
            break;
        case 1:
            reg_base = PWM1_BASE_V;
            break;
        case 2:
            reg_base = PWM2_BASE_V;
            break;
        case 3:
            reg_base = PWM3_BASE_V;
            break;
        case 4:
            reg_base = PWM4_BASE_V;
            break;
        case 5:
            reg_base = PWM5_BASE_V;
            break;
        case 6:
            reg_base = PWM6_BASE_V;
            break;
        case 7:
            reg_base = PWM7_BASE_V;
            break;
        case 8:
            reg_base = PWM8_BASE_V;
            break;
        case 9:
            reg_base = PWM9_BASE_V;
            break;
    }

    return reg_base;
}

/*
 * 禁止pwm
 * @pwm_info PWMn的详细信息
 */
void pwm_disable(pwm_info_t *pwm_info)
{
    unsigned int pwm_reg_base = 0;
    
    // 检查入参
    if (NULL == pwm_info)
    {
        return ;
    }

    pwm_reg_base = pwm_get_reg_base(pwm_info->num);
    //reg_write_32(0, (volatile unsigned int *)(pwm_reg_base + LS1C_PWM_CTRL));
    writel(0, (volatile unsigned int *)(pwm_reg_base + BX_PWM_CTRL));

    return ;
}

/*
 * 使能PWM
 * @pwm_info PWMn的详细信息
 */
void pwm_enable(pwm_info_t *pwm_info)
{
    unsigned int pwm_reg_base = 0;
    unsigned int ctrl = 0;

    // 检查入参
    if (NULL == pwm_info)
    {
        return ;
    }

    // 获取基地址
    pwm_reg_base = pwm_get_reg_base(pwm_info->num);

    // 清零计数器
    //reg_write_32(0, (volatile unsigned int *)(pwm_reg_base + LS1C_PWM_CNTR));
    writel(0, (volatile unsigned int *)(pwm_reg_base + BX_PWM_CNTR));

    // 设置控制寄存器
    ctrl = (0 << BX_PWM_INT_LRC_EN)
           | (0 << BX_PWM_INT_HRC_EN)
           | (0 << BX_PWM_CNTR_RST)
           | (0 << BX_PWM_INT_SR)
           | (0 << BX_PWM_INTEN)
           | (0 << BX_PWM_OE)
           | (1 << BX_PWM_CNT_EN);
    if (PWM_MODE_PULSE == pwm_info->mode)     // 单脉冲
    {
        ctrl |= (1 << BX_PWM_SINGLE);
    }
    else                            // 连续脉冲
    {
        ctrl &= ~(1 << BX_PWM_SINGLE);
    }
    //reg_write_32(ctrl, (volatile unsigned int *)(pwm_reg_base + LS1C_PWM_CTRL));
    writel(ctrl, (volatile unsigned int *)(pwm_reg_base + BX_PWM_CTRL));
    pr_info("pwm_enable===>ctrl:0x%x\r\n",ctrl);
    
    return ;
}

/*
 * 初始化PWMn
 * @pwm_info PWMn的详细信息
 */

void pwm_init(pwm_info_t *pwm_info)
{
    unsigned int gpio;
    unsigned long pwm_clk = 0;          // pwm模块的时钟频率
    unsigned long tmp = 0;
    unsigned int pwm_reg_base = 0;
    unsigned long period = 0;
    
    // 判断入参
    if (NULL == pwm_info)
    {
        // 入参非法，则直接返回
        return ;
    }
    
    // 根据占空比和pwm周期计算寄存器HRC和LRC的值
    // 两个64位数相乘，只能得到低32位，linux下却可以得到64位结果，
    // 暂不清楚原因，用浮点运算代替
    pwm_clk = clk_get_apb_rate();
    //period = (long)(pwm_clk * pwm_info->period_ns) / (1000000000L);
    period = pwm_clk/50;
    pr_info("pwm_init===>period1:%ld,pwm_clk * pwm_info->period_ns:%ld\r\n",period,pwm_clk * pwm_info->period_ns);
    period = MIN(period, PWM_MAX_PERIOD);       // 限制周期不能超过最大值
    //tmp = period - (period * pwm_info->duty);
    tmp = (pwm_info->duty2-pwm_info->duty1)*period/pwm_info->duty2;
    pr_info("pwm_init===>tmp:%ld,period2:%ld\r\n",tmp,period);
    
    // 写寄存器HRC和LRC
    pwm_reg_base = pwm_get_reg_base(pwm_info->num);
    //reg_write_32(--tmp, (volatile unsigned int *)(pwm_reg_base + BX_PWM_HRC));
    writel(--tmp, (volatile unsigned int *)(pwm_reg_base + BX_PWM_HRC));
    //reg_write_32(--period, (volatile unsigned int *)(pwm_reg_base + BX_PWM_LRC));
    writel(--period, (volatile unsigned int *)(pwm_reg_base + BX_PWM_LRC));

    // 写主计数器
    pwm_enable(pwm_info);
    
    return ;
}

static unsigned char pwm_pulse(unsigned short angle,unsigned int num)
{
    pwm_info_t pwm_info;
    //float duty;
    unsigned char duty1;
    

    if((angle<0) || (angle>180))
    {
        return (-1);
    }

    //duty = (((float)angle/180)*2)/20+0.025;
    //printf("duty:%f\r\n",duty);
    duty1 = angle/45;

    pwm_info.num = num;                           // 输出pwm波形的引脚
    pwm_info.mode = PWM_MODE_NORMAL;      //PWM_MODE_PULSE;
    pwm_info.duty1 = duty1;                           // pwm占空比
    pwm_info.duty2 = 40;
    pwm_info.period_ns = 20*1000*1000;               // pwm周期20ms

    pwm_init(&pwm_info);

    return 0;
}

static int mypwm_open(struct inode *inode, struct file *filp)
{
	return 0;
}

static int mypwm_release(struct inode *inode, struct file *filp)
{
	return 0;
}

static long mypwm_ioctl(struct file *file, unsigned int cmd, unsigned long arg) 
{
       unsigned char databuf[5];
       int ret = -EINVAL,i;
       void __user *data = (void __user *)arg;
       unsigned char num,*pAngle;
       unsigned short angle;

       memset(databuf,0,sizeof(databuf));
       if(copy_from_user(&databuf, arg, sizeof(databuf))) {
		ret = -EFAULT;
		goto L;
       }
       num = databuf[0];       
       memcpy(&angle,&databuf[1],2);
       pr_info("mypwm_ioctl===>num:%d,angle:%d\r\n",num,angle);

       switch(cmd) {
           case CMD_PWM_PULSE:
                 pwm_pulse(angle,num);
                break;
           default:
                break;
       };

L:
       return ret;
}

/* 设备操作函数 */
static struct file_operations mypwm_fops = {
	.owner = THIS_MODULE,
	.open = mypwm_open,
	.release = mypwm_release,
        .unlocked_ioctl = mypwm_ioctl,
};

/* MISC设备结构体 */
static struct miscdevice pwm_miscdev = {
	.minor = MY_PWM_MINOR,
	.name = MY_PWM_NAME,
	.fops = &mypwm_fops,
};

static int __init pwm_drv_init(void)
{
       int ret = 0;
       u32 val;

       PWM0_BASE_V = ioremap(BX_PWM0_BASE, 4);
       PWM1_BASE_V = ioremap(BX_PWM1_BASE, 4);
       PWM2_BASE_V = ioremap(BX_PWM2_BASE, 4);
       PWM3_BASE_V = ioremap(BX_PWM3_BASE, 4);
       PWM4_BASE_V = ioremap(BX_PWM4_BASE, 4);
       PWM5_BASE_V = ioremap(BX_PWM5_BASE, 4);
       PWM6_BASE_V = ioremap(BX_PWM6_BASE, 4);
       PWM7_BASE_V = ioremap(BX_PWM7_BASE, 4);
       PWM8_BASE_V = ioremap(BX_PWM8_BASE, 4);
       PWM9_BASE_V = ioremap(BX_PWM9_BASE, 4);

       ret = misc_register(&pwm_miscdev);
       if(ret < 0){
	   pr_info("misc device register failed!\r\n");
	   return -EFAULT;
       }

       return 0;
}

static void __exit pwm_drv_exit(void)
{
       iounmap(PWM0_BASE_V);
       iounmap(PWM1_BASE_V);
       iounmap(PWM2_BASE_V);
       iounmap(PWM3_BASE_V);
       iounmap(PWM4_BASE_V);
       iounmap(PWM5_BASE_V);
       iounmap(PWM6_BASE_V);
       iounmap(PWM7_BASE_V);
       iounmap(PWM8_BASE_V);
       iounmap(PWM9_BASE_V);

       return;
}

module_init(pwm_drv_init);
module_exit(pwm_drv_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("wugang");





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

#define MYGPIO_MINOR		200		/* 主设备号 */
#define MYGPIO_NAME		"gpio" 	/* 设备名字 */

#define GPIO_OE_BASE				(0x1FD010D0)
#define GPIO_IN_BASE				(0x1FD010E0)
#define GPIO_OUT_BASE				(0x1FD010F0)

#define  MAGIC_NUMBER    'k'
#define CMD_SETIO _IO(MAGIC_NUMBER    ,1)

static void __iomem *GPIO_OE;
static void __iomem *GPIO_IN;
static void __iomem *GPIO_OUT;

static int mygpio_open(struct inode *inode, struct file *filp)
{
	return 0;
}

static ssize_t mygpio_read(struct file *filp, char __user *buf, size_t cnt, loff_t *offt)
{
         u8 val,ret;

         val = readb(GPIO_IN);

         ret = copy_to_user(buf, &val, 1);

	return ret;
}

static ssize_t mygpio_write(struct file *filp, const char __user *buf, size_t cnt, loff_t *offt)
{
        int retvalue,i;
	unsigned char databuf[8];
        u8 val,val2;

	retvalue = copy_from_user(databuf, buf, cnt);
	if(retvalue < 0) {
		pr_info("kernel write failed!\r\n");
		return -EFAULT;
	}

        val = readb(GPIO_OUT);
        //pr_info("mygpio_write===>val:%x\r\n",val);

        for(i=0;i<8;i++)
        {
             if(databuf[i])
                 val &= (1<<i);
             else
                 val |= (1<<i);
        }
        writeb(val, GPIO_OUT);

        val2 = readb(GPIO_OUT);
        //pr_info("mygpio_write===>val2:%x\r\n",val2);

       return 0;
}

static int mygpio_release(struct inode *inode, struct file *filp)
{
	return 0;
}

static long mygpio_ioctl(struct file *file, unsigned int cmd, unsigned long arg) 
{
       unsigned char databuf[8];
       int ret = -EINVAL,i;
       void __user *data = (void __user *)arg;
       u8 val,val2;

       if(copy_from_user(&databuf, arg, sizeof(databuf))) {
		ret = -EFAULT;
		goto L;
	}

        pr_info("mygpio_ioctl===>databuf:%x-%x-%x-%x-%x-%x-%x-%x\r\n", \
                 databuf[0],databuf[1],databuf[2],databuf[3],databuf[4],databuf[5],databuf[6],databuf[7]);
        for(i=0;i<8;i++)
        {
             if(databuf[i])
                 val |= (1<<i);
             else
                 val = val & (~(1<<i));
        }

       switch(cmd) {
           case CMD_SETIO:
                 writeb(val, GPIO_OE);
                break;
           default:
                break;
       };

       val2 = readb(GPIO_OE);
       pr_info("mygpio_ioctl===>val2:%x\r\n",val2);

L:
       return ret;
}

/* 设备操作函数 */
static struct file_operations mygpio_fops = {
	.owner = THIS_MODULE,
	.open = mygpio_open,
	.read = mygpio_read,
	.write = mygpio_write,
	.release = mygpio_release,
        .unlocked_ioctl = mygpio_ioctl,
};

/* MISC设备结构体 */
static struct miscdevice mygpio_miscdev = {
	.minor = MYGPIO_MINOR,
	.name = MYGPIO_NAME,
	.fops = &mygpio_fops,
};


static int __init gpio_drv_init(void)
{
       int ret = 0;
       u8 val,val2;

       GPIO_OE = ioremap(GPIO_OE_BASE, 1);
       GPIO_IN = ioremap(GPIO_IN_BASE, 1);
       GPIO_OUT = ioremap(GPIO_OUT_BASE, 1);

       val = readb(GPIO_OE);
       pr_info("gpio_drv_init===>val:%x\r\n",val);
 /*      val &= 0xf0;
       writeb(val, GPIO_OE);    

       val2 = readb(GPIO_OE);
       pr_info("gpio_drv_init===>val2:%x\r\n",val2);   
*/

       val2 = readb(GPIO_IN);
       pr_info("gpio_drv_init===>val2:%x\r\n",val2);

       ret = misc_register(&mygpio_miscdev);
       if(ret < 0){
	   pr_info("misc device register failed!\r\n");
	   return -EFAULT;
       }

       return 0;
}

static void __exit gpio_drv_exit(void)
{
       iounmap(GPIO_OE);
       iounmap(GPIO_IN);
       iounmap(GPIO_OUT);

       return;
}

module_init(gpio_drv_init);
module_exit(gpio_drv_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("wugang");

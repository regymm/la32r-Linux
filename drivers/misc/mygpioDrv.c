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

#define MYGPIO_MINOR		210		/* 主设备号 */
#define MYGPIO_NAME		"mygpio" 	/* 设备名字 */

#define GPIO_OE_BASE				(0x1FD010D0)
#define GPIO_IN_BASE				(0x1FD010E0)
#define GPIO_OUT_BASE				(0x1FD010F0)

static void __iomem *GPIO_OE_V;
static void __iomem *GPIO_IN_V;
static void __iomem *GPIO_OUT_V;

static int mygpio_open(struct inode *inode, struct file *filp)
{
	return 0;
}

static ssize_t mygpio_read(struct file *filp, char __user *buf, size_t cnt, loff_t *offt)
{
         u8 val,ret;
         unsigned char data;

         val = readb(GPIO_IN_V);
         if((val & (1 << 7)))
               data=1;
         else 
               data=0;

         ret = copy_to_user(buf, &data, 1);

	return ret;
}

static ssize_t mygpio_write(struct file *filp, const char __user *buf, size_t cnt, loff_t *offt)
{
        int ret;
        unsigned char data,val,val2;

        ret = copy_from_user(&data, buf, cnt);
	if(ret < 0) {
		pr_info("kernel write failed!\r\n");
		return -EFAULT;
	}

        val = readb(GPIO_OUT_V);
        if(data)
           val |= (1 << 5);
        else
           val &= ~(1 << 5);

        writeb(val, GPIO_OUT_V);

        val2 = readb(GPIO_OE_V);
        pr_info("mygpio_write===>data:%d,val:0x%x,val2:0x%x\r\n",data,val,val2);

        return 0;
}

static int mygpio_release(struct inode *inode, struct file *filp)
{
	return 0;
}

/* 设备操作函数 */
static struct file_operations mygpio_fops = {
	.owner = THIS_MODULE,
	.open = mygpio_open,
	.read = mygpio_read,
	.write = mygpio_write,
	.release = mygpio_release,
};

/* MISC设备结构体 */
static struct miscdevice mygpio_miscdev = {
	.minor = MYGPIO_MINOR,
	.name = MYGPIO_NAME,
	.fops = &mygpio_fops,
};

static int __init mygpio_init(void)
{
       int ret = 0;
       u8 val,val2;

       GPIO_OE_V = ioremap(GPIO_OE_BASE, 1);
       GPIO_IN_V = ioremap(GPIO_IN_BASE, 1);
       GPIO_OUT_V = ioremap(GPIO_OUT_BASE, 1);

       val = readb(GPIO_OE_V);
       val |= (1 << 7);
       val &= ~(1 << 5);
       writeb(val, GPIO_OE_V);  
       pr_info("mygpio_init===>val:0x%x\r\n",val);

       ret = misc_register(&mygpio_miscdev);
       if(ret < 0){
	   pr_info("misc device register failed!\r\n");
	   return -EFAULT;
       }

       return 0;
}

static void __exit mygpio_exit(void)
{
       iounmap(GPIO_OE_V);
       iounmap(GPIO_IN_V);
       iounmap(GPIO_OUT_V);

       return;
}

module_init(mygpio_init);
module_exit(mygpio_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("wugang");


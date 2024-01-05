#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/compiler.h>
#include <linux/pci.h>
#include <linux/init.h>
#include <linux/ioport.h>
#include <linux/delay.h>
#include <linux/completion.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <asm/irq.h>
#include <linux/platform_device.h>
#include <linux/hrtimer.h>
 
//#define HRTIMER_TEST_CYCLE   0, (1000000 / 2)
#define HRTIMER_TEST_CYCLE   0, (1000000000/2)

#define GPIO_OE_BASE				(0x1FD010D0)
#define GPIO_OUT_BASE				(0x1FD010F0)
#define HCNTR_CTL_BASE                          (0x1FE7C000)
#define HCNTR_COUNTER_BASE                      (0x1FE7C004)
#define INT0_EN_BASE                            (0x1FD01044)
#define INT0_SR_BASE                            (0x1FD01040)
#define INT0_EDGE_BASE                          (0x1FD01054)
#define INT0_POL_BASE                           (0x1FD01050)
#define INT0_CLR_BASE                           (0x1FD0104C)


static void __iomem *GPIO_OE_V;
static void __iomem *GPIO_OUT_V;
static void __iomem *HCNTR_CTL_V;
static void __iomem *HCNTR_COUNTER_V;
static void __iomem *INT0_EN_V;
static void __iomem *INT0_SR_V;
static void __iomem *INT0_EDGE_V;
static void __iomem *INT0_POL_V;
static void __iomem *INT0_CLR_V;

static u8 g_val,g_val2;
 
static struct hrtimer kthread_timer;
int value = 0;
static u8 g_count = 0;

struct hrtimer_dev {
	dev_t devid;			/* 设备号 	 */
	void *private_data;	/* 私有数据 */
        int irq_num;
};
static struct hrtimer_dev my_dev;

static irqreturn_t hrtimer_irq(int irq, void *dev_id)
{
	struct hrtimer_dev *dev = (struct hrtimer_dev *)dev_id;
        u32 val_en;

        pr_info("hrtimer_irq===>irq:%d\r\n",irq);
   
        val_en = readl(INT0_EN_V);
        val_en |= (1<<21);
        writel(val_en,INT0_EN_V);          
        
	return IRQ_RETVAL(IRQ_HANDLED);
}
 
enum hrtimer_restart hrtimer_cb_func(struct hrtimer *timer) {
    u32 val,val2;
    u32 val_en,val_sr;
    u32 val_edge,val_pol,val_clr;

    value = !value;

    val = readb(GPIO_OUT_V);
    if(value)
           val |= (1 << 5);
    else
           val &= ~(1 << 5);

    writeb(val, GPIO_OUT_V);

    hrtimer_forward(timer, timer->base->get_time(), ktime_set(HRTIMER_TEST_CYCLE));

    ++g_count;

    val = 0x8000000f;
    if (100==g_count)
    {
        //writel(0x01f,HCNTR_CTL_V);
    }
    
    val = readl(HCNTR_CTL_V);
    val2 = readl(HCNTR_COUNTER_V);
    pr_info("======>val:0x%x,val2:0x%x\r\n",val,val2);
/*
    if(0==val2)
    {
        writel(val,HCNTR_CTL_V);
        pr_info("======>val:0x%x,val2:0x%x\r\n",val,val2);
    }
*/

    val_en = readl(INT0_EN_V);
    val_en |= (1<<21);
    writel(val_en,INT0_EN_V);

    val_en = readl(INT0_EN_V);
    val_sr = readl(INT0_SR_V);
    pr_info("hrtimer_cb_func===>val_en2:0x%x,val_sr2:0x%x\r\n",val_en,val_sr);

    val_edge = readl(INT0_EDGE_V);
    val_pol = readl(INT0_POL_V);
    val_clr = readl(INT0_CLR_V);
    pr_info("hrtimer_cb_func===>val_edge:0x%x,val_pol:0x%x,val_clr:0x%x\r\n",val_edge,val_pol,val_clr);

    return HRTIMER_RESTART;
}
 
void kthread_hrtimer_init(void) {
    hrtimer_init(&kthread_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
    kthread_timer.function = hrtimer_cb_func;
    hrtimer_start(&kthread_timer, ktime_set(HRTIMER_TEST_CYCLE), HRTIMER_MODE_REL);
}
 
static int __init hrtimer_test_init(void) {
    u32 val,val2;
    u32 val_en,val_sr;
    u32 val_edge,val_pol,val_clr;
    int ret = 0;

    printk(KERN_ALERT "hrtimer_test : Init !!\n");
 
    //printk(KERN_ALERT "hrtimer_test: init success!!\n");
    
    GPIO_OE_V = ioremap(GPIO_OE_BASE, 1);
    GPIO_OUT_V = ioremap(GPIO_OUT_BASE, 1);
    HCNTR_CTL_V = ioremap(HCNTR_CTL_BASE, 4);
    HCNTR_COUNTER_V = ioremap(HCNTR_COUNTER_BASE, 4);
    INT0_EN_V = ioremap(INT0_EN_BASE, 4);
    INT0_SR_V = ioremap(INT0_SR_BASE, 4);
    INT0_EDGE_V = ioremap(INT0_EDGE_BASE, 4);
    INT0_POL_V = ioremap(INT0_POL_BASE, 4);
    INT0_CLR_V = ioremap(INT0_CLR_BASE, 4);

    
    g_val = readb(GPIO_OE_V);
    g_val &= ~(1 << 5);
    writeb(g_val, GPIO_OE_V);
    g_val2 = readb(GPIO_OUT_V);
    pr_info("hr_timer_test_init===>g_val:0x%x,g_val2:0x%x\r\n",g_val,g_val2);

    //kthread_hrtimer_init();

    //val_en = readl(INT0_EN_V);
    //val_sr = readl(INT0_SR_V);
    //pr_info("hr_timer_test_init===>val_en:0x%x,val_sr:0x%x\r\n",val_en,val_sr);

    //val_en |= (1<<21);
    //val_en |= 0xffff0000;
    //writel(val_en,INT0_EN_V);

    val = 0x80000015;
    writel(val,HCNTR_CTL_V);
    
//while(1){
    val_en = readl(INT0_EN_V);
    val_sr = readl(INT0_SR_V);
    pr_info("hr_timer_test_init===>val_en2:0x%x,val_sr2:0x%x\r\n",val_en,val_sr);

    val = readl(HCNTR_CTL_V);
    val2 = readl(HCNTR_COUNTER_V);
    pr_info("===hr_timer_test_init===>val:0x%x,val2:0x%x\r\n",val,val2);

    INT0_EDGE_V = ioremap(INT0_EDGE_BASE, 4);
    INT0_POL_V = ioremap(INT0_POL_BASE, 4);
    INT0_CLR_V = ioremap(INT0_CLR_BASE, 4);

    val_edge = readl(INT0_EDGE_V);
    val_pol = readl(INT0_POL_V);
    val_clr = readl(INT0_CLR_V);
    pr_info("===hr_timer_test_init===>val_edge:0x%x,val_pol:0x%x,val_clr:0x%x\r\n",val_edge,val_pol,val_clr);

    msleep(1000);

    my_dev.irq_num = 50;
    ret = request_irq(my_dev.irq_num, hrtimer_irq, IRQF_TRIGGER_FALLING|IRQF_TRIGGER_RISING, "hrtimer-irq", &my_dev);  //IRQF_TRIGGER_FALLING|IRQF_TRIGGER_RISING
    if(ret < 0){
	printk("irq %d request failed!\r\n", my_dev.irq_num);
	return -EFAULT;
    }
 
    return 0;
}
 
static void __exit hrtimer_test_exit(void) {
 
    hrtimer_cancel(&kthread_timer);

    iounmap(GPIO_OE_V);
    iounmap(GPIO_OUT_V);
    iounmap(HCNTR_CTL_V);
    iounmap(HCNTR_COUNTER_V);
    iounmap(INT0_EN_V);
    iounmap(INT0_SR_V);
    iounmap(INT0_EDGE_V);
    iounmap(INT0_POL_V);
    iounmap(INT0_CLR_V);
 
    printk(KERN_ALERT "hrtimer_test: exit success!!\n");
}
 
module_init(hrtimer_test_init);
module_exit(hrtimer_test_exit);
 
MODULE_AUTHOR("wugang");
MODULE_LICENSE("GPL");


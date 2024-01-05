#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/sched.h>//jiffies在此头文件中定义
#include <linux/init.h>
#include <linux/timer.h>
#include <linux/delay.h>
#include <asm/uaccess.h>
#include <asm/io.h>
 
#include <linux/hrtimer.h>
 
//#define	HRTIMER0_US	(123 * 1000 * 1000)
#define	HRTIMER0_US	(1000)

#define GPIO_OE_BASE				(0x1FD010D0)
#define GPIO_OUT_BASE				(0x1FD010F0)

static void __iomem *GPIO_OE_V;
static void __iomem *GPIO_OUT_V;
static u8 g_val,g_val2;
static unsigned int tim_cnt1=0,tim_cnt2=0,count = 0;
 
struct hrtimer g_hrtimer0;
 
 
static enum hrtimer_restart hrtimer_test_fn(struct hrtimer *hrtimer)
{
    u8 val;
    ktime_t time_val;
    //pr_info("#### hrtimer timeout: %lu(ms)\n",HRTIMER0_US/1000/1000);
    if(!count) {
        time_val=ktime_get(); 
        tim_cnt1=ktime_to_us(time_val); //转us
     }
     ++count;
/*
    val = readb(GPIO_OUT_V);
    val |= 0x01;
    writeb(val, GPIO_OUT_V);  //输出1
    val &= 0xfe;                          
    writeb(val, GPIO_OUT_V); //输出0
*/
	
    hrtimer_forward_now(hrtimer, ns_to_ktime(HRTIMER0_US));

    if(!(9999 ^ count)) {
        time_val=ktime_get(); 
        tim_cnt2=ktime_to_us(time_val); //转us
        count = 0;
        pr_info("#### 10000 hrtimer timeout: %lu(us)\n",tim_cnt2-tim_cnt1);
     }
     
	return HRTIMER_RESTART;
}
 
static int __init hr_timer_test_init (void)
{
	pr_info("#### hr_timer_test module init...\n");

        GPIO_OE_V = ioremap(GPIO_OE_BASE, 1);
        GPIO_OUT_V = ioremap(GPIO_OUT_BASE, 1);

        g_val = readb(GPIO_OE_V);
        g_val &= 0xfe;
        writeb(g_val, GPIO_OE_V);
        g_val2 = readb(GPIO_OUT_V);
        pr_info("hr_timer_test_init===>val:0x%x,val2:0x%x\r\n",g_val,g_val2);
	
	struct hrtimer *hrtimer = &g_hrtimer0;
	hrtimer_init(hrtimer, CLOCK_MONOTONIC, HRTIMER_MODE_REL_HARD);
	hrtimer->function = hrtimer_test_fn;
	hrtimer_start(hrtimer, ns_to_ktime(HRTIMER0_US),
		      HRTIMER_MODE_REL_PINNED);
    
	return 0;
}
 
static void __exit hr_timer_test_exit (void)
{
	struct hrtimer *hrtimer = &g_hrtimer0;
	hrtimer_cancel(hrtimer);

        iounmap(GPIO_OE_V);
        iounmap(GPIO_OUT_V);

    pr_info("#### hr_timer_test module exit...\n");
}
 
module_init(hr_timer_test_init);
module_exit(hr_timer_test_exit);
 
MODULE_AUTHOR("wugang");
MODULE_LICENSE("GPL");


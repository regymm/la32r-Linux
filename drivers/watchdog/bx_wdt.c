#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/types.h>
#include <linux/watchdog.h>

#define WDT_MAX_TIMEOUT         16
#define WDT_MIN_TIMEOUT         1
#define DRV_NAME		"bx-wdt"

#define WDT_MODE_EN             (1 << 0)

static bool nowayout = WATCHDOG_NOWAYOUT;
static unsigned int timeout;

module_param(timeout, uint, 0);
MODULE_PARM_DESC(timeout, "Watchdog heartbeat in seconds");

module_param(nowayout, bool, 0);
MODULE_PARM_DESC(nowayout, "Watchdog cannot be stopped once started "
		"(default=" __MODULE_STRING(WATCHDOG_NOWAYOUT) ")");

struct bx_wdt_reg {
	u8 wdt_set;
	u8 wdt_timer;
	u8 wdt_en;
};

static const struct bx_wdt_reg bx_wdt_reg = {
	.wdt_set = 0x00,
	.wdt_timer = 0x04,
	.wdt_en = 0x08,
};

static const struct of_device_id bx_wdt_dt_ids[] = {
	{ .compatible = "ls,bx-wdt", .data = &bx_wdt_reg },
};

MODULE_DEVICE_TABLE(of, bx_wdt_dt_ids);

struct bx_wdt_dev {
	struct watchdog_device wdt_dev;
	void __iomem *wdt_base;
	const struct bx_wdt_reg *wdt_regs;
        spinlock_t		io_lock;
};

static int bx_wdt_set_timeout(struct watchdog_device *wdt_dev, unsigned int timeout)
{
        unsigned int freq;

        pr_info("bx_wdt_set_timeout begin===>\r\n");
        
        struct bx_wdt_dev *bx_wdt = watchdog_get_drvdata(wdt_dev);
	void __iomem *wdt_base = bx_wdt->wdt_base;
	const struct bx_wdt_reg *regs = bx_wdt->wdt_regs;
	u32 reg;

        bx_wdt->wdt_dev.timeout = 33000000*timeout;
        writel(bx_wdt->wdt_dev.timeout, wdt_base + regs->wdt_timer);        

	return 0;
}

static int bx_wdt_start(struct watchdog_device *wdt_dev)
{
        pr_info("bx_wdt_start begin===>\r\n");

        struct bx_wdt_dev *bx_wdt = watchdog_get_drvdata(wdt_dev);
	void __iomem *wdt_base = bx_wdt->wdt_base;
	const struct bx_wdt_reg *regs = bx_wdt->wdt_regs;
        int ret;

	/* Enable watchdog */
	writel(0x1, wdt_base + regs->wdt_en);

        ret = bx_wdt_set_timeout(&bx_wdt->wdt_dev,bx_wdt->wdt_dev.timeout);

        writel(0x1, wdt_base + regs->wdt_set);

	return 0;
}

static int bx_wdt_stop(struct watchdog_device *wdt_dev)
{
        pr_info("bx_wdt_stop begin===>\r\n");

        struct bx_wdt_dev *bx_wdt = watchdog_get_drvdata(wdt_dev);
	void __iomem *wdt_base = bx_wdt->wdt_base;
	const struct bx_wdt_reg *regs =bx_wdt->wdt_regs;

        writel(0x0, wdt_base + regs->wdt_set);
	writel(0x0, wdt_base + regs->wdt_en);

	return 0;
}

static int bx_wdt_ping(struct watchdog_device *wdt_dev)
{
        
        pr_info("bx_wdt_ping begin===>\r\n");

        struct bx_wdt_dev *bx_wdt = watchdog_get_drvdata(wdt_dev);
	void __iomem *wdt_base = bx_wdt->wdt_base;
	const struct bx_wdt_reg *regs =bx_wdt->wdt_regs;

        writel(0x1, wdt_base + regs->wdt_en);
	writel(bx_wdt->wdt_dev.timeout, wdt_base + regs->wdt_timer);
	writel(0x1, wdt_base + regs->wdt_set);

	return 0;
}


static const struct watchdog_info bx_wdt_info = {
	.options = WDIOF_SETTIMEOUT | WDIOF_KEEPALIVEPING | WDIOF_MAGICCLOSE,
	.identity = DRV_NAME,
};

static const struct watchdog_ops bx_wdt_ops = {
	.owner = THIS_MODULE,
	.start = bx_wdt_start,
	.stop  = bx_wdt_stop,
	.ping  = bx_wdt_ping,
	.set_timeout = bx_wdt_set_timeout,
};

static int bx_wdt_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct bx_wdt_dev *bx_wdt;
	int err;
        unsigned long freq;

	bx_wdt = devm_kzalloc(dev, sizeof(*bx_wdt), GFP_KERNEL);
	if (!bx_wdt)
		return -ENOMEM;

	bx_wdt->wdt_regs = of_device_get_match_data(dev);
	if (!bx_wdt->wdt_regs)
		return -ENODEV;

	bx_wdt->wdt_base = devm_platform_ioremap_resource(pdev, 0);
	if (IS_ERR(bx_wdt->wdt_base))
		return PTR_ERR(bx_wdt->wdt_base);

        pr_info("bx_wdt_probe begin===>bx_wdt->wdt_base:0x%0x\r\n",bx_wdt->wdt_base);

        bx_wdt->wdt_dev.info = &bx_wdt_info;
	bx_wdt->wdt_dev.ops = &bx_wdt_ops;
	bx_wdt->wdt_dev.timeout = WDT_MAX_TIMEOUT;
	bx_wdt->wdt_dev.max_timeout = WDT_MAX_TIMEOUT;
	bx_wdt->wdt_dev.min_timeout = WDT_MIN_TIMEOUT;
	bx_wdt->wdt_dev.parent = dev;

	watchdog_init_timeout(&bx_wdt->wdt_dev, timeout, dev);
	watchdog_set_nowayout(&bx_wdt->wdt_dev, nowayout);
	watchdog_set_restart_priority(&bx_wdt->wdt_dev, 128);

	watchdog_set_drvdata(&bx_wdt->wdt_dev, bx_wdt);

        bx_wdt_stop(&bx_wdt->wdt_dev);
        spin_lock_init(&bx_wdt->io_lock);

	watchdog_stop_on_reboot(&bx_wdt->wdt_dev);
	err = devm_watchdog_register_device(dev, &bx_wdt->wdt_dev);
	if (unlikely(err))
		return err;

	dev_info(dev, "Watchdog enabled (timeout=%d sec, nowayout=%d)", bx_wdt->wdt_dev.timeout, nowayout);

        return 0;
}

static struct platform_driver bx_wdt_driver = {
	.probe		= bx_wdt_probe,
	.driver		= {
		.name	= DRV_NAME,
                .of_match_table	= bx_wdt_dt_ids,
	},
};

module_platform_driver(bx_wdt_driver);

MODULE_AUTHOR("wugang");
MODULE_DESCRIPTION("bx Watchdog Driver");
MODULE_LICENSE("GPL");

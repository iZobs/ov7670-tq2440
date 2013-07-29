/*************************************************************************
	> File Name: ov7670.c
	> Author: izobs
	> Mail: ivincentlin@gmail.com
	> Created Time: 2013年07月27日 星期六 08时32分48秒
 ************************************************************************/

#include <linux/module.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/ioport.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/clk.h>
#include <linux/random.h>
#include <linux/version.h>
#include <linux/videodev2.h>
#include <linux/dma-mapping.h>
#ifdef CONFIG_VIDEO_V4L1_COMPAT
#include <linux/videodev.h>
#endif
#include <linux/interrupt.h>
#include <media/v4l2-common.h>
#include <linux/highmem.h>
#include <linux/miscdevice.h>

#include <asm/io.h>
#include <asm/memory.h>
#include <mach/regs-gpio.h>
#include <mach/regs-gpioj.h>
#include <mach/regs-clock.h>
#include <mach/map.h>

#include "ov7670.h"

/* 摄像头名字 */
#define DEVICE_NAME		"ov7670"
#define CAMIF_INIT_FUNCTION      "In camif_init()"

#define DEBUG 1

static unsigned has_ov7670;
unsigned long camif_base_addr;

/* camera device(s) */
static struct ov7670_camif_dev camera;

/*sccb_start*/
static void __inline__ sccb_start(void)
{
	CFG_WRITE(SIO_D);

	Low(SIO_D);
	WAIT_STABLE();
}


static void __inline__ sccb_write_byte(u8 data)
{
	int i;

	CFG_WRITE(SIO_D);
	WAIT_STABLE();

	/* write 8-bits octet. */
	for (i=0;i<8;i++)
	{
		Low(SIO_C);
		WAIT_STABLE();

		if (data & 0x80)
		{
			High(SIO_D);
		}
		else
		{
			Low(SIO_D);
		}
		data = data<<1;
		WAIT_CYCLE();

		High(SIO_C);
		WAIT_CYCLE();
	}

	/* write byte done, wait the Don't care bit now. */
	{
		Low(SIO_C);
		High(SIO_D);
		CFG_READ(SIO_D);
		WAIT_CYCLE();

		High(SIO_C);
		WAIT_CYCLE();
	}
}


static u8 __inline__ sccb_read_byte(void)
{
	int i;
	u8 data;

	CFG_READ(SIO_D);
	WAIT_STABLE();

	Low(SIO_C);
	WAIT_CYCLE();

	data = 0;
	for (i=0;i<8;i++)
	{
		High(SIO_C);
		WAIT_STABLE();

		data = data<<1;
		data |= State(SIO_D)?1:0;
		WAIT_CYCLE();

		Low(SIO_C);
		WAIT_CYCLE();
	}

	/* read byte down, write the NA bit now.*/
	{
		CFG_WRITE(SIO_D);
		High(SIO_D);
		WAIT_CYCLE();

		High(SIO_C);
		WAIT_CYCLE();
	}

	return data;
}

static void __inline__ sccb_stop(void)
{
	Low(SIO_C);
	WAIT_STABLE();

	CFG_WRITE(SIO_D);
	Low(SIO_D);
	WAIT_CYCLE();

	High(SIO_C);
	WAIT_STABLE();

	High(SIO_D);
	WAIT_CYCLE();

	CFG_READ(SIO_D);
}


void sccb_write(u8 IdAddr, u8 SubAddr, u8 data)
{
	down(&bus_lock);
	sccb_start();
	sccb_write_byte(IdAddr);
	sccb_write_byte(SubAddr);
	sccb_write_byte(data);
	sccb_stop();
	up (&bus_lock);
}

u8 sccb_read(u8 IdAddr, u8 SubAddr)
{
	u8 data;

	down(&bus_lock);
	sccb_start();
	sccb_write_byte(IdAddr);
	sccb_write_byte(SubAddr);
	sccb_stop();

	sccb_start();
	sccb_write_byte(IdAddr|0x01);
	data = sccb_read_byte();
	sccb_stop();
	up(&bus_lock);

	return data;
}

static u32 __inline__ show_ov7670_product_id(void)
{
    u32 pid,manuf;

	pid = sccb_read(OV7670_SCCB_ADDR, 0x0a)<<8;
	pid |= sccb_read(OV7670_SCCB_ADDR, 0x0b);

	manuf = sccb_read(OV7670_SCCB_ADDR, 0x1c)<<8;
	manuf |= sccb_read(OV7670_SCCB_ADDR, 0x1d);


	printk(DEVICE_NAME"----OV7670 address 0x%02X, product ID: 0x%04X, manufacture: 0x%04X\n", OV7670_SCCB_ADDR, pid, manuf);
	return pid;
}



/*
 * Stuff that knows about the sensor.
 */

static int  ov7670_reset(void)
{
	sccb_write(OV7670_SCCB_ADDR,REG_COM7,COM7_RESET);
	msleep(1);
	return 0;
}

//在这个函数中完成对ov7670寄存器的配置
static void ov7670_init_defual_regs(void)
{
	int i;

	down(&regs_mutex);
	for (i=0; i<ARRAY_SIZE(regs); i++)
	{
		if (regs[i].subaddr == 0xff)
		{
			mdelay(regs[i].value);
			continue;
		}
		sccb_write(OV7670_SCCB_ADDR, regs[i].subaddr, regs[i].value);
	}
	up(&regs_mutex);
}



/*
 * Store a set of start/stop values into the camera.
 */
static int ov7670_set_hw(int hstart, int hstop,
		int vstart, int vstop)
{
	u8 v;
/*
 * Horizontal: 11 bits, top 8 live in hstart and hstop.  Bottom 3 of
 * hstart are in href[2:0], bottom 3 of hstop in href[5:3].  There is
 * a mystery "edge offset" value in the top two bits of href.
 */
	#if DEBUG
	v = sccb_read(OV7670_SCCB_ADDR,REG_HREF);
	printk(DEVICE_NAME"----the REG_HREF is:%x\n",v);
	v = (v & 0xc0) | ((hstop & 0x7) << 3) | (hstart & 0x7);
	msleep(10);
	sccb_write(OV7670_SCCB_ADDR,REG_HREF, v);
	msleep(10);

	v = sccb_read(OV7670_SCCB_ADDR,REG_HREF);
	printk(DEVICE_NAME"----the REG_HREF is:%x\n",v);

	#endif
	return v;
}




/*sccb_init*/
static void __inline__ init_sccb(void)
{

//	sccb_init();
	CFG_WRITE(SIO_C);
	CFG_WRITE(SIO_D);

	High(SIO_C);
	High(SIO_D);
	WAIT_STABLE();

        #if DEBUG
	printk(DEVICE_NAME"----init_sccb.....\n");
	#endif
}

/*
 * camif_open()
 */
static int camif_open(struct inode *inode, struct file *file)
{
    show_ov7670_product_id();
    ov7670_reset();
    ov7670_set_hw(1,1,1,1);
    return 0;
}


/*
 * camif_release()
 */
static int camif_release(struct inode *inode, struct file *file)
{
    return 0;
}

/*
 * camif_read()
 */
static ssize_t camif_read(struct file *file, char __user *data, size_t count, loff_t *ppos)
{
    return 0;
}



static struct file_operations camif_fops =
{
	.owner		= THIS_MODULE,
	.open		= camif_open,
	.release	= camif_release,
	.read		= camif_read,
};

static struct miscdevice misc = {
        .minor = MISC_DYNAMIC_MINOR,
        .name = DEVICE_NAME,
        .fops = &camif_fops,
};

/*
 * camif_init()
 */
static int __init camif_init(void)
{
    int ret;
    struct ov7670_camif_dev * pdev;
    struct clk * camif_upll_clk;

    pdev = &camera;

    #if DEBUG
    printk(DEVICE_NAME"----init ov7670.......\n");
    #endif

	/* set gpio-j to camera mode. */
	s3c2410_gpio_cfgpin(S3C2440_GPJ0, S3C2440_GPJ0_CAMDATA0);
	s3c2410_gpio_cfgpin(S3C2440_GPJ1, S3C2440_GPJ1_CAMDATA1);
	s3c2410_gpio_cfgpin(S3C2440_GPJ2, S3C2440_GPJ2_CAMDATA2);
	s3c2410_gpio_cfgpin(S3C2440_GPJ3, S3C2440_GPJ3_CAMDATA3);
	s3c2410_gpio_cfgpin(S3C2440_GPJ4, S3C2440_GPJ4_CAMDATA4);
	s3c2410_gpio_cfgpin(S3C2440_GPJ5, S3C2440_GPJ5_CAMDATA5);
	s3c2410_gpio_cfgpin(S3C2440_GPJ6, S3C2440_GPJ6_CAMDATA6);
	s3c2410_gpio_cfgpin(S3C2440_GPJ7, S3C2440_GPJ7_CAMDATA7);
	s3c2410_gpio_cfgpin(S3C2440_GPJ8, S3C2440_GPJ8_CAMPCLK);
	s3c2410_gpio_cfgpin(S3C2440_GPJ9, S3C2440_GPJ9_CAMVSYNC);
	s3c2410_gpio_cfgpin(S3C2440_GPJ10, S3C2440_GPJ10_CAMHREF);
	s3c2410_gpio_cfgpin(S3C2440_GPJ11, S3C2440_GPJ11_CAMCLKOUT);
	s3c2410_gpio_cfgpin(S3C2440_GPJ12, S3C2440_GPJ12_CAMRESET);


	/* init camera's virtual memory. */
	if (!request_mem_region((unsigned long)S3C2440_PA_CAMIF, S3C2440_SZ_CAMIF, DEVICE_NAME))
	{
		ret = -EBUSY;
		goto error1;
	}


	/* remap the virtual memory. */
	/*内核内存，类型用unsigned long*/
	camif_base_addr = (unsigned long)ioremap_nocache((unsigned long)S3C2440_PA_CAMIF, S3C2440_SZ_CAMIF);
	if (camif_base_addr == (unsigned long)NULL)
	{
		ret = -EBUSY;
		goto error2;
	}

	/* init camera clock. */
	pdev->clk = clk_get(NULL, "camif"); //获得"camif"的时钟
	if (IS_ERR(pdev->clk))
	{
		ret = -ENOENT;
		goto error3;
	}
	clk_enable(pdev->clk);   //使能这个时钟

	camif_upll_clk = clk_get(NULL, "camif-upll");
	clk_set_rate(camif_upll_clk, 24000000);
	mdelay(100);


	/* init reference counter and its mutex. */
	mutex_init(&pdev->rcmutex);
	pdev->rc = 0;

	/* init image input source. */
	pdev->input = 0;

	/* init camif state and its lock. */
	pdev->state = CAMIF_STATE_FREE;

	/* init command code, command lock and the command wait queue. */
	pdev->cmdcode = CAMIF_CMD_NONE;
	init_waitqueue_head(&pdev->cmdqueue);


	/* register to videodev layer. */
	if (misc_register(&misc) < 0)
	{
		ret = -EBUSY;
		goto error4;
	}
	init_sccb();
	return 0;

	#if DEBUG
	printk(KERN_ALERT"ov7670 camif init done\n");
	#endif
error4:
        misc_deregister(&misc);
 	#if DEBUG
	printk(CAMIF_INIT_FUNCTION"----misc_register error 4 is called,falied request mem\n");
	#endif

error3:
	clk_put(pdev->clk);
 	#if DEBUG
	printk(CAMIF_INIT_FUNCTION"----clk_err error 3 is called,falied request mem\n");
	#endif

error2:
        iounmap((void *)camif_base_addr);
 	#if DEBUG
	printk(CAMIF_INIT_FUNCTION"----ioremap_nocache error 2 is called,falied request mem\n");
	#endif

error1:
	release_mem_region((unsigned long)S3C2440_PA_CAMIF, S3C2440_SZ_CAMIF);
	#if DEBUG
	printk(CAMIF_INIT_FUNCTION"----mem_region error 1 is called,falied request mem\n");
	#endif

}


/*
 * camif_cleanup()
 */
static void __exit camif_cleanup(void)
{
        struct ov7670_camif_dev *pdev;

	CFG_READ(SIO_C);
	CFG_READ(SIO_D);

	pdev = &camera;

	misc_deregister(&misc);

	clk_put(pdev->clk);

	iounmap((void *)camif_base_addr);
	release_mem_region((unsigned long)S3C2440_PA_CAMIF, S3C2440_SZ_CAMIF);

	#if DEBUG
        printk(DEVICE_NAME"----ov7670_camif: module removed\n");
	#endif
}

MODULE_LICENSE("GPL");

MODULE_AUTHOR("izobs<Email:ivincentlin@gmail.com>");

module_init(camif_init);
module_exit(camif_cleanup);

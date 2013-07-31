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

#define DEBUG 0
#define DEBUG_CAMIF 1

static unsigned has_ov7670;
unsigned long camif_base_addr;
static u8 update_cmaif_regs_flag = 0;

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

int s3c2440_ov7670_init(void)
{
    #if DEBUG
    printk(DEVICE_NAME"----s3c2440_ov7670_init is called\n");
    #endif

    ov7670_init_defual_regs();

    printk(DEVICE_NAME"----ov7670 init_defual_regs is done\n");

    show_ov7670_product_id();
    return 0;
}


/* update CISRCFMT only. */
static void __inline__ update_source_fmt_regs(struct ov7670_camif_dev * pdev)
{
	u32 cisrcfmt;
	u32 recisrcfmt;

	recisrcfmt = ioread32(S3C244X_CISRCFMT);
       #if DEBUG_CAMIF
	printk(DEVICE_NAME"--------------before write, S3C244X_CISRCFMT is %x\n",recisrcfmt);
	#endif

	cisrcfmt = (1<<31)					// ITU-R BT.601 YCbCr 8-bit mode
		|(0<<30)				// CB,Cr value offset cntrol for YCbCr
		|(pdev->srcHsize<<16)	// source image width
		|(2<<14)				// input order is CbYCrY
		|(pdev->srcVsize<<0);	// source image height

	iowrite32(cisrcfmt, S3C244X_CISRCFMT);
	mdelay(3);

	recisrcfmt = ioread32(S3C244X_CISRCFMT);
	#if DEBUG_CAMIF
	printk(DEVICE_NAME"--------------after wirte,the S3C244X_CISRCFMT is %x\n",recisrcfmt);
	#endif
}


/* calculate main burst size and remained burst size. */
static void __inline__ calc_burst_size(u32 pixperword,u32 hSize, u32 *mainBurstSize, u32 *remainedBurstSize)
{
	u32 tmp;

	tmp = (hSize/pixperword)%16;

	switch(tmp)
	{
	case 0:
		*mainBurstSize = 16;
		*remainedBurstSize = 16;
		break;
	case 4:
		*mainBurstSize = 16;
		*remainedBurstSize = 4;
		break;
	case 8:
		*mainBurstSize=16;
		*remainedBurstSize = 8;
		break;
	default:
	       tmp=(hSize/pixperword)%8;
		switch(tmp)
		{
		case 0:
			*mainBurstSize = 8;
			*remainedBurstSize = 8;
			break;
		case 4:
			*mainBurstSize = 8;
			*remainedBurstSize = 4;
		default:
			*mainBurstSize = 4;
			tmp = (hSize/pixperword)%4;
			*remainedBurstSize = (tmp)?tmp:4;
			break;
		}
		break;
	}
}


/* update registers:
 *	PREVIEW path:
 *		CIPRCLRSA1 ~ CIPRCLRSA4
 *		CIPRTRGFMT
 *		CIPRCTRL
 *		CIPRSCCTRL
 *		CIPRTAREA
 *	CODEC path:
 *		CICOYSA1 ~ CICOYSA4
 *		CICOCBSA1 ~ CICOCBSA4
 *		CICOCRSA1 ~ CICOCRSA4
 *		CICOTRGFMT
 *		CICOCTRL
 *		CICOTAREA
 */
static void __inline__ update_target_fmt_regs(struct ov7670_camif_dev * pdev)
{
	u32 ciprtrgfmt;
	u32 ciprctrl;
	u32 ciprscctrl;

	u32 reciprtrgfmt;
	u32 reciprctrl;

	u32 mainBurstSize, remainedBurstSize;

	/* CIPRCLRSA1 ~ CIPRCLRSA4. */
        /*RGB frame start address for preview DMA,p-path is RGB*/
	iowrite32(img_buff[0].phy_base, S3C244X_CIPRCLRSA1);
	iowrite32(img_buff[1].phy_base, S3C244X_CIPRCLRSA2);
	iowrite32(img_buff[2].phy_base, S3C244X_CIPRCLRSA3);
	iowrite32(img_buff[3].phy_base, S3C244X_CIPRCLRSA4);

	/* CIPRTRGFMT. */
	ciprtrgfmt =	(pdev->preTargetHsize<<16)		// horizontal pixel number of target image
			|(0<<14)						// don't mirror or rotation.
			|(pdev->preTargetVsize<<0);	// vertical pixel number of target image
	iowrite32(ciprtrgfmt, S3C244X_CIPRTRGFMT);


	#if DEBUG_CAMIF
	mdelay(3);
	reciprtrgfmt = ioread32(S3C244X_CIPRTRGFMT);
	printk(DEVICE_NAME"--------------the CIPRTRGFMT is %x\n",reciprtrgfmt);
	#endif


	/* CIPRCTRL. */
	/*RGB 16-bit ,2 pixle/word ,mB=16,rB=16*/
	calc_burst_size(2, pdev->preTargetHsize, &mainBurstSize, &remainedBurstSize);    //2 pixle a word
	ciprctrl = (mainBurstSize<<19)|(remainedBurstSize<<14);
	ciprctrl = ciprctrl|(1<<2);

	iowrite32(ciprctrl, S3C244X_CIPRCTRL);

	#if DEBUG_CAMIF
	mdelay(3);
	reciprctrl=ioread32(S3C244X_CIPRCTRL);
	printk(DEVICE_NAME"--------------the S3C244X_CIPRCTRL is %x\n",reciprctrl);
	#endif


	/* CIPRSCCTRL. */
	ciprscctrl = ioread32(S3C244X_CIPRSCCTRL);

	#if DEBUG_CAMIF
	printk(DEVICE_NAME"-------------before write,S3C244X_CIPRSCCTRL, is %x\n",ciprscctrl);
	#endif

	/* CIPRSCCTRL. */
	ciprscctrl &= 1<<15;	// clear all other info except 'preview scaler start'.
	ciprscctrl |= 0<<30;	// 16-bits RGB
	iowrite32(ciprscctrl, S3C244X_CIPRSCCTRL);	// 16-bit RGB

	#if DEBUG_CAMIF
	mdelay(1);
	ciprscctrl = ioread32(S3C244X_CIPRSCCTRL);
	printk(DEVICE_NAME"--------------S3C244X_CIPRSCCTRL is %x\n",ciprscctrl);
	#endif

	/* CIPRTAREA. */
	iowrite32(pdev->preTargetHsize * pdev->preTargetVsize, S3C244X_CIPRTAREA);

	#if DEBUG
	printk(DEVICE_NAME"----update_target_fmt_regs is done\n");
	#endif
}


/* update CIWDOFST only. */
static void __inline__ update_target_wnd_regs(struct ov7670_camif_dev * pdev)
{
	u32 ciwdofst;
	u32 reciwdofst;
	u32 winHorOfst, winVerOfst;

	winHorOfst = (pdev->srcHsize - pdev->wndHsize)>>1;
	winVerOfst = (pdev->srcVsize - pdev->wndVsize)>>1;

	winHorOfst &= 0xFFFFFFF8;
	winVerOfst &= 0xFFFFFFF8;
	if ((winHorOfst == 0)&&(winVerOfst == 0))
	{
		ciwdofst = 0;	// disable windows offset.
	}
	else
	{
		ciwdofst = (1<<31)			// window offset enable
			|(1<<30)			// clear the overflow ind flag of input CODEC FIFO Y
			|(winHorOfst<<16)		// windows horizontal offset
			|(1<<15)			// clear the overflow ind flag of input CODEC FIFO Cb
			|(1<<14)			// clear the overflow ind flag of input CODEC FIFO Cr
			|(1<<13)			// clear the overflow ind flag of input PREVIEW FIFO Cb
			|(1<<12)			// clear the overflow ind flag of input PREVIEW FIFO Cr
			|(winVerOfst<<0);		// window vertical offset
	}

	iowrite32(ciwdofst, S3C244X_CIWDOFST);

	#if DEBUG_CAMIF
	mdelay(1);
	reciwdofst = ioread32(S3C244X_CIWDOFST);
	printk(DEVICE_NAME"------------S3C244X_CIWDOFST is %x\n",reciwdofst);
	#endif


	#if DEBUG
	printk(DEVICE_NAME"----update_target_wnd_regs is done\n");
	#endif
}


/* calculate prescaler ratio and shift. */
/*根据datasheet提供的算法*/
static void __inline__ calc_prescaler_ratio_shift(u32 SrcSize, u32 DstSize, u32 *ratio, u32 *shift)
{
	if(SrcSize>=32*DstSize)
	{
		*ratio=32;
		*shift=5;
	}
	else if(SrcSize>=16*DstSize)
	{
		*ratio=16;
		*shift=4;
	}
	else if(SrcSize>=8*DstSize)
	{
		*ratio=8;
		*shift=3;
	}
	else if(SrcSize>=4*DstSize)
	{
		*ratio=4;
		*shift=2;
	}
	else if(SrcSize>=2*DstSize)
	{
		*ratio=2;
		*shift=1;
	}
	else
	{
		*ratio=1;
		*shift=0;
	}
}


/* update registers:
 *	PREVIEW path:
 *		CIPRSCPRERATIO
 *		CIPRSCPREDST
 *		CIPRSCCTRL
 *	CODEC path:
 *		CICOSCPRERATIO
 *		CICOSCPREDST
 *		CICOSCCTRL
 */
static void __inline__ update_target_zoom_regs(struct ov7670_camif_dev * pdev)
{
	u32 preHratio, preVratio;
	u32 Hshift, Vshift;
	u32 shfactor;
	u32 preDstWidth, preDstHeight;
	u32 Hscale, Vscale;
	u32 mainHratio, mainVratio;

	u32 ciprscpreratio;
	u32 ciprscpredst;
	u32 ciprscctrl;


	/* CIPRSCPRERATIO. */
	calc_prescaler_ratio_shift(pdev->wndHsize, pdev->preTargetHsize, &preHratio, &Hshift);
	calc_prescaler_ratio_shift(pdev->wndVsize, pdev->preTargetVsize, &preVratio, &Vshift);

	shfactor = 10 - (Hshift + Vshift);

	ciprscpreratio =	(shfactor<<28)		// shift factor for preview pre-scaler
			|(preHratio<<16)		// horizontal ratio of preview pre-scaler
			|(preVratio<<0);		// vertical ratio of preview pre-scaler
	iowrite32(ciprscpreratio, S3C244X_CIPRSCPRERATIO);

	/* CIPRSCPREDST. */
	preDstWidth = pdev->wndHsize / preHratio;
	preDstHeight = pdev->wndVsize / preVratio;
	ciprscpredst =	(preDstWidth<<16)		// destination width for preview pre-scaler
		   	|(preDstHeight<<0);		// destination height for preview pre-scaler
	iowrite32(ciprscpredst, S3C244X_CIPRSCPREDST);

	/* CIPRSCCTRL. */
	Hscale = (pdev->wndHsize >= pdev->preTargetHsize)?0:1;
	Vscale = (pdev->wndVsize >= pdev->preTargetVsize)?0:1;
	mainHratio = (pdev->wndHsize<<8)/(pdev->preTargetHsize<<Hshift);
	mainVratio = (pdev->wndVsize<<8)/(pdev->preTargetVsize<<Vshift);
	ciprscctrl = ioread32(S3C244X_CIPRSCCTRL);
	#if DEBUG
	printk(DEVICE_NAME"----S3C244x_CIPRSCCTRL is %x\n",ciprscctrl);
	#endif
	ciprscctrl &= (1<<30)|(1<<15);	// keep preview image format (RGB565 or RGB24), and preview scaler start state.
	ciprscctrl	|= (1<<31)	// this bit should be always 1.
		|(Hscale<<29)	// horizontal scale up/down.
		|(Vscale<<28)	// vertical scale up/down.
		|(mainHratio<<16)	// horizontal scale ratio for preview main-scaler
		|(mainVratio<<0);	// vertical scale ratio for preview main-scaler
	iowrite32(ciprscctrl, S3C244X_CIPRSCCTRL);
	#if DEBUG
	printk(DEVICE_NAME"----update_target_zoom_regs is done\n");
	#endif
}





/* update camif registers, called only when camif ready, or ISR. */
static void __inline__ update_camif_regs(struct ov7670_camif_dev * pdev)
{

	if (!in_irq())
	{
		while(1)	// wait until VSYNC is 'L'
		{
			barrier();
			if ((ioread32(S3C244X_CICOSTATUS)&(1<<28)) == 0)
			{
			    printk(DEVICE_NAME"---- vsync is 'L'\n");
				break;
			}
		}
	}

	/* WARNING: don't change the statement sort below!!! */

	update_source_fmt_regs(pdev);
	update_target_wnd_regs(pdev);
	update_target_fmt_regs(pdev);
	update_target_zoom_regs(pdev);

        #if DEBUG_CAMIF
	update_cmaif_regs_flag +=1;
	printk(DEVICE_NAME"----update_camif_regs call [%d] time\n",update_cmaif_regs_flag);
	#endif

}


/* update camera interface with the new config. */
static void update_camif_config (struct ov7670_camif_fh * fh, u32 cmdcode)
{
	struct ov7670_camif_dev	* pdev;

	pdev = fh->dev;

	switch (pdev->state)
	{
	    /*in camif_open():
	      dev->state = CAMIF_STATE_READY;
	      so in init_camif_config(),this case will be cailled
	    */
	case CAMIF_STATE_READY:
		update_camif_regs(fh->dev);		// config the regs directly.
		break;

	case CAMIF_STATE_PREVIEWING:

		/* camif is previewing image. */

		disable_irq(IRQ_S3C2440_CAM_P);		// disable cam-preview irq.

		/* source image format. */
		if (cmdcode & CAMIF_CMD_SFMT)
		{
			// ignore it, nothing to do now.
		}

		/* target image format. */
		if (cmdcode & CAMIF_CMD_TFMT)
		{
				/* change target image format only. */
				pdev->cmdcode |= CAMIF_CMD_TFMT;
		}

		/* target image window offset. */
		if (cmdcode & CAMIF_CMD_WND)
		{
			pdev->cmdcode |= CAMIF_CMD_WND;
		}

		/* target image zoomi & zoomout. */
		if (cmdcode & CAMIF_CMD_ZOOM)
		{
			pdev->cmdcode |= CAMIF_CMD_ZOOM;
		}

		/* stop previewing. */
		if (cmdcode & CAMIF_CMD_STOP)
		{
			pdev->cmdcode |= CAMIF_CMD_STOP;
		}
		enable_irq(IRQ_S3C2440_CAM_P);	// enable cam-preview irq.

		wait_event(pdev->cmdqueue, (pdev->cmdcode==CAMIF_CMD_NONE));	// wait until the ISR completes command.
		break;

	case CAMIF_STATE_CODECING:

		/* camif is previewing image. */

		disable_irq(IRQ_S3C2440_CAM_C);		// disable cam-codec irq.

		/* source image format. */
		if (cmdcode & CAMIF_CMD_SFMT)
		{
			// ignore it, nothing to do now.
		}

		/* target image format. */
		if (cmdcode & CAMIF_CMD_TFMT)
		{
				/* change target image format only. */
				pdev->cmdcode |= CAMIF_CMD_TFMT;
		}

		/* target image window offset. */
		if (cmdcode & CAMIF_CMD_WND)
		{
			pdev->cmdcode |= CAMIF_CMD_WND;
		}

		/* target image zoomi & zoomout. */
		if (cmdcode & CAMIF_CMD_ZOOM)
		{
			pdev->cmdcode |= CAMIF_CMD_ZOOM;
		}

		/* stop previewing. */
		if (cmdcode & CAMIF_CMD_STOP)
		{
			pdev->cmdcode |= CAMIF_CMD_STOP;
		}
		enable_irq(IRQ_S3C2440_CAM_C);	// enable cam-codec irq.
		wait_event(pdev->cmdqueue, (pdev->cmdcode==CAMIF_CMD_NONE));	// wait until the ISR completes command.
		break;

	default:
		break;
	}


}

/* config camif when master-open camera.*/
static void init_camif_config(struct ov7670_camif_fh *fh)
{
        struct ov7670_camif_dev *pdev;
        pdev = fh->dev;
	pdev->input = 0;	// FIXME, the default input image format, see inputs[] for detail.

	/* the source image size (input from external camera). */
	pdev->srcHsize = 640;	// FIXME, the OV7670's horizontal output pixels.
	pdev->srcVsize = 480;	// FIXME, the OV7670's verical output pixels.

	/* the windowed image size. */
	pdev->wndHsize = 640;
	pdev->wndVsize = 480;

	/* codec-path target(output) image size. */
	pdev->coTargetHsize = pdev->wndHsize;
	pdev->coTargetVsize = pdev->wndVsize;

	/* preview-path target(preview) image size. */
	pdev->preTargetHsize = 320;
	pdev->preTargetVsize = 240;

	update_camif_config(fh, CAMIF_CMD_STOP);  //call update_camif_regs()
}

static void __inline__ invalid_image_buffer(void)
{
	img_buff[0].state = CAMIF_BUFF_INVALID;
	img_buff[1].state = CAMIF_BUFF_INVALID;
	img_buff[2].state = CAMIF_BUFF_INVALID;
	img_buff[3].state = CAMIF_BUFF_INVALID;
}

/* free image buffers (only when the camif is latest close). */
static void __inline__ free_image_buffer(void)
{
	free_pages(img_buff[0].virt_base, img_buff[0].order);
	free_pages(img_buff[1].virt_base, img_buff[1].order);
	free_pages(img_buff[2].virt_base, img_buff[2].order);
	free_pages(img_buff[3].virt_base, img_buff[3].order);

	img_buff[0].order = 0;
	img_buff[0].virt_base = (unsigned long)NULL;
	img_buff[0].phy_base = (unsigned long)NULL;

	img_buff[1].order = 0;
	img_buff[1].virt_base = (unsigned long)NULL;
	img_buff[1].phy_base = (unsigned long)NULL;

	img_buff[2].order = 0;
	img_buff[2].virt_base = (unsigned long)NULL;
	img_buff[2].phy_base = (unsigned long)NULL;

	img_buff[3].order = 0;
	img_buff[3].virt_base = (unsigned long)NULL;
	img_buff[3].phy_base = (unsigned long)NULL;
}

/* init image buffer (only when the camif is first open). */
static int __inline__ init_image_buffer(void)
{
	int size1, size2;
	unsigned long size;
	unsigned int order;

	/* size1 is the max image size of codec path. */
	size1 = MAX_C_WIDTH * MAX_C_HEIGHT * 16 / 8;

	/* size2 is the max image size of preview path. */
	size2 = MAX_P_WIDTH * MAX_P_HEIGHT * 16 / 8;

	size = (size1 > size2)?size1:size2;

	order = get_order(size);
	img_buff[0].order = order;
	img_buff[0].virt_base = __get_free_pages(GFP_KERNEL|GFP_DMA, img_buff[0].order);
	if (img_buff[0].virt_base == (unsigned long)NULL)
	{
		goto error0;
	}
	img_buff[0].phy_base = img_buff[0].virt_base - PAGE_OFFSET + PHYS_OFFSET;	// the DMA address.
	#if DEBUG
	printk(DEVICE_NAME"----img_buff[0]'s DMA address is %lx\n",img_buff[0].phy_base);
	#endif

	img_buff[1].order = order;
	img_buff[1].virt_base = __get_free_pages(GFP_KERNEL|GFP_DMA, img_buff[1].order);
	if (img_buff[1].virt_base == (unsigned long)NULL)
	{
		goto error1;
	}
	img_buff[1].phy_base = img_buff[1].virt_base - PAGE_OFFSET + PHYS_OFFSET;	// the DMA address.
	#if DEBUG
	printk(DEVICE_NAME"----img_buff[1]'s DMA address is %lx\n",img_buff[1].phy_base);
	#endif

	img_buff[2].order = order;
	img_buff[2].virt_base = __get_free_pages(GFP_KERNEL|GFP_DMA, img_buff[2].order);
	if (img_buff[2].virt_base == (unsigned long)NULL)
	{
		goto error2;
	}
	img_buff[2].phy_base = img_buff[2].virt_base - PAGE_OFFSET + PHYS_OFFSET;	// the DMA address.
	#if DEBUG
	printk(DEVICE_NAME"----img_buff[2]'s DMA address is %lx\n",img_buff[2].phy_base);
	#endif


	img_buff[3].order = order;
	img_buff[3].virt_base = __get_free_pages(GFP_KERNEL|GFP_DMA, img_buff[3].order);
	if (img_buff[3].virt_base == (unsigned long)NULL)
	{
		goto error3;
	}
	img_buff[3].phy_base = img_buff[3].virt_base - PAGE_OFFSET + PHYS_OFFSET;	// the DMA address.
	#if DEBUG
	printk(DEVICE_NAME"----img_buff[3]'s DMA address is %lx\n",img_buff[3].phy_base);
	#endif



	invalid_image_buffer();

	return 0;
error3:
	free_pages(img_buff[2].virt_base, order);
	img_buff[2].phy_base = (unsigned long)NULL;
error2:
	free_pages(img_buff[1].virt_base, order);
	img_buff[1].phy_base = (unsigned long)NULL;
error1:
	free_pages(img_buff[0].virt_base, order);
	img_buff[0].phy_base = (unsigned long)NULL;
error0:
	return -ENOMEM;
}

/* software reset camera interface. */
static void __inline__ soft_reset_camif(void)
{
	u32 cigctrl;

	cigctrl = (1<<31)|(1<<29);
	iowrite32(cigctrl, S3C244X_CIGCTRL);
	mdelay(10);

	cigctrl = (1<<29);
	iowrite32(cigctrl, S3C244X_CIGCTRL);
	mdelay(10);

}

/* software reset camera interface. */
static void __inline__ hw_reset_camif(void)
{
	u32 cigctrl;

	cigctrl = (1<<30)|(1<<29);
	iowrite32(cigctrl, S3C244X_CIGCTRL);
	mdelay(10);

	cigctrl = (1<<29);
	iowrite32(cigctrl, S3C244X_CIGCTRL);
	mdelay(10);

}

/* switch camif from codec path to preview path. */
static void __inline__ camif_c2p(struct ov7670_camif_dev * pdev)
{
	/* 1. stop codec. */
	{
		u32 cicoscctrl;
		cicoscctrl = ioread32(S3C244X_CICOSCCTRL);
		cicoscctrl &= ~(1<<15);	// stop preview scaler.
		iowrite32(cicoscctrl, S3C244X_CICOSCCTRL);
	}

	/* 2. soft-reset camif. */
	soft_reset_camif();

	/* 3. clear all overflow. */
	{
		u32 ciwdofst;
		ciwdofst = ioread32(S3C244X_CIWDOFST);
		ciwdofst |= (1<<30)|(1<<15)|(1<<14)|(1<<13)|(1<<12);
		iowrite32(ciwdofst, S3C244X_CIWDOFST);

		ciwdofst &= ~((1<<30)|(1<<15)|(1<<14)|(1<<13)|(1<<12));
		iowrite32(ciwdofst, S3C244X_CIWDOFST);
	}
}

/* stop image capture, always called in ISR.
 *	P-path regs:
 *		CIPRSCCTRL
 *		CIPRCTRL
 *	C-path regs:
 *		CICOSCCTRL.
 *		CICOCTRL
 *	Global regs:
 *		CIIMGCPT
 */
static void stop_capture(struct ov7670_camif_dev * pdev)
{
	u32 ciprscctrl;
	u32 ciprctrl;

	u32 cicoscctrl;
	u32 cicoctrl;

	switch(pdev->state)
	{
	case CAMIF_STATE_PREVIEWING:
		/* CIPRCTRL. */
		ciprctrl = ioread32(S3C244X_CIPRCTRL);
		ciprctrl |= 1<<2;		// enable last IRQ at the end of frame capture.
		iowrite32(ciprctrl, S3C244X_CIPRCTRL);

		/* CIPRSCCTRL. */
		ciprscctrl = ioread32(S3C244X_CIPRSCCTRL);
		ciprscctrl &= ~(1<<15);		// clear preview scaler start bit.
		iowrite32(ciprscctrl, S3C244X_CIPRSCCTRL);

		/* CIIMGCPT. */
		iowrite32(0, S3C244X_CIIMGCPT);
		pdev->state = CAMIF_STATE_READY;

		break;

	case CAMIF_STATE_CODECING:
		/* CICOCTRL. */
		cicoctrl = ioread32(S3C244X_CICOCTRL);
		cicoctrl |= 1<<2;		// enable last IRQ at the end of frame capture.
		iowrite32(cicoctrl, S3C244X_CICOCTRL);

		/* CICOSCCTRL. */
		cicoscctrl = ioread32(S3C244X_CICOSCCTRL);
		cicoscctrl &= ~(1<<15);		// clear codec scaler start bit.
		iowrite32(cicoscctrl, S3C244X_CICOSCCTRL);

		/* CIIMGCPT. */
		iowrite32(0, S3C244X_CIIMGCPT);
		pdev->state = CAMIF_STATE_READY;

		break;

	}
}


/*
 * ISR: service for C-path interrupt.
 */
static irqreturn_t on_camif_irq_c(int irq, void * dev)
{
	u32 cicostatus;

	u32 frame;
	struct ov7670_camif_dev * pdev;

	cicostatus = ioread32(S3C244X_CICOSTATUS);
	#if DEBUG
	printk("----on_camif_irq_c is call\n");
	printk(DEVICE_NAME"----the S3C244X_CICOSTATUS is %x\n",cicostatus);
        #endif

	/*编码通道图像捕捉是否使能*/
	if ((cicostatus & (1<<21))== 0)
	{
		return IRQ_RETVAL(IRQ_NONE);
	}

	pdev = (struct ov7670_camif_dev *)dev;

	/* valid img_buff[x] just DMAed. */
	frame = (cicostatus&(3<<26))>>26;
	frame = (frame+4-1)%4;

	/*pdev->cmdcode 在update_camif_config()中被赋值*/
	if (pdev->cmdcode & CAMIF_CMD_STOP)
	{
		stop_capture(pdev);

		pdev->state = CAMIF_STATE_READY;
	}
	else
	{
		if (pdev->cmdcode & CAMIF_CMD_C2P)
		{
			camif_c2p(pdev);
		}

		if (pdev->cmdcode & CAMIF_CMD_WND)
		{
			update_target_wnd_regs(pdev);
		}

		if (pdev->cmdcode & CAMIF_CMD_TFMT)
		{
			update_target_fmt_regs(pdev);
		}

		if (pdev->cmdcode & CAMIF_CMD_ZOOM)
		{
			update_target_zoom_regs(pdev);
		}

		invalid_image_buffer();
	}
	pdev->cmdcode = CAMIF_CMD_NONE;
	wake_up(&pdev->cmdqueue);

	return IRQ_RETVAL(IRQ_HANDLED);
}

/*
 * ISR: service for P-path interrupt.
 */
static irqreturn_t on_camif_irq_p(int irq, void * dev)
{
	u32 ciprstatus;

	u32 frame;
	struct ov7670_camif_dev * pdev;
	ciprstatus = ioread32(S3C244X_CIPRSTATUS);

	#if DEBUG
    printk(DEVICE_NAME"----------on camif_irq_p is called\n");
	printk(DEVICE_NAME"----the S3C244X_CIPRSTATUS is %x\n",ciprstatus);
        #endif


	if ((ciprstatus & (1<<21))== 0)
	{
		return IRQ_RETVAL(IRQ_NONE);
	}

	pdev = (struct ov7670_camif_dev *)dev;

	/* valid img_buff[x] just DMAed. */
	frame = (ciprstatus&(3<<26))>>26;
	frame = (frame+4-1)%4;

	img_buff[frame].state = CAMIF_BUFF_RGB565;

	if (pdev->cmdcode & CAMIF_CMD_STOP)
	{
		stop_capture(pdev);

		pdev->state = CAMIF_STATE_READY;
	}
	else
	{
		if (pdev->cmdcode & CAMIF_CMD_P2C)
		{
			camif_c2p(pdev);
		}

		if (pdev->cmdcode & CAMIF_CMD_WND)
		{
			update_target_wnd_regs(pdev);
		}

		if (pdev->cmdcode & CAMIF_CMD_TFMT)
		{
			update_target_fmt_regs(pdev);
		}

		if (pdev->cmdcode & CAMIF_CMD_ZOOM)
		{
			update_target_zoom_regs(pdev);
		}

		invalid_image_buffer();
	}
	pdev->cmdcode = CAMIF_CMD_NONE;
	wake_up(&pdev->cmdqueue);

	return IRQ_RETVAL(IRQ_HANDLED);
}



/*
 * camif_open()
 */
static int camif_open(struct inode *inode, struct file *file)
{
       int ret;
       struct ov7670_camif_dev *pdev;
       struct ov7670_camif_fh *fh;
       if(!has_ov7670)
       {
	        return -ENODEV;
       }
       pdev = &camera;

	fh = kzalloc(sizeof(*fh),GFP_KERNEL); // alloc memory for filehandle
	if (NULL == fh)
	{
		return -ENOMEM;
	}
	fh->dev = pdev;

	pdev->state = CAMIF_STATE_READY;

	init_camif_config(fh);

	ret = init_image_buffer();	// init image buffer.
	if (ret < 0)
	{
		goto error1;
	}

	/*request irq for c-path*/
        request_irq(IRQ_S3C2440_CAM_C, on_camif_irq_c, IRQF_DISABLED, "CAM_C", pdev);	// setup ISRs
	if (ret < 0)
	{
		goto error2;
	}

	/*request irq for p-path*/
	request_irq(IRQ_S3C2440_CAM_P, on_camif_irq_p, IRQF_DISABLED, "CAM_P", pdev);
	if (ret < 0)
	{
		goto error3;
	}

	clk_enable(pdev->clk);		// and enable camif clock.


	soft_reset_camif();

	file->private_data = fh;
	fh->dev  = pdev;
	update_camif_config(fh, 0);
	return 0;

error3:
	free_irq(IRQ_S3C2440_CAM_C, pdev);
error2:
	free_image_buffer();
error1:
	kfree(fh);
	return ret;
}


/*
 * camif_release()
 */
static int camif_release(struct inode *inode, struct file *file)
{
    return 0;
}


/* start image capture.
 *
 * param 'stream' means capture pictures streamly or capture only one picture.
 */
static int start_capture(struct ov7670_camif_dev * pdev, int stream)
{
	int ret;

	u32 ciwdofst;
	u32 ciprscctrl;
	u32 ciimgcpt;

	ciwdofst = ioread32(S3C244X_CIWDOFST);

	#if DEBUG
	printk(DEVICE_NAME"--------the S3C244X_CIWDOFST is %x\n",ciwdofst);
        #endif
	ciwdofst	|= (1<<30)	// Clear the overflow indication flag of input CODEC FIFO Y
		|(1<<15)		// Clear the overflow indication flag of input CODEC FIFO Cb
		|(1<<14)		// Clear the overflow indication flag of input CODEC FIFO Cr
		|(1<<13)		// Clear the overflow indication flag of input PREVIEW FIFO Cb
		|(1<<12);		// Clear the overflow indication flag of input PREVIEW FIFO Cr

	iowrite32(ciwdofst, S3C244X_CIWDOFST);
	mdelay(1);

	ciwdofst = ioread32(S3C244X_CIWDOFST);
        #if DEBUG
	printk(DEVICE_NAME"----------the S3C244X_CIWDOFST is %x\n",ciwdofst);
        #endif

	ciprscctrl = ioread32(S3C244X_CIPRSCCTRL);
        #if DEBUG
	printk(DEVICE_NAME"--------the S3C244X_CIPRSCCTRL is %x\n",ciprscctrl);
        #endif

	ciprscctrl |= 1<<15;    	// preview scaler start
	iowrite32(ciprscctrl, S3C244X_CIPRSCCTRL);

	pdev->state = CAMIF_STATE_PREVIEWING;

	ciimgcpt = (1<<31)		// camera interface global capture enable
		 |(1<<29);		// capture enable for preview scaler.
	iowrite32(ciimgcpt, S3C244X_CIIMGCPT);

	ret = 0;
	if (stream == 0)
	{
		pdev->cmdcode = CAMIF_CMD_STOP;

		ret = wait_event_interruptible(pdev->cmdqueue, pdev->cmdcode == CAMIF_CMD_NONE);
	}

	return ret;
}






/*
 * camif_read()
 */
static ssize_t camif_read(struct file *file, char __user *data, size_t count, loff_t *ppos)
{
	int i;
	struct ov7670_camif_fh * fh;
	struct ov7670_camif_dev * pdev;

	fh = file->private_data;
	pdev = fh->dev;

         /*开启stream*/
	if (start_capture(pdev, 0) != 0)
	{
		return -ERESTARTSYS;
	}


	disable_irq(IRQ_S3C2440_CAM_C);
	disable_irq(IRQ_S3C2440_CAM_P);
	for (i = 0; i < 4; i++)
	{
		if (img_buff[i].state != CAMIF_BUFF_INVALID)
		{
		    copy_to_user(data, (void *)img_buff[i].virt_base, count); /*将img_buff的虚拟地址复制到user*/
			img_buff[i].state = CAMIF_BUFF_INVALID;
		}
	}
	enable_irq(IRQ_S3C2440_CAM_P);
	enable_irq(IRQ_S3C2440_CAM_C);

	return count;
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


    #if DEBUG
    printk(DEVICE_NAME"----seting gpio-j to camera mode.......\n");
    #endif

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

    #if DEBUG
    printk(DEVICE_NAME"----enable the clock and rate it.......\n");
    #endif


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
	hw_reset_camif();

        has_ov7670 = s3c2440_ov7670_init() >= 0;

       /*给lcd上电*/
	s3c2410_gpio_setpin(S3C2410_GPG4, 1);

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
	return ret;
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

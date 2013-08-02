#ifndef __S3C2440CAMIF_H__
#define __S3C2440CAMIF_H__


#define OV7670_SCCB_ADDR	0x42
#define OV7670_MANUFACT_ID	0x7FA2
#define OV7670_PRODUCT_ID	0x7673



/* Registers from /driver/meida/video/ov7670.c */
#define REG_GAIN	0x00	/* Gain lower 8 bits (rest in vref) */
#define REG_BLUE	0x01	/* blue gain */
#define REG_RED		0x02	/* red gain */
#define REG_VREF	0x03	/* Pieces of GAIN, VSTART, VSTOP */
#define REG_COM1	0x04	/* Control 1 */
#define  COM1_CCIR656	  0x40  /* CCIR656 enable */
#define REG_BAVE	0x05	/* U/B Average level */
#define REG_GbAVE	0x06	/* Y/Gb Average level */
#define REG_AECHH	0x07	/* AEC MS 5 bits */
#define REG_RAVE	0x08	/* V/R Average level */
#define REG_COM2	0x09	/* Control 2 */
#define  COM2_SSLEEP	  0x10	/* Soft sleep mode */
#define REG_PID		0x0a	/* Product ID MSB */
#define REG_VER		0x0b	/* Product ID LSB */
#define REG_COM3	0x0c	/* Control 3 */
#define  COM3_SWAP	  0x40	  /* Byte swap */
#define  COM3_SCALEEN	  0x08	  /* Enable scaling */
#define  COM3_DCWEN	  0x04	  /* Enable downsamp/crop/window */
#define REG_COM4	0x0d	/* Control 4 */
#define REG_COM5	0x0e	/* All "reserved" */
#define REG_COM6	0x0f	/* Control 6 */
#define REG_AECH	0x10	/* More bits of AEC value */
#define REG_CLKRC	0x11	/* Clocl control */
#define   CLK_EXT	  0x40	  /* Use external clock directly */
#define   CLK_SCALE	  0x3f	  /* Mask for internal clock scale */
#define REG_COM7	0x12	/* Control 7 */
#define   COM7_RESET	  0x80	  /* Register reset */
#define   COM7_FMT_MASK	  0x38
#define   COM7_FMT_VGA	  0x00
#define	  COM7_FMT_CIF	  0x20	  /* CIF format */
#define   COM7_FMT_QVGA	  0x10	  /* QVGA format */
#define   COM7_FMT_QCIF	  0x08	  /* QCIF format */
#define	  COM7_RGB	  0x04	  /* bits 0 and 2 - RGB format */
#define	  COM7_YUV	  0x00	  /* YUV */
#define	  COM7_BAYER	  0x01	  /* Bayer format */
#define	  COM7_PBAYER	  0x05	  /* "Processed bayer" */
#define REG_COM8	0x13	/* Control 8 */
#define   COM8_FASTAEC	  0x80	  /* Enable fast AGC/AEC */
#define   COM8_AECSTEP	  0x40	  /* Unlimited AEC step size */
#define   COM8_BFILT	  0x20	  /* Band filter enable */
#define   COM8_AGC	  0x04	  /* Auto gain enable */
#define   COM8_AWB	  0x02	  /* White balance enable */
#define   COM8_AEC	  0x01	  /* Auto exposure enable */
#define REG_COM9	0x14	/* Control 9  - gain ceiling */
#define REG_COM10	0x15	/* Control 10 */
#define   COM10_HSYNC	  0x40	  /* HSYNC instead of HREF */
#define   COM10_PCLK_HB	  0x20	  /* Suppress PCLK on horiz blank */
#define   COM10_HREF_REV  0x08	  /* Reverse HREF */
#define   COM10_VS_LEAD	  0x04	  /* VSYNC on clock leading edge */
#define   COM10_VS_NEG	  0x02	  /* VSYNC negative */
#define   COM10_HS_NEG	  0x01	  /* HSYNC negative */
#define REG_HSTART	0x17	/* Horiz start high bits */
#define REG_HSTOP	0x18	/* Horiz stop high bits */
#define REG_VSTART	0x19	/* Vert start high bits */
#define REG_VSTOP	0x1a	/* Vert stop high bits */
#define REG_PSHFT	0x1b	/* Pixel delay after HREF */
#define REG_MIDH	0x1c	/* Manuf. ID high */
#define REG_MIDL	0x1d	/* Manuf. ID low */
#define REG_MVFP	0x1e	/* Mirror / vflip */
#define   MVFP_MIRROR	  0x20	  /* Mirror image */
#define   MVFP_FLIP	  0x10	  /* Vertical flip */

#define REG_AEW		0x24	/* AGC upper limit */
#define REG_AEB		0x25	/* AGC lower limit */
#define REG_VPT		0x26	/* AGC/AEC fast mode op region */
#define REG_HSYST	0x30	/* HSYNC rising edge delay */
#define REG_HSYEN	0x31	/* HSYNC falling edge delay */
#define REG_HREF	0x32	/* HREF pieces */
#define REG_TSLB	0x3a	/* lots of stuff */
#define   TSLB_YLAST	  0x04	  /* UYVY or VYUY - see com13 */
#define REG_COM11	0x3b	/* Control 11 */
#define   COM11_NIGHT	  0x80	  /* NIght mode enable */
#define   COM11_NMFR	  0x60	  /* Two bit NM frame rate */
#define   COM11_HZAUTO	  0x10	  /* Auto detect 50/60 Hz */
#define	  COM11_50HZ	  0x08	  /* Manual 50Hz select */
#define   COM11_EXP	  0x02
#define REG_COM12	0x3c	/* Control 12 */
#define   COM12_HREF	  0x80	  /* HREF always */
#define REG_COM13	0x3d	/* Control 13 */
#define   COM13_GAMMA	  0x80	  /* Gamma enable */
#define	  COM13_UVSAT	  0x40	  /* UV saturation auto adjustment */
#define   COM13_UVSWAP	  0x01	  /* V before U - w/TSLB */
#define REG_COM14	0x3e	/* Control 14 */
#define   COM14_DCWEN	  0x10	  /* DCW/PCLK-scale enable */
#define REG_EDGE	0x3f	/* Edge enhancement factor */
#define REG_COM15	0x40	/* Control 15 */
#define   COM15_R10F0	  0x00	  /* Data range 10 to F0 */
#define	  COM15_R01FE	  0x80	  /*            01 to FE */
#define   COM15_R00FF	  0xc0	  /*            00 to FF */
#define   COM15_RGB565	  0x10	  /* RGB565 output */
#define   COM15_RGB555	  0x30	  /* RGB555 output */
#define REG_COM16	0x41	/* Control 16 */
#define   COM16_AWBGAIN   0x08	  /* AWB gain enable */
#define REG_COM17	0x42	/* Control 17 */
#define   COM17_AECWIN	  0xc0	  /* AEC window - must match COM4 */
#define   COM17_CBAR	  0x08	  /* DSP Color bar */

/*
 * This matrix defines how the colors are generated, must be
 * tweaked to adjust hue and saturation.
 *
 * Order: v-red, v-green, v-blue, u-red, u-green, u-blue
 *
 * They are nine-bit signed quantities, with the sign bit
 * stored in 0x58.  Sign for v-red is bit 0, and up from there.
 */
#define	REG_CMATRIX_BASE 0x4f
#define   CMATRIX_LEN 6
#define REG_CMATRIX_SIGN 0x58


#define REG_BRIGHT	0x55	/* Brightness */
#define REG_CONTRAS	0x56	/* Contrast control */

#define REG_GFIX	0x69	/* Fix gain control */

#define REG_REG76	0x76	/* OV's name */
#define   R76_BLKPCOR	  0x80	  /* Black pixel correction enable */
#define   R76_WHTPCOR	  0x40	  /* White pixel correction enable */

#define REG_RGB444	0x8c	/* RGB 444 control */
#define   R444_ENABLE	  0x02	  /* Turn on RGB444, overrides 5x5 */
#define   R444_RGBX	  0x01	  /* Empty nibble at end */

#define REG_HAECC1	0x9f	/* Hist AEC/AGC control 1 */
#define REG_HAECC2	0xa0	/* Hist AEC/AGC control 2 */

#define REG_BD50MAX	0xa5	/* 50hz banding step limit */
#define REG_HAECC3	0xa6	/* Hist AEC/AGC control 3 */
#define REG_HAECC4	0xa7	/* Hist AEC/AGC control 4 */
#define REG_HAECC5	0xa8	/* Hist AEC/AGC control 5 */
#define REG_HAECC6	0xa9	/* Hist AEC/AGC control 6 */
#define REG_HAECC7	0xaa	/* Hist AEC/AGC control 7 */
#define REG_BD60MAX	0xab	/* 60hz banding step limit */


/*定义SIO_C和SIO_D管脚*/
#define SIO_C		S3C2410_GPE14   //i2cscl
#define SIO_D		S3C2410_GPE15   //i2csda

/*保持，拉高，拉低*/
#define State(x)		s3c2410_gpio_getpin(x)
#define High(x)		do{s3c2410_gpio_setpin(x,1); smp_mb();}while(0)
#define Low(x)		do{s3c2410_gpio_setpin(x,0); smp_mb();}while(0)

/*延时*/
#define WAIT_STABLE()	do{udelay(10);}while(0)
#define WAIT_CYCLE()	do{udelay(90);}while(0)

/*set gpio to input or output*/
#define CFG_READ(x)	do{s3c2410_gpio_cfgpin(x,S3C2410_GPIO_INPUT);smp_mb();}while(0)
#define CFG_WRITE(x)	do{s3c2410_gpio_cfgpin(x,S3C2410_GPIO_OUTPUT);smp_mb();}while(0)


DECLARE_MUTEX(regs_mutex);

static DECLARE_MUTEX(bus_lock);
static struct ov7670_reg
{
	u8 subaddr;
	u8 value;
}
regs[] = {
	{ REG_COM7, COM7_RESET },
/*
 * Clock scale: 3 = 15fps
 *              2 = 20fps
 *              1 = 30fps
 */

        { REG_CLKRC, 0x1 },	/* OV: clock scale (30 fps) */
	{ REG_TSLB,  0x04 },	/* OV */
	{ REG_COM7, 0x00 },	/* VGA */
	/*
	 * Set the hardware window.  These values from OV don't entirely
	 * make sense - hstop is less than hstart.  But they work...
	 */
	{ REG_HSTART, 0x13 },	{ REG_HSTOP, 0x01 },
	{ REG_HREF, 0xb6 },	{ REG_VSTART, 0x02 },
	{ REG_VSTOP, 0x7a },	{ REG_VREF, 0x0a },

	{ REG_COM3, 0 },	{ REG_COM14, 0 },
	/* Mystery scaling numbers */
	{ 0x70, 0x3a },		{ 0x71, 0x35 },
	{ 0x72, 0x11 },		{ 0x73, 0xf0 },
	{ 0xa2, 0x02 },		{ REG_COM10, 0x0 },

/* Gamma curve values */
	{ 0x7a, 0x20 },		{ 0x7b, 0x10 },
	{ 0x7c, 0x1e },		{ 0x7d, 0x35 },
	{ 0x7e, 0x5a },		{ 0x7f, 0x69 },
	{ 0x80, 0x76 },		{ 0x81, 0x80 },
	{ 0x82, 0x88 },		{ 0x83, 0x8f },
	{ 0x84, 0x96 },		{ 0x85, 0xa3 },
	{ 0x86, 0xaf },		{ 0x87, 0xc4 },
	{ 0x88, 0xd7 },		{ 0x89, 0xe8 },

	/* AGC and AEC parameters.  Note we start by disabling those features,
	   then turn them only after tweaking the values. */
	{ REG_COM8, COM8_FASTAEC | COM8_AECSTEP | COM8_BFILT },
	{ 0x01, 0x58 }, { 0x02, 0x68 },
	{ REG_GAIN, 0 },	{ REG_AECH, 0 },
	{ REG_COM4, 0x40 }, /* magic reserved bit */
	{ REG_COM9, 0x18 }, /* 4x gain + magic rsvd bit */
	{ REG_BD50MAX, 0x05 },	{ REG_BD60MAX, 0x07 },
	{ REG_AEW, 0x95 },	{ REG_AEB, 0x33 },
	{ REG_VPT, 0xe3 },	{ REG_HAECC1, 0x78 },
	{ REG_HAECC2, 0x68 },	{ 0xa1, 0x03 }, /* magic */
	{ REG_HAECC3, 0xd8 },	{ REG_HAECC4, 0xd8 },
	{ REG_HAECC5, 0xf0 },	{ REG_HAECC6, 0x90 },
	{ REG_HAECC7, 0x94 },
	{ REG_COM8, COM8_FASTAEC|COM8_AECSTEP|COM8_BFILT|COM8_AGC|COM8_AEC },

	/* Almost all of these are magic "reserved" values.  */
	{ REG_COM5, 0x61 },	{ REG_COM6, 0x4b },
	{ 0x16, 0x02 },		{ REG_MVFP, 0x27 },
	{ 0x21, 0x02 },		{ 0x22, 0x91 },
	{ 0x29, 0x07 },		{ 0x33, 0x0b },
	{ 0x35, 0x0b },		{ 0x37, 0x1d },
	{ 0x38, 0x71 },		{ 0x39, 0x2a },
	{ REG_COM12, 0x78 },	{ 0x4d, 0x40 },
	{ 0x4e, 0x20 },		{ REG_GFIX, 0 },
	{ 0x6b, 0x0a },		{ 0x74, 0x10 },
	{ 0x8d, 0x4f },		{ 0x8e, 0 },
	{ 0x8f, 0 },	{0x92, 19 },	{ 0x90, 0 },
	{ 0x91, 0 }, 		{ 0x96, 0 },
	{ 0x9a, 80 },		{ 0xb0, 0x84 },
	{ 0xb1, 0x0c },		{ 0xb2, 0x0e },
	{ 0xb3, 0x82 },		{ 0xb8, 0x0a },


	/* More reserved magic, some of which tweaks white balance */
	{ 0x43, 0x14 },		{ 0x44, 0xf0 },
	{ 0x45, 0x34 },		{ 0x46, 0x58 },
	{ 0x47, 0x28 },		{ 0x48, 0x3a },
	{ 0x59, 0x88 },		{ 0x5a, 0x88 },
	{ 0x5b, 0x44 },		{ 0x5c, 0x67 },
	{ 0x5d, 0x49 },		{ 0x5e, 0x0e }, { 0x64, 0x04 }, { 0x65, 0x20 }, { 0x66, 0x05 }, { 0x94, 0x04 }, { 0x95, 0x08 },
	{ 0x6c, 0x0a },		{ 0x6d, 0x55 },
	{ 0x6e, 0x11 },		{ 0x6f, 0x9f }, /* "9e for advance AWB" */
	{ 0x6a, 0x40 },		{ REG_BLUE, 0x40 },
	{ REG_RED, 0x40 },
	{ REG_COM8, COM8_FASTAEC|COM8_AECSTEP|COM8_BFILT|COM8_AGC|COM8_AEC|COM8_AWB },



	/* Matrix coefficients */
	/*
	{ 0x4f, 0x80 },		{ 0x50, 0x80 },
	{ 0x51, 0 },		{ 0x52, 0x22 },
	{ 0x53, 0x5e },		{ 0x54, 0x80 },
	{ 0x58, 0x9e },

	{ REG_COM16, COM16_AWBGAIN },	{ REG_EDGE, 0 },
	{ 0x75, 0x04 },		{ 0x76, 0xe1 },
	{ 0x4c, 0 },		{ 0x77, 0x01 },
	{ REG_COM13, 0xc2 },	{ 0x4b, 0x09 },
	{ 0xc9, 0x60 },		{ REG_COM16, 0x38 },
	{ 0x56, 0x40 },

	{ 0x34, 0x11 },		{ REG_COM11, 0x0a },
	{ 0xa4, 0x88 },		{ 0x96, 0 },
	{ 0x97, 0x30 },		{ 0x98, 0x20 },
	{ 0x99, 0x30 },		{ 0x9a, 0x84 },
	{ 0x9b, 0x29 },		{ 0x9c, 0x03 },
	{ 0x9d, 0x4c },		{ 0x9e, 0x3f },
	{ 0x78, 0x04 },
	*/

	/* Extra-weird stuff.  Some sort of multiplexor register */
	{ 0x79, 0x01 },		{ 0xc8, 0xf0 },
	{ 0x79, 0x0f },		{ 0xc8, 0x00 },
	{ 0x79, 0x10 },		{ 0xc8, 0x7e },
	{ 0x79, 0x0a },		{ 0xc8, 0x80 },
	{ 0x79, 0x0b },		{ 0xc8, 0x01 },
	{ 0x79, 0x0c },		{ 0xc8, 0x0f },
	{ 0x79, 0x0d },		{ 0xc8, 0x20 },
	{ 0x79, 0x09 },		{ 0xc8, 0x80 },
	{ 0x79, 0x02 },		{ 0xc8, 0xc0 },
	{ 0x79, 0x03 },		{ 0xc8, 0x40 },
	{ 0x79, 0x05 },		{ 0xc8, 0x30 },
	{ 0x79, 0x26 },

	/*Set YUV output*/
	{ REG_COM7, 0x0 },  /* Selects YUV mode */
	{ REG_RGB444, 0 },	/* No RGB444 please */
	{ REG_COM1, 0 },
	{ REG_COM15, 0xc0},
	{ REG_COM9, 0x18 }, /* 4x gain ceiling; 0x8 is reserved bit */
	{ 0x4f, 0x80 }, 	/* "matrix coefficient 1" */
	{ 0x50, 0x80 }, 	/* "matrix coefficient 2" */
	{ 0x51, 0    },		/* vb */
	{ 0x52, 0x22 }, 	/* "matrix coefficient 4" */
	{ 0x53, 0x5e }, 	/* "matrix coefficient 5" */
	{ 0x54, 0x80 }, 	/* "matrix coefficient 6" */
	{ REG_COM13, COM13_GAMMA|COM13_UVSAT },


//	{REG_COM15,0xc1}, /*01 - fe*/
////////////////////////////////////////////////////////////////////

	{0xff,0xff},
};


/*-------------------------------------------------*/
/*
 * Basic window sizes.  These probably belong somewhere more globally
 * useful.
 */
#define VGA_WIDTH	640
#define VGA_HEIGHT	480
#define QVGA_WIDTH	320
#define QVGA_HEIGHT	240
#define CIF_WIDTH	352
#define CIF_HEIGHT	288
#define QCIF_WIDTH	176
#define	QCIF_HEIGHT	144



static struct ov7670_win_size {
	int	width;
	int	height;
	unsigned char com7_bit;
	int	hstart;		/* Start/stop values for the camera.  Note */
	int	hstop;		/* that they do not always make complete */
	int	vstart;		/* sense to humans, but evidently the sensor */
	int	vstop;		/* will do the right thing... */
	struct regval_list *regs; /* Regs to tweak */
/* h/vref stuff */
} ov7670_win_sizes[] = {
	/* VGA */
	{
		.width		= VGA_WIDTH,
		.height		= VGA_HEIGHT,
		.com7_bit	= COM7_FMT_VGA,
		.hstart		= 158,		/* These values from */
		.hstop		=  14,		/* Omnivision */
		.vstart		=  10,
		.vstop		= 490,
		.regs 		= NULL,
	},
	/* CIF */
	{
		.width		= CIF_WIDTH,
		.height		= CIF_HEIGHT,
		.com7_bit	= COM7_FMT_CIF,
		.hstart		= 170,		/* Empirically determined */
		.hstop		=  90,
		.vstart		=  14,
		.vstop		= 494,
		.regs 		= NULL,
	},
	/* QVGA */
	{
		.width		= QVGA_WIDTH,
		.height		= QVGA_HEIGHT,
		.com7_bit	= COM7_FMT_QVGA,
		.hstart		= 164,		/* Empirically determined */
		.hstop		=  20,
		.vstart		=  14,
		.vstop		= 494,
		.regs 		= NULL,
	},
	/* QCIF */
	{
		.width		= QCIF_WIDTH,
		.height		= QCIF_HEIGHT,
		.com7_bit	= COM7_FMT_VGA, /* see comment above */
		.hstart		= 456,		/* Empirically determined */
		.hstop		=  24,
		.vstart		=  14,
		.vstop		= 494,
	},
};

#define N_WIN_SIZES (ARRAY_SIZE(ov7670_win_sizes))


/*---------------------------------------------------------------*/

#define MIN_C_WIDTH		32
#define MIN_C_HEIGHT		48
#define MAX_C_WIDTH		640
#define MAX_C_HEIGHT		480

#define MIN_P_WIDTH		32
#define MIN_P_HEIGHT		48
#define MAX_P_WIDTH		640
#define MAX_P_HEIGHT		480

enum
{
	CAMIF_BUFF_INVALID = 0,
	CAMIF_BUFF_RGB565 = 1,
	CAMIF_BUFF_RGB24 = 2,
	CAMIF_BUFF_YCbCr420 = 3,
	CAMIF_BUFF_YCbCr422 = 4,
};

/* image buffer for s3c2440 camif. */
struct ov7670_camif_buffer
{
	int state;
	ssize_t img_size;

	unsigned int order;
	unsigned long virt_base;
	unsigned long phy_base;
};

/* image buffer for previewing. */
struct ov7670_camif_buffer img_buff[] =
{
	{
		.state = CAMIF_BUFF_INVALID,
		.img_size = 0,
		.order = 0,
		.virt_base = (unsigned long)NULL,
		.phy_base = (unsigned long)NULL
	},
	{
		.state = CAMIF_BUFF_INVALID,
		.img_size = 0,
		.order = 0,
		.virt_base = (unsigned long)NULL,
		.phy_base = (unsigned long)NULL
	},
	{
		.state = CAMIF_BUFF_INVALID,
		.img_size = 0,
		.order = 0,
		.virt_base = (unsigned long)NULL,
		.phy_base = (unsigned long)NULL
	},
	{
		.state = CAMIF_BUFF_INVALID,
		.img_size = 0,
		.order = 0,
		.virt_base = (unsigned long)NULL,
		.phy_base = (unsigned long)NULL
	}
};

/* for ov7670_camif_dev->state field. */
enum
{
	CAMIF_STATE_FREE = 0,		// not openned
	CAMIF_STATE_READY = 1,	// openned, but standby
	CAMIF_STATE_PREVIEWING = 2,	// in previewing
	CAMIF_STATE_CODECING = 3	// in capturing
};

/* for ov7670_camif_dev->cmdcode field. */
enum
{
	CAMIF_CMD_NONE	= 0,
	CAMIF_CMD_SFMT	= 1<<0,		// source image format changed.
	CAMIF_CMD_WND	= 1<<1,		// window offset changed.
	CAMIF_CMD_ZOOM	= 1<<2,		// zoom picture in/out
	CAMIF_CMD_TFMT	= 1<<3,		// target image format changed.
	CAMIF_CMD_P2C		= 1<<4,		// need camif switches from p-path to c-path
	CAMIF_CMD_C2P		= 1<<5,		// neet camif switches from c-path to p-path

	CAMIF_CMD_STOP	= 1<<16		// stop capture
};

/* main s3c2440 camif structure. */
struct ov7670_camif_dev
{
    /* for sub-devices */
	struct list_head devlist;

	/* minor device */
	struct video_device * vfd;

      /*hardware clock. */
	struct clk * clk;

	/* reference count. */
	struct mutex rcmutex;
	int rc;

	/* the input images's format select. */
	int input;

	/* source(input) image size. */
	int srcHsize;
	int srcVsize;

	/* windowed image size. */
	int wndHsize;
	int wndVsize;

	/* codec-path target(output) image size. */
	int coTargetHsize;
	int coTargetVsize;

	/* preview-path target(preview) image size. */
	int preTargetHsize;
	int preTargetVsize;

	/* the camera interface state. */
	int state;	// CMAIF_STATE_FREE, CAMIF_STATE_PREVIEWING, CAMIF_STATE_CAPTURING.

	/* for executing camif commands. */
	int cmdcode;				// command code, CAMIF_CMD_START, CAMIF_CMD_CFG, etc.
	wait_queue_head_t cmdqueue;	// wait queue for waiting untile command completed (if in preview or in capturing).
};

/* opened file handle.*/
struct ov7670_camif_fh
{
	/* the camif */
	struct ov7670_camif_dev	* dev;

	/* master flag, only master openner could execute 'set' ioctls. */
	int master;
};


#define S3C244X_CAMIFREG(x) ((x) + camif_base_addr)
//经过remap后，camif_base_addr有个值
/* CAMIF control registers */
#define S3C244X_CISRCFMT		S3C244X_CAMIFREG(0x00)
#define S3C244X_CIWDOFST		S3C244X_CAMIFREG(0x04)
#define S3C244X_CIGCTRL			S3C244X_CAMIFREG(0x08)
#define S3C244X_CICOYSA1		S3C244X_CAMIFREG(0x18)
#define S3C244X_CICOYSA2		S3C244X_CAMIFREG(0x1C)
#define S3C244X_CICOYSA3		S3C244X_CAMIFREG(0x20)
#define S3C244X_CICOYSA4		S3C244X_CAMIFREG(0x24)
#define S3C244X_CICOCBSA1		S3C244X_CAMIFREG(0x28)
#define S3C244X_CICOCBSA2		S3C244X_CAMIFREG(0x2C)
#define S3C244X_CICOCBSA3		S3C244X_CAMIFREG(0x30)
#define S3C244X_CICOCBSA4		S3C244X_CAMIFREG(0x34)
#define S3C244X_CICOCRSA1		S3C244X_CAMIFREG(0x38)
#define S3C244X_CICOCRSA2		S3C244X_CAMIFREG(0x3C)
#define S3C244X_CICOCRSA3		S3C244X_CAMIFREG(0x40)
#define S3C244X_CICOCRSA4		S3C244X_CAMIFREG(0x44)
#define S3C244X_CICOTRGFMT		S3C244X_CAMIFREG(0x48)
#define S3C244X_CICOCTRL		S3C244X_CAMIFREG(0x4C)
#define S3C244X_CICOSCPRERATIO		S3C244X_CAMIFREG(0x50)
#define S3C244X_CICOSCPREDST		S3C244X_CAMIFREG(0x54)
#define S3C244X_CICOSCCTRL		S3C244X_CAMIFREG(0x58)
#define S3C244X_CICOTAREA		S3C244X_CAMIFREG(0x5C)
#define S3C244X_CICOSTATUS		S3C244X_CAMIFREG(0x64)
#define S3C244X_CIPRCLRSA1		S3C244X_CAMIFREG(0x6C)
#define S3C244X_CIPRCLRSA2		S3C244X_CAMIFREG(0x70)
#define S3C244X_CIPRCLRSA3		S3C244X_CAMIFREG(0x74)
#define S3C244X_CIPRCLRSA4		S3C244X_CAMIFREG(0x78)
#define S3C244X_CIPRTRGFMT		S3C244X_CAMIFREG(0x7C)
#define S3C244X_CIPRCTRL		S3C244X_CAMIFREG(0x80)
#define S3C244X_CIPRSCPRERATIO		S3C244X_CAMIFREG(0x84)
#define S3C244X_CIPRSCPREDST		S3C244X_CAMIFREG(0x88)
#define S3C244X_CIPRSCCTRL		S3C244X_CAMIFREG(0x8C)
#define S3C244X_CIPRTAREA		S3C244X_CAMIFREG(0x90)
#define S3C244X_CIPRSTATUS		S3C244X_CAMIFREG(0x98)
#define S3C244X_CIIMGCPT		S3C244X_CAMIFREG(0xA0)


#endif

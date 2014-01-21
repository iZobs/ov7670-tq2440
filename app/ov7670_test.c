/*
 *   TQ2440 camera test program
 *
 *   preview : 320x240 overlay on 320x240 16bpp LCD
 *
 *   TFT LCD size : 320x240
 */
#include <sys/time.h>
#include <sys/types.h>
#include <asm/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <errno.h>

#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/videodev.h>
#include <linux/videodev2.h>
#include <linux/fb.h>

#define PIXFMT_NUM			5
#define INPUT_NUM 			5
#define CTRL_NUM 			100

#define V4L2_DEV_NODE			"/dev/ov7670"
#define FB_DEV_NODE			"/dev/fb/0"

//typedef struct v4l2_input    V_INPUT;
//typedef struct v4l2_format   V_FORMAT;
//typedef struct v4l2_fmtdesc  V_FMTDESC;
//typedef struct v4l2_queryctrl V_QRYCTRL;

typedef struct fb_var_screeninfo F_VINFO;

unsigned int x_lcd_size, y_lcd_size;

static void v4l2_show_on_fb(int fd, char *fbmem, int frames)
{
	int i;
	int ret;
	char preview_buf[240*320*2];
	
	while(1) {
			if ((ret = read (fd, &preview_buf, 240*320*2)) < 0) {
				perror("read ");
				return;
			}
	#if 1			//for 320*240,640*480,800*480
			{
			int y;
			for (y = 0; y < 240; y++) 
                {
				    memcpy(fbmem + x_lcd_size*2*y, preview_buf + 320*2*y, 320*2);
			    }
            }
	#else			//for 240*320
			memcpy(fbmem, &preview_buf, 240*320*2);
	#endif
			fflush(stdout);
	}
	
	printf("\n");
}

static unsigned int fb_grab(int fd, char **fbmem)
{
	F_VINFO modeinfo;
	unsigned int length;

	if (ioctl(fd, FBIOGET_VSCREENINFO, &modeinfo) < 0) {
		perror("FBIOGET_VSCREENINFO");
		exit (EXIT_FAILURE);
	}
	length = modeinfo.xres * modeinfo.yres * (modeinfo.bits_per_pixel >> 3);

	x_lcd_size=modeinfo.xres;             //width of tft lcd
	y_lcd_size=modeinfo.yres;             //height of tft lcd

	printf("fb memory info=xres (%d) x yres (%d), %d bpp\n",
			modeinfo.xres, modeinfo.yres, modeinfo.bits_per_pixel);

	*fbmem = mmap(0, length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (*fbmem < 0) {
		perror("mmap()");
		length = 0;
	}

	return length;
}

static void fb_ungrab(char **fbmem, unsigned int length)
{
	if (*fbmem)
		munmap(*fbmem, length);
}


int main(int argc, char *argv[])
{
	int v4l2_fd = -1;
	int fb_fd = -1;
	char *fbmem = NULL;
	unsigned int fb_length = 0;

	int preview_frames = 180;
	int tmp;

	printf("ov7670 Camera Test Program, Start !\n");
	if (argc > 1) {
		if (sscanf(argv[1], "%d", &tmp) == 1)
			preview_frames = tmp;
	}

	v4l2_fd = open(V4L2_DEV_NODE, O_RDWR);
	if (v4l2_fd < 0) {
		perror(V4L2_DEV_NODE);
		goto out;
	}

	fb_fd = open(FB_DEV_NODE, O_RDWR);
	if (fb_fd < 0) {
		perror(FB_DEV_NODE);
		goto out;
	}
	
	fflush(stdout);
	if ((fb_length = fb_grab(fb_fd, &fbmem)) == 0)
		goto out;
	memset(fbmem, 0, fb_length);

	printf("Press Ctrl+C to stop !\n");
	fflush(stdout);

	v4l2_show_on_fb(v4l2_fd, fbmem, preview_frames);
	
	printf("\n");

out:

	if (v4l2_fd > 0)
		close(v4l2_fd);

	fb_ungrab(&fbmem, fb_length);

	return 0;
}

/*
 * main.c - top level of fbdev test utility
 * 08-31-25 E. Brombaugh
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <getopt.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/fb.h>
#include <sys/mman.h>

/* version */
const char *swVersionStr = "V0.1";

/* build time */
const char *bdate = __DATE__;
const char *btime = __TIME__;

/*
 * Pass 8-bit (each) R,G,B, get back 16-bit packed color
*/
uint16_t ST7789_Color565(uint32_t rgb24)
{
	uint16_t color16;
	color16 = 	(((rgb24>>16) & 0xF8) << 8) |
				(((rgb24>>8) & 0xFC) << 3) |
				((rgb24 & 0xF8) >> 3);
	
	return color16;
}
/*
 * top level
 */
int main(int argc, char **argv)
{
	extern char *optarg;
	char *fbdev = "/dev/fb0";
	int opt;
	int backlight = 1, mode = 0, i, verbose = 0;
	int iret;
    
	/* parse options */
	while((opt = getopt(argc, argv, "+b:vVh")) != EOF)
	{
		switch(opt)
		{
			case 'b':
				backlight = atoi(optarg);
				break;
			
			case 'v':
				verbose = 1;
				break;
			
			case 'V':
				fprintf(stderr, "%s version %s\n", argv[0], swVersionStr);
				exit(0);
			
			case 'h':
			case '?':
				fprintf(stderr, "USAGE: %s [options] MODE\n", argv[0]);
				fprintf(stderr, "Version %s, %s %s\n", swVersionStr, bdate, btime);
				fprintf(stderr, "Options: -b 0|1 dis/enable backlight\n");
				fprintf(stderr, "         -v enables verbose progress messages\n");
				fprintf(stderr, "         -V prints the tool version\n");
				fprintf(stderr, "         -h prints this help\n");
				exit(1);
		}
	}
    if (optind < argc)
	{
		/* operating mode is first non-option argument */
		mode = atoi(argv[optind]) & 3;
    }
	else
	{
		printf("Must specify MODE (0-7)\n");
		exit(1);
	}
	
	if(verbose)
        printf("MODE: %d\n", mode);
	
	/* handle backlight */
	if(verbose)
		printf("Turn %s Backlight...\n", backlight ? "on" : "off");
	
	char cmd[80];
	sprintf(cmd, "echo %d > /sys/class/backlight/fb_st7789v/bl_power", backlight);
	system(cmd);
		
	/* open device */
	if(verbose)
		printf("Opening device %s...\n", fbdev);
	int fbfd = open (fbdev, O_RDWR);
	if(fbfd < 0)
	{
		fprintf(stderr, "Couldn't open FB device %s\n", fbdev);
		exit(1);
	}
	
	struct fb_var_screeninfo vinfo;
	ioctl (fbfd, FBIOGET_VSCREENINFO, &vinfo);
	int fb_width = vinfo.xres;
	int fb_height = vinfo.yres;
	int fb_bpp = vinfo.bits_per_pixel;
	int fb_bytes = fb_bpp / 8;

	if(verbose)
	{
		fprintf(stderr, "Width = %d\n", fb_width);
		fprintf(stderr, "Height = %d\n", fb_height);
		fprintf(stderr, "Bits/pixel = %d\n", fb_bpp);
		fprintf(stderr, "Bytes = %d\n", fb_bytes);
	}
	
	/* set up mmap for fbdev frame buffer */
	int fb_data_size = fb_width * fb_height * fb_bytes;
	uint16_t *fbdata = mmap (0, fb_data_size, 
			PROT_READ | PROT_WRITE, MAP_SHARED, fbfd, (off_t)0);
	if(fbdata == MAP_FAILED)
	{
		fprintf(stderr, "Couldn't MMAP %d bytes\n", fb_data_size);
		close(fbfd);
		exit(1);
	}
	
	if(verbose)
		fprintf(stderr, "MMAP %d bytes succeeded\n", fb_data_size);

	/* do stuff to the buffer */
	uint16_t color = ST7789_Color565(0xFF << (mode*8));
	for(int i=0;i<fb_data_size/2;i++)
		fbdata[i] = color;

	/* clean up */
	munmap(fbdata, fb_data_size);
	close(fbfd);
	
	return 0;
}

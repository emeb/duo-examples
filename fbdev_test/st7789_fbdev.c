/*
 * ST7789_fbdev.c - interface routines for ST7789_fbdev LCD on dspod_module.
 * shamelessly ganked from Adafruit_ST7789_fbdev library
 * 08-12-19 E. Brombaugh
 * 10-21-20 E. Brombaugh - updated for f405_codec_v2
 * 10-28-20 E. Brombaugh - updated for f405 feather + tftwing
 * 10-11-21 E. Brombaugh - updated for RP2040
 * 09-12-22 E. Brombaugh - updated for 01Space LCD
 * 08-28-25 E. Brombaugh - updated for dspod_module
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include "st7789_fbdev.h"

/* ----------------------- I/O definitions ----------------------- */

/* high level driver interface */
GFX_DRIVER ST7789_fbdev_drvr =
{
	ST7789_fbdev_TFTWIDTH,
	ST7789_fbdev_TFTHEIGHT,
	ST7789_fbdev_init,
	ST7789_fbdev_setRotation,
    ST7789_fbdev_Color565,
    ST7789_fbdev_ColorRGB,
	ST7789_fbdev_fillRect,
	ST7789_fbdev_drawPixel,
	ST7789_fbdev_bitblt
};

/* LCD state */
char *fbdev = "/dev/fb0";
int fbfd;
int fb_data_size;
uint16_t *fbdata;
uint8_t rowstart, colstart;
uint16_t _width, _height, rotation;

/* ----------------------- Public functions ----------------------- */
// Initialization for ST7789_fbdevR red tab screens
uint8_t ST7789_fbdev_init(void)
{
	/* open fb device */
	int fbfd = open (fbdev, O_RDWR);
	if(fbfd < 0)
	{
		fprintf(stderr, "Couldn't open FB device %s\n", fbdev);
		return 1;
	}
	
	/* get device info */
	struct fb_var_screeninfo vinfo;
	ioctl (fbfd, FBIOGET_VSCREENINFO, &vinfo);
	int fb_width = vinfo.xres;
	int fb_height = vinfo.yres;
	int fb_bpp = vinfo.bits_per_pixel;
	int fb_bytes = fb_bpp / 8;
	
	/* set up mmap for fbdev frame buffer */
	fb_data_size = fb_width * fb_height * fb_bytes;
	fbdata = mmap (0, fb_data_size, 
			PROT_READ | PROT_WRITE, MAP_SHARED, fbfd, (off_t)0);
	if(fbdata == MAP_FAILED)
	{
		fprintf(stderr, "Couldn't MMAP %d bytes\n", fb_data_size);
		close(fbfd);
		return 1;
	}
	
	/* set screen dimensions - override device height as this one is only 170 */
	_width = ST7789_fbdev_drvr.xmax = fb_width;
	_height = ST7789_fbdev_drvr.ymax = ST7789_fbdev_TFTHEIGHT;
	rowstart = ST7789_fbdev_ROWSTART; // not set properly in device tree
	colstart = ST7789_fbdev_COLSTART;

	return 0;
}

// clean up at end
void ST7789_fbdev_deinit(void)
{
	munmap(fbdata, fb_data_size);
	close(fbfd);
}

// draw single pixel
void ST7789_fbdev_drawPixel(int16_t x, int16_t y, uint16_t color)
{
	if((x < 0) ||(x >= _width) || (y < 0) || (y >= _height)) return;

	uint32_t addr = _width * (y+rowstart) + (x+colstart);
	fbdata[addr] = color;
}

// fill a rectangle
void ST7789_fbdev_fillRect(int16_t x, int16_t y, int16_t w, int16_t h,
	uint16_t color)
{
	// rudimentary clipping (drawChar w/big text requires this)
	if((x >= _width) || (y >= _height)) return;
	if((x + w - 1) >= _width)  w = _width  - x;
	if((y + h - 1) >= _height) h = _height - y;

	/* fill area with color */
	while(h--)
	{
		uint32_t addr = _width * (y+rowstart) + (x+colstart);
		//printf("addr = %d\n", addr);
		int16_t cnt = w;
		uint16_t *ptr = &fbdata[addr];
		while(cnt--)
			*ptr++ = color;
		y++;
	}
}

// Pass 8-bit (each) R,G,B, get back 16-bit packed color
uint16_t ST7789_fbdev_Color565(uint32_t rgb24)
{
	uint16_t color16;
	color16 = 	(((rgb24>>16) & 0xF8) << 8) |
				(((rgb24>>8) & 0xFC) << 3) |
				((rgb24 & 0xF8) >> 3);
	
	return color16;
}

// Pass 16-bit packed color, get back 8-bit (each) R,G,B in 32-bit
uint32_t ST7789_fbdev_ColorRGB(uint16_t color16)
{
    uint32_t r,g,b;
	
    r = (color16 & 0xF800)>>8;
    g = (color16 & 0x07E0)>>3;
    b = (color16 & 0x001F)<<3;
	return (r<<16) | (g<<8) | b;
}

// bitblt a region to the display
void ST7789_fbdev_bitblt(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t *buf)
{
	// rudimentary clipping (drawChar w/big text requires this)
	if((x >= _width) || (y >= _height)) return;
	if((x + w - 1) >= _width)  w = _width  - x;
	if((y + h - 1) >= _height) h = _height - y;

	/* fill area with data from buffer */
	while(h--)
	{
		uint32_t addr = _width * (y+rowstart) + (x*colstart);
		int16_t cnt = w;
		int16_t *ptr = &fbdata[addr];
		while(cnt--)
			*ptr++ = *buf++;
		y++;
	}
}

// set orientation of display - not used for fbdev
void ST7789_fbdev_setRotation(uint8_t m)
{
}

// backlight on/off
void ST7789_fbdev_setBacklight(uint8_t ena)
{
	char cmd[80];
	sprintf(cmd, "echo %d > /sys/class/backlight/fb_st7789v/bl_power", ena);
	system(cmd);
}

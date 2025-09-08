/*
 * ST7789_fbdev.h - interface routines for ST7789_fbdev LCD.
 * shamelessly ganked from Adafruit_ST7789_fbdev library
 * 08-28-20 E. Brombaugh
 * 10-21-20 E. Brombaugh - updated for f405_codec_v2
 * 10-28-20 E. Brombaugh - updated for f405 feather + tftwing
 * 10-11-21 E. Brombaugh - updated for RP2040
 * 08-31-25 E. Brombaugh - updated for linux fbdev
 */

#ifndef __ST7789_fbdev__
#define __ST7789_fbdev__

#include "gfx.h"

// dimensions for LCD on dsod_module as seen by fbdev
#define ST7789_fbdev_TFTWIDTH 320
#define ST7789_fbdev_TFTHEIGHT 170
#define ST7789_fbdev_ROWSTART 35
#define ST7789_fbdev_COLSTART 0

extern GFX_DRIVER ST7789_fbdev_drvr;

uint8_t ST7789_fbdev_init(void);
void ST7789_fbdev_deinit(void);
void ST7789_fbdev_setAddrWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);
void ST7789_fbdev_drawPixel(int16_t x, int16_t y, uint16_t color);
void ST7789_fbdev_fillRect(int16_t x, int16_t y, int16_t w, int16_t h,
	uint16_t color);
uint16_t ST7789_fbdev_Color565(uint32_t rgb24);
uint32_t ST7789_fbdev_ColorRGB(uint16_t color16);
void ST7789_fbdev_bitblt(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t *buf);
void ST7789_fbdev_setRotation(uint8_t m);
void ST7789_fbdev_setBacklight(uint8_t ena);

#endif

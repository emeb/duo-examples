/*
 * main.c - top level of fbdev test utility
 * 08-31-25 E. Brombaugh
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <getopt.h>
#include <errno.h>
#include <math.h>
#include "st7789_fbdev.h"
#include "gfx.h"
#include "encoder.h"
#include "adc.h"

/* version */
const char *swVersionStr = "V0.1";

/* build time */
const char *bdate = __DATE__;
const char *btime = __TIME__;

/*
 * top level
 */
int main(int argc, char **argv)
{
	extern char *optarg;
	int opt;
	int backlight = 1, mode = 0, i, verbose = 0;
	int iret;
	GFX_RECT rect;
	char textbuf[32];
	int16_t val = 0;
	uint8_t btn = 0;
    
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
		mode = atoi(argv[optind]);
    }
	else
	{
		printf("Must specify MODE (0-7)\n");
		exit(1);
	}
	
	if(verbose)
		printf("Backlight: %s\n", backlight ? "on" : "off");
	ST7789_fbdev_setBacklight(backlight);

	if(verbose)
        printf("MODE: %d\n", mode);
	
	if(gfx_init(&ST7789_fbdev_drvr))
	{
		fprintf(stderr, "Couldn't init graphics lib\n");
		exit(1);
	}
	if(verbose)
        printf("Graphics initialized\n");
	
	/* test some stuff */
	switch(mode)
	{
		case 0:
			/* display extent */
			printf("Test offsets.\n");
			gfx_drawline(0, 0, 319, 169);
			gfx_drawline(319, 0, 0, 169);
			gfx_drawstr(0, 0, "0, 0");
			gfx_drawstr(160, 85, "160, 85");
			gfx_drawstr(255, 161, "255, 161");
			break;
		
		case 1:
			/* rounded rectangles */
			printf("Test rounded rects\n");
		
			/* white background rect */
			rect.x0 = 2;
			rect.y0 = 2;
			rect.x1 = 317;
			rect.y1 = 167;
			gfx_fillroundedrect(&rect, 20);
			
			/* cyan box */
			rect.x0 = 50;
			rect.y0 = 50;
			rect.x1 = 150;
			rect.y1 = 150;
			gfx_set_forecolor(GFX_CYAN);
			gfx_fillroundedrect(&rect, 20);
			
			/* magenta tall box w/ radius clamped to dimensions */
			rect.x0 = 200;
			rect.y0 = 20;
			rect.x1 = 250;
			rect.y1 = 160;
			gfx_set_forecolor(GFX_MAGENTA);
			gfx_fillroundedrect(&rect, 50);
			
			/* blue box with white text */
			rect.x0 = 20;
			rect.y0 = 10;
			rect.x1 = 170;
			rect.y1 = 40;
			gfx_set_forecolor(GFX_BLUE);
			gfx_fillroundedrect(&rect, 5);
			gfx_set_backcolor(GFX_BLUE);
			gfx_set_forecolor(GFX_WHITE);
			gfx_set_txtscale(3);
			gfx_drawstrctr((rect.x0+rect.x1)/2, (rect.y0+rect.y1)/2, "Hello");
			break;
		
		case 2:
			/* circles and lines */
			printf("Test circles\n");
			uint8_t hsv[] = {0,255,255};
			GFX_COLOR color;
			for(int i = 0;i<256;i+=16)
			{
				hsv[0] = i;
				color = gfx_hsv2rgb(hsv);
				gfx_set_forecolor(color);
				int16_t x, y;
				float th = (float)i * 6.2832F / 256.0F;
				x = 160 + 70.0F*sinf(th);
				y = 85 - 70.0F*cosf(th);
				gfx_fillcircle(x, y, 10);
			}
			
			gfx_set_forecolor(GFX_GREEN);
			int cnt = 20;
			int16_t x = 160, y = 85 - 55;
			while(cnt--)
			{
				for(int i=0;i<256;i++)
				{
					float th = (float)i * 6.2832F / 256.0F;
					gfx_set_forecolor(GFX_BLACK);
					gfx_drawline(160,85,x, y);
					x = 160 + 55.0F*sinf(th);
					y = 85 - 55.0F*cosf(th);
					gfx_set_forecolor(GFX_GREEN);
					gfx_drawline(160,85,x, y);
					usleep(4000);
				}
			}
			break;
		
		case 3:
			{
				/* circles and lines */
				printf("Test drawing + encoder\n");
				uint8_t hsv[] = {0,255,255};
				GFX_COLOR color;
				for(int i = 0;i<256;i+=16)
				{
					hsv[0] = i;
					color = gfx_hsv2rgb(hsv);
					gfx_set_forecolor(color);
					int16_t x, y;
					float th = (float)i * 6.2832F / 256.0F;
					x = 160 + 70.0F*sinf(th);
					y = 85 - 70.0F*cosf(th);
					gfx_fillcircle(x, y, 10);
				}
			
				encoder_init();
				int16_t val = 0;
				uint8_t btn = 0;
				gfx_set_forecolor(GFX_GREEN);
				int16_t x = 160, y = 85 - 55;
				gfx_drawline(160,85,x, y);
				int i = 0;
				while(1)
				{
					if(encoder_poll(&val, &btn))
					{
						i -= 16*val;
						float th = (float)i * 6.2832F / 256.0F;
						gfx_set_forecolor(GFX_BLACK);
						gfx_drawline(160,85,x, y);
						
						if(btn&2)
							break;
						
						x = 160 + 55.0F*sinf(th);
						y = 85 - 55.0F*cosf(th);
						gfx_set_forecolor(GFX_GREEN);
						gfx_drawline(160,85,x, y);
					}
				}
				encoder_deinit();
			}
			break;
		
		case 4:
			/* adc realtime */
			encoder_init();
			adc_init("/dev/cvi-saradc0");
			while(1)
			{
				for(uint8_t i=0;i<4;i++)
				{
					adc_set_chl(i, 1);
					val = adc_get_value();
					sprintf(textbuf, "%1d: %4d", i, val);
					gfx_set_forecolor(GFX_WHITE);
					gfx_drawstr(20, 20+20*i, textbuf);
					rect.x0 = 90;
					rect.y0 = 20+20*i;
					rect.x1 = 90+val/18;
					rect.y1 = rect.y0+8;
					gfx_set_forecolor(GFX_GREEN);
					gfx_fillrect(&rect);
					rect.x0 = rect.x1;
					rect.x1 = 319;
					gfx_set_forecolor(GFX_BLACK);
					gfx_fillrect(&rect);
				}
				encoder_poll(&val, &btn);
				if(btn&2)
					break;
			}
			adc_deinit();
			encoder_deinit();
			break;
		
		default:
			gfx_set_txtscale(3);
			gfx_drawstrctr(160, 85, "Hello World!");
	}
	
	/* clean up */
	if(verbose)
        printf("Cleaning up\n");
	ST7789_fbdev_deinit();
	
	return 0;
}

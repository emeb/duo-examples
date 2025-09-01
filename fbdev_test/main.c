/*
 * main.c - top level of fbdev test utility
 * 08-31-25 E. Brombaugh
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <getopt.h>
#include <errno.h>
#include "st7789_fbdev.h"
#include "gfx.h"

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
			GFX_RECT rect = {2,2,317,167};
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
			
		default:
			gfx_set_txtscale(3);
			gfx_drawstrctr(160, 85-24, "Hello World!");
	}
	
	/* clean up */
	if(verbose)
        printf("Cleaning up\n");
	ST7789_fbdev_deinit();
	
	return 0;
}

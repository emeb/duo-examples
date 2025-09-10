/*
 * menu.c - menuing UI for dspod_cv1800b
 * 09-08-25 E. Brombaugh
 */

#include <stdio.h>
#include "main.h"
#include "menu.h"
#include "widgets.h"
#include "audio.h"
#include "encoder.h"

#define MENU_XMAX 319
#define MENU_YMAX 169
#define MENU_CV_WIDTH 50
#define MENU_VU_WIDTH 50
#define MENU_NUM_ALGO 20

static uint8_t menu_reset;
static int8_t menu_next_algo, menu_curr_algo;

/*
 * Draw splash screen
 */
void menu_splash(const char *swVersionStr, const char *bdate, const char *btime)
{
	GFX_RECT rect;
	char textbuf[32];

	rect.x0 = 2;
	rect.y0 = 2;
	rect.x1 = 317;
	rect.y1 = 167;
	gfx_fillroundedrect(&rect, 20);
	rect.x0 = 40;
	rect.y0 = 40;
	rect.x1 = 279;
	rect.y1 = 130;
	gfx_set_forecolor(GFX_BLUE);
	gfx_fillroundedrect(&rect, 20);
	gfx_set_backcolor(GFX_BLUE);
	gfx_set_forecolor(GFX_WHITE);
	gfx_set_txtscale(2);
	gfx_drawstrctr((rect.x0+rect.x1)/2, (rect.y0+rect.y1)/2, "DSPOD");
	gfx_set_txtscale(1);
	sprintf(textbuf, "Version %s", swVersionStr);
	gfx_drawstrctr((rect.x0+rect.x1)/2, (rect.y0+rect.y1)/2+16, textbuf);
	sprintf(textbuf, "%s %s", bdate, btime);
	gfx_drawstrctr((rect.x0+rect.x1)/2, (rect.y0+rect.y1)/2+32, textbuf);
}

/*
 * redraw the menu
 */
void menu_render(void)
{
	uint8_t i;
	GFX_RECT rect;
	char textbuf[32];

	/* refresh static items */
	if(menu_reset)
	{
		menu_reset = 0;
		
		/* set constants */
		gfx_set_backcolor(GFX_DGRAY);
		gfx_set_forecolor(GFX_WHITE);
		
		/* CV indicators */
		gfx_drawstr(MENU_XMAX/2-MENU_CV_WIDTH-6-16, 141, "C0");
		gfx_drawstr(MENU_XMAX/2-MENU_CV_WIDTH-6-16, 151, "C1");
		gfx_drawstr(MENU_XMAX/2+5+MENU_CV_WIDTH+2, 141, "C2");
		gfx_drawstr(MENU_XMAX/2+5+MENU_CV_WIDTH+2 , 151, "C3");
	
		/* vu meters labels and boxes */
		gfx_drawstr(10, 141, "il");
		gfx_drawstr(10, 151, "ir");
		gfx_drawstr(MENU_XMAX-10-16, 141, "ol");
		gfx_drawstr(MENU_XMAX-10-16, 151, "or");
	}
	
	/* update dynamic items */
	sprintf(textbuf, "Load: %2u%% ", Audio_get_load());
	gfx_drawstr(20, 20, textbuf);
	
	/* algo selection */
	sprintf(textbuf, "Next: %2u   Curr: %2u", menu_next_algo, menu_curr_algo);
	gfx_drawstr(0, 40, textbuf);

	/* CV indicators */
	widg_bargraphH(MENU_XMAX/2-5-MENU_CV_WIDTH, 140, MENU_CV_WIDTH, 8, adc_buffer[0]/41);
	widg_bargraphH(MENU_XMAX/2-5-MENU_CV_WIDTH, 150, MENU_CV_WIDTH, 8, adc_buffer[1]/41);
	widg_bargraphH(MENU_XMAX/2+5, 140, MENU_CV_WIDTH, 8, adc_buffer[2]/41);
	widg_bargraphH(MENU_XMAX/2+5, 150, MENU_CV_WIDTH, 8, adc_buffer[3]/41);
	
	/* VU meters */
	widg_bargraphHG(30, 140, MENU_VU_WIDTH, 8, Audio_get_level(0)/328);
	widg_bargraphHG(30, 150, MENU_VU_WIDTH, 8, Audio_get_level(1)/328);
	widg_bargraphHG(MENU_XMAX-10-16-6-MENU_VU_WIDTH, 140, MENU_VU_WIDTH, 8, Audio_get_level(2)/328);
	widg_bargraphHG(MENU_XMAX-10-16-6-MENU_VU_WIDTH, 150, MENU_VU_WIDTH, 8, Audio_get_level(3)/328);
	
}

/*
 * init the menu state
 */
void menu_init(void)
{
	/* wipe screen */
	gfx_set_backcolor(GFX_DGRAY);
	gfx_set_forecolor(GFX_WHITE);
	gfx_clrscreen();
	
	/* create VU gradient */
	widg_gradient_init(MENU_VU_WIDTH);
	
	menu_reset = 1;
	menu_curr_algo = menu_next_algo = 0;
	
	menu_render();
}

/*
 * process menu events
 */
void menu_process(void)
{
	int16_t enc_val;
	uint8_t enc_btn;
	
	// detect encoder changes
	if(encoder_poll(&enc_val, &enc_btn))
	{
		menu_next_algo += enc_val;
		menu_next_algo = menu_next_algo < 0 ? 0 : menu_next_algo;
		menu_next_algo = menu_next_algo >= MENU_NUM_ALGO ? MENU_NUM_ALGO-1 : menu_next_algo;
		
		if(enc_btn)
		{
			menu_curr_algo = menu_next_algo;
		}
	}
	
	// update display
	menu_render();
}

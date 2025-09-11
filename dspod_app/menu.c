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
#include "fx.h"

#define MENU_XMAX 319
#define MENU_YMAX 169
#define MENU_CV_WIDTH 50
#define MENU_VU_WIDTH 50

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
		
		/* current algo name box */
		gfx_set_forecolor(GFX_BLUE);
		rect.x0 = MENU_XMAX/2 - 120;
		rect.y0 = 36;
		rect.x1 = MENU_XMAX/2 + 120;
		rect.y1 = 60;
		gfx_fillroundedrect(&rect, 24);
		gfx_set_forecolor(GFX_WHITE);
		gfx_set_backcolor(GFX_BLUE);
		gfx_set_txtscale(2);
		sprintf(textbuf, "%2u: %s", fx_get_algo(), fx_get_curr_algo_name());
		gfx_drawstrctr(MENU_XMAX/2, 48, textbuf);
		gfx_set_txtscale(1);

		/* set constants */
		gfx_set_backcolor(GFX_DGRAY);
		
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
	
#if 1
	/* update dynamic items */
	sprintf(textbuf, "Load: %2u%% ", Audio_get_load());
	gfx_drawstr(10, 4, textbuf);
	
	/* update algo params */
	gfx_set_backcolor(GFX_DGRAY);
	gfx_set_txtscale(1);
	for(i=0;i<3;i++)
	{
		//menu_item_values[menu_algo][i] = adc_buffer[i];
		fx_render_parm(i);
	}
	
	/* update W/D mix param */
	gfx_drawstrctr((240+319)/2, 129-16, "W/D Mix");
	sprintf(textbuf, "%2d%% ", adc_buffer[3]/41);
	gfx_drawstrctr((240+319)/2, 129-6, textbuf);
	
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
#endif
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
	GFX_RECT rect;
	char textbuf[32];
	
	// detect encoder changes
	if(encoder_poll(&enc_val, &enc_btn))
	{
		if(enc_val)
		{
			menu_next_algo += enc_val;
			menu_next_algo = menu_next_algo < 0 ? 0 : menu_next_algo;
			menu_next_algo = menu_next_algo >= FX_NUM_ALGOS ? FX_NUM_ALGOS-1 : menu_next_algo;
		
			/* next algo box */
			gfx_set_forecolor(GFX_MAGENTA);
			rect.x0 = MENU_XMAX/2 - 60;
			rect.y0 = 16;
			rect.x1 = MENU_XMAX/2 + 60;
			rect.y1 = 32;
			gfx_fillroundedrect(&rect, 16);
			
			/* algo selection */
			gfx_set_forecolor(GFX_WHITE);
			gfx_set_backcolor(GFX_MAGENTA);
			sprintf(textbuf, "%2u: %s", menu_next_algo, fx_get_algo_name(menu_next_algo));
			gfx_drawstrctr(MENU_XMAX/2, 24, textbuf);
			gfx_set_backcolor(GFX_DGRAY);
		}
		
		if(enc_btn)
		{
			/* erase next algo box */
			gfx_set_forecolor(GFX_DGRAY);
			rect.x0 = MENU_XMAX/2 - 60;
			rect.y0 = 16;
			rect.x1 = MENU_XMAX/2 + 60;
			rect.y1 = 32;
			gfx_fillrect(&rect);
			gfx_set_forecolor(GFX_WHITE);
			
			/* update current algo & redraw */
			menu_reset = 1;
			menu_curr_algo = menu_next_algo;
			fx_select_algo(menu_next_algo);
		}
	}
	
	// update display
	menu_render();
}

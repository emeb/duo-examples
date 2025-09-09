/*
 * menu.c - menuing UI for dspod_cv1800b
 * 09-08-25 E. Brombaugh
 */

#include "main.h"
#include "menu.h"
#include "widgets.h"
#include "audio.h"

#define MENU_XMAX 319
#define MENU_YMAX 169
#define MENU_VU_WIDTH 50

static uint8_t menu_reset;

/*
 * redraw the menu
 */
void menu_render(void)
{
	uint8_t i;
	GFX_RECT rect;
	
	/* refresh static items */
	if(menu_reset)
	{
		menu_reset = 0;
		
		/* set constants */
		gfx_set_backcolor(GFX_DGRAY);
		gfx_set_forecolor(GFX_WHITE);
	
		/* vu meters labels and boxes */
		gfx_drawstr(10, 141, "il");
		gfx_drawstr(10, 151, "ir");
		gfx_drawstr(MENU_XMAX-10-16, 141, "ol");
		gfx_drawstr(MENU_XMAX-10-16, 151, "or");
	}
	
	/* update dynamic items */
	widg_bargraphHG(30, 140, MENU_VU_WIDTH, 8, Audio_get_level(0)/328);
	widg_bargraphHG(30, 150, MENU_VU_WIDTH, 8, Audio_get_level(1)/328);
	widg_bargraphHG(MENU_XMAX-10-16 - 6 - MENU_VU_WIDTH, 140, MENU_VU_WIDTH, 8, Audio_get_level(2)/328);
	widg_bargraphHG(MENU_XMAX-10-16 - 6 - MENU_VU_WIDTH, 150, MENU_VU_WIDTH, 8, Audio_get_level(3)/328);
	
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
	
	menu_render();
}

/*
 * process menu events
 */
void menu_process(void)
{
	menu_render();
}

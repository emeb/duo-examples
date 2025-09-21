/*
 * fx.h - Algorithm access for dspod cv1800b
 * 09-11-25 E. Brombaugh
 */

#ifndef __fx__
#define __fx__

#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "main.h"
#include "dsp_lib.h"
#include "adc.h"
#include "gfx.h"

#define SAMPLE_RATE     (48000)
#define FRAMESZ			(64)

#define FX_NUM_ALGOS  6
#define FX_MAX_PARAMS 3
#define FX_MAX_MEM (320*1024)		// 320kB
#define FX_EXT_MEM (16*1024*1024)	// 16MB

/* pre-allocated external memory */
extern int16_t *fx_ext_buffer;
extern size_t fx_ext_sz;

/*
 * structure containing algorithm access info
 */
typedef struct
{
	const char *name;
	uint8_t parms;
	const char **parm_names;
	void * (*init)(uint32_t *mem);
	void (*cleanup)(void *blk);
	void (*proc)(void *blk, int16_t *dst, int16_t *src, uint16_t sz);
	void (*render_parm)(void *blk, uint8_t idx, GFX_RECT *rect, uint8_t init);
} fx_struct;

/* array of ptrs to effects structs */
extern const fx_struct *effects[FX_NUM_ALGOS];

void fx_bypass_Cleanup(void *dummy);
void fx_bypass_Render_Parm(void *blk, uint8_t idx, GFX_RECT *rect, uint8_t init);

uint8_t fx_init(void);
uint8_t fx_deinit(void);
void fx_select_algo(uint8_t algo);
void fx_proc(int16_t *dst, int16_t *src, uint16_t sz);
uint8_t fx_get_algo(void);
uint8_t fx_get_num_parms(void);
char * fx_get_algo_name(uint8_t algo_num);
char * fx_get_curr_algo_name(void);
char * fx_get_parm_name(uint8_t idx);
void fx_render_parm(uint8_t idx, uint8_t init);

#endif


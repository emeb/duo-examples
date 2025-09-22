/*
 * audio.c - audio processing routines for long delay
 * 08-20-20 E. Brombaugh
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <math.h>
#include "main.h"
#include "audio.h"
#include "dsp_lib.h"
#include "fx.h"

/* stereo or mono */
#define CHLS 2

#define FADE_BITS 11
#define FADE_MAX ((1<<FADE_BITS)-1)

#define NUM_PT 4
#define WAV_PHS 10
#define WAV_LEN (1<<WAV_PHS)
#define INTERP_BITS 10

enum fadestate 
{
    FADE_OFF,
    FADE_IN,
    FADE_OUT,
};

int16_t *prcbuf;
uint32_t fadecnt;
uint8_t init, fadest;
int16_t audio_sl[4];
int16_t audio_mute_state, audio_mute_cnt;
int16_t prev_wet;

/*
 * init audio
 */
int32_t Audio_Init(uint32_t buffer_size)
{
	/* init fx */
	if(fx_init())
	{
		if(verbose)
			fprintf(stderr, "Audio_Init: fx_init() failed\n");
		return 1;
	}
	else
	{
		if(verbose)
			fprintf(stderr, "Audio_Init: fx_init() OK\n");
	}
	
	/* signal levels */
	audio_sl[0] = audio_sl[1] = audio_sl[2] = audio_sl[3] = 0;
	
	/* Muting */
	audio_mute_state = 2;	// start up  muted
	audio_mute_cnt = 0;
	
	/* procesing buffer */
	if(!(prcbuf = malloc(buffer_size)))
	{
		fprintf(stderr, "Audio_Init: couldn't allocate processing buffer\n");
		fx_deinit();
		return 1;
	}
	
	return 0;
}

/*
 * free audio resources 
 */
void Audio_Close(void)
{
    if(prcbuf)
        free(prcbuf);
}

/*
 * signal level calc
 */
void level_calc(int16_t sig, int16_t *level)
{
	/* rectify */
	sig = (sig < 0) ? -sig : sig;

	/* peak hold - externally reset */
	if(*level < sig)
		*level = sig;
}

/*
 * process the audio
 */
void Audio_Process(char *wrbuf, char *rdbuf, int inframes)
{
	int16_t *src = (int16_t *)rdbuf;
	int16_t *prc = (int16_t *)prcbuf;
	int16_t *dst = (int16_t *)wrbuf;
	uint16_t index;
	int32_t wet, dry, mix;
	float live_wet, slope_wet;
	
	/* check input levels */
	for(index=0;index<inframes;index++)
	{
		level_calc(*(src+0), &audio_sl[0]);
		level_calc(*(src+1), &audio_sl[1]);
	}
	
	/* apply the effect */
	fx_proc(prc, src, inframes);
	
	/* set W/D mix gain and prep linear interp */
	wet = adc_buffer[3];
	live_wet = prev_wet;
	slope_wet = (float)(wet - prev_wet) / (float)inframes;
	prev_wet = wet;

	/* W/D mixing and output level detect */
	for(index=0;index<inframes;index++)
	{
		/* linear interp W/D mix gain */
		wet = live_wet;
		dry = 0xfff - wet;
		live_wet += slope_wet;
		
		/* W/D with saturation */
		mix = *prc++ * wet + *src++ * dry;
		*dst++ = dsp_ssat16(mix>>12);
		mix = *prc++ * wet + *src++ * dry;
		*dst++ = dsp_ssat16(mix>>12);

		/* handle muting */
		switch(audio_mute_state)
		{
			case 0:
				/* pass thru and wait for foreground to force a transition */
				break;
			
			case 1:
				/* transition to mute state */
				mix = (*(dst-2) * audio_mute_cnt);
				*(dst-2) = dsp_ssat16(mix>>9);
				mix = (*(dst-1) * audio_mute_cnt);
				*(dst-1) = dsp_ssat16(mix>>9);
				audio_mute_cnt--;
				if(audio_mute_cnt == 0)
					audio_mute_state = 2;
				break;
				
			case 2:
				/* mute and wait for foreground to force a transition */
				*(dst-2) = 0;
				*(dst-1) = 0;
				break;
			
			case 3:
				/* transition to unmute state */
				mix = (*(dst-2) * audio_mute_cnt);
				*(dst-2) = dsp_ssat16(mix>>9);
				mix = (*(dst-1) * audio_mute_cnt);
				*(dst-1) = dsp_ssat16(mix>>9);
				audio_mute_cnt++;
				if(audio_mute_cnt == 512)
				{
					audio_mute_state = 0;
					audio_mute_cnt = 0;
				}
				break;
				
			default:
				/* go to legal state */
				audio_mute_state = 0;
				break;
		}

		/* check output levels */
		level_calc(*(dst-2), &audio_sl[2]);
		level_calc(*(dst-1), &audio_sl[3]);
	}
}

/*
 * get audio level for in/out right/left
 */
int16_t Audio_get_level(uint8_t idx)
{
	idx &= 3;
	int16_t result = audio_sl[idx];
	audio_sl[idx] = 0;
	return result;
}

/*
 * internal soft mute - called from foreground context to control muting
 * blocks until background has completed mute/unmute transition
 */
void Audio_mute(uint8_t enable)
{
    if(verbose)
		fprintf(stdout, "audio_mute: start - state = %d, enable = %d\n", audio_mute_state, enable);
	
	if((audio_mute_state == 0) && (enable == 1))
	{
		audio_mute_cnt = 512;
		audio_mute_state = 1;
		while(audio_mute_state != 2)
		{
			usleep(1000);
		}
	}
	else if((audio_mute_state == 2) && (enable == 0))
	{
		audio_mute_cnt = 0;
		audio_mute_state = 3;
		while(audio_mute_state != 0)
		{
			usleep(1000);
		}
	}
	
    if(verbose)
		fprintf(stdout, "audio_mute: done\n");
}

/*
 * audio.c - audio processing routines for long delay
 * 08-20-20 E. Brombaugh
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <math.h>
#include <sys/time.h>
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

uint32_t bufsz;
int16_t *dlybuf, *wptr;
int16_t *prcbuf;
uint32_t fadecnt;
int32_t gain;
uint8_t init, fadest, pt;
int32_t frq, phs;
int16_t sinetab[1024];
int16_t audio_sl[4];
uint64_t audio_load[3];
int16_t audio_mute_state, audio_mute_cnt;
int16_t prev_wet;

/*
 * init audio
 */
int32_t Audio_Init(uint32_t buffer_size, uint32_t dlysamp, uint8_t proc_typ, float amp, float freq)
{
	if(verbose)
		fprintf(stderr, "Audio_Init: proc = %d\n", proc_typ);
	
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
	
	/* audio load calcs */
	audio_load[0] = audio_load[1] = audio_load[2] = 0;
	
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
	
	if(verbose)
		fprintf(stderr, "Audio_Init: allocated %d proc buffer\n", buffer_size);
	
#if 0
	/* NCO */
	phs = 0;
	frq = (int32_t)floorf(freq * powf(2.0F, 32.0F) / 48000.0F);
	
	/* build sinewave LUT */
	float th = 0.0F, thinc = 6.2832F/((float)WAV_LEN);
	for(int i=0;i<WAV_LEN;i++)
	{
		sinetab[i] = floorf(32767.0F * sinf(th) + 0.5F);
		th += thinc;
	}
#endif
	
	/* processing type */
	pt = proc_typ%NUM_PT;
	
	/* output gain shift */
	gain = amp * 32767.0F;
	
	/* delay buffer */
    bufsz = CHLS*dlysamp;
    dlybuf = (int16_t *)malloc(bufsz*sizeof(int16_t));
	if(!dlybuf)
	{
		fprintf(stderr, "Audio_Init: couldn't allocate delay buffer\n");
		free(prcbuf);
		fx_deinit();
		return 1;
	}
	
 	if(verbose)
		fprintf(stderr, "Audio_Init: allocated %d delay buffer\n", bufsz);
	
	wptr = dlybuf;
    init = 1;
    fadecnt = FADE_MAX;
    fadest =  FADE_IN;
    
	return 0;
}

/*
 * free audio resources 
 */
void Audio_Close(void)
{
    if(dlybuf)
        free(dlybuf);
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

#if 0
/*
 * sine waveform interp
 */
int16_t sine_interp(uint32_t phs)
{
	
	int32_t a, b, sum;
	uint32_t ip, fp;
	
	ip = phs>>(32-WAV_PHS);
	a = sinetab[ip];
	b = sinetab[(ip + 1)&(WAV_LEN-1)];
	
	fp = (phs & ((1<<(32-WAV_PHS))-1)) >> ((32-WAV_PHS)-INTERP_BITS);
	sum = b * fp;
	sum += a * (((1<<INTERP_BITS)-1)-fp);
	
	return sum >> INTERP_BITS; 
}

/*
 * compute effect on buffers
 */
void fx_proc(int16_t *dst, int16_t *src, uint32_t len)
{
	uint32_t index;
	int16_t tbuf[CHLS];
    int32_t tmpscl;
    uint8_t chl;
	
	/* process I2S data */
	for(index=0;index<len;index++)
	{
		/* apply chosen effect */
		if(pt == 2)
		{
			/* delayed loopback */
			/* get output from buffer */
			if(!init)
			{
				for(chl=0;chl<CHLS;chl++)
					tbuf[chl] = *(wptr+chl);
			}
			else
			{
				/* no output until buffer filled */
				for(chl=0;chl<CHLS;chl++)
					tbuf[chl] = 0;
			}
			
			/* save input in buffer */
			for(chl=0;chl<CHLS;chl++)
			{
				tmpscl = *src++;
				if(fadest == FADE_IN)
					tmpscl = (tmpscl * (FADE_MAX-fadecnt))>>FADE_BITS;
				
				*wptr++ = tmpscl;
				*dst++ = tbuf[chl];
			}
			
			/* wrap pointer */
			if((wptr-dlybuf) >= bufsz)
			{
				wptr = dlybuf;
				//printf("%1d",init);
				//fflush(stdout);
				init = 0;
			}
			
			/* update fader */
			if(fadecnt)
			{
				fadecnt--;
				if(fadecnt==0)
					fadest = FADE_OFF;
			}
		}
		else if(pt==3)
		{
			/* immediate loopback */
			for(chl=0;chl<CHLS;chl++)
				*dst++ = (*src++);
		}
		else if(pt==1)
		{
			/* sawtooth generation */
			int32_t wav = (((int32_t)phs >> 16)*gain) >> 15;
			*dst++ = wav;
			*dst++ = -wav;
			phs += frq;
		}
		else
		{
			/* sine generation */
			int32_t wav = (sine_interp((uint32_t)phs)*gain)>>15;
			*dst++ = wav;
			*dst++ = -wav;
			phs += frq;
		}
	}
}
#endif

/*
 * process the audio
 */
void Audio_Process(char *rdbuf, int inframes)
{
	int16_t *src = (int16_t *)rdbuf;
	int16_t *prc = (int16_t *)prcbuf;
	int16_t *dst = src;
	uint16_t index;
	int32_t wet, dry, mix;
	float live_wet, slope_wet;
	struct timeval tv;
	
	/* get entry time */
	gettimeofday(&tv,NULL);
	audio_load[2] = audio_load[0];
	audio_load[0] = 1000000 * tv.tv_sec + tv.tv_usec;
	
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
	
	/* get exit time */
	gettimeofday(&tv,NULL);
	audio_load[1] = 1000000 * tv.tv_sec + tv.tv_usec;
}

/*
 * get cpu loading
 */
uint8_t Audio_get_load(void)
{
	uint64_t period = audio_load[0] - audio_load[2];
	uint64_t duration = audio_load[1] - audio_load[0];
	uint64_t load = 0;
	
	/* update load indicator */
	if(period != 0)
	{
		load = 100*duration/period;
	}
	
	fprintf(stdout, "Load: % 6llu / % 6llu = % 3llu%% \r", duration, period, load);
	fflush(stdout);
	
	return load;
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

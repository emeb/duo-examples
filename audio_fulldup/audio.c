/*
 * audio.c - audio processing routines for long delay
 * 08-20-20 E. Brombaugh
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "main.h"
#include "audio.h"

/* stereo or mono */
#define CHLS 2

#define FADE_BITS 11
#define FADE_MAX ((1<<FADE_BITS)-1)

#define NUM_PT 3
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
uint32_t fadecnt;
int32_t gain;
uint8_t init, fadest, pt;
int32_t frq, phs;
int16_t sinetab[1024];

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
 * init audio
 */
int32_t Audio_Init(uint32_t dlysamp, uint8_t proc_typ, float amp, float freq)
{
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
	
	/* processing type */
	pt = proc_typ%NUM_PT;
	
	/* output gain shift */
	gain = amp * 32767.0F;
	
	/* delay buffer */
    bufsz = CHLS*dlysamp;
    dlybuf = (int16_t *)malloc(bufsz*sizeof(int16_t));
    wptr = dlybuf;
    init = 1;
    fadecnt = FADE_MAX;
    fadest =  FADE_IN;
    
    if(dlybuf)
        return 0;
    else
        return 1;
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
 * process the audio
 */
void Audio_Process(char *rdbuf, int inframes)
{
	int16_t *src = (int16_t *)rdbuf;
	int16_t *dst = src;
	int16_t tbuf[CHLS];
	uint16_t index;
    int32_t tmpscl;
    uint8_t chl;
    
	/* process I2S data */
	for(index=0;index<inframes;index++)
	{
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
				*dst++ = (*src++)*chl;
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

/*
 * print status
 */
void Audio_Status(void)
{
}

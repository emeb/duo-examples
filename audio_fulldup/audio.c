/*
 * audio.c - audio processing routines for long delay
 * 08-20-20 E. Brombaugh
 */

#include <stdio.h>
#include <stdlib.h>
#include "main.h"
#include "audio.h"

/* stereo or mono */
#define CHLS 2

#define FADE_BITS 11
#define FADE_MAX ((1<<FADE_BITS)-1)

enum fadestate 
{
    FADE_OFF,
    FADE_IN,
    FADE_OUT,
};

uint32_t bufsz;
int16_t *dlybuf, *wptr;
uint32_t fadecnt;
uint8_t init, fadest;

/*
 * init audio
 */
int32_t Audio_Init(uint32_t dlysamp)
{
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
#if 1
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
#else
		for(chl=0;chl<CHLS;chl++)
			*dst++ = (*src++)*chl;
#endif
	}
}

/*
 * print status
 */
void Audio_Status(void)
{
}

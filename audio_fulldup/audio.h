/*
 * audio.h - audio processing for long delay
 * 08-20-20 E. Brombaugh
 */

#ifndef __audio__
#define __audio__

int32_t Audio_Init(uint32_t dlysamp, uint8_t proc_typ, float amp, float freq);
void Audio_Close(void);
void Audio_Process(char *rdbuf, int inframes);
void Audio_Status(void);

#endif

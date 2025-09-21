/*
 * audio.h - audio processing for long delay
 * 08-20-20 E. Brombaugh
 */

#ifndef __audio__
#define __audio__

#include <stdint.h>

int32_t Audio_Init(uint32_t buffer_size);
void Audio_Close(void);
void Audio_Process(char *wrbuf, char *rdbuf, int inframes);
uint8_t Audio_get_load(void);
int16_t Audio_get_level(uint8_t idx);
void Audio_mute(uint8_t enable);

#endif

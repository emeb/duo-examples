/*
 * dsp_lib.h - miscellaneous DSP stuff for dspod cv1800b
 * 09-10-25 E. Brombaugh
 */

#ifndef __dsp_lib__
#define __dsp_lib__

#include <stdint.h>

uint8_t dsp_gethyst(int16_t *oldval, int16_t newval);
uint8_t dsp_ratio_hyst_arb(uint16_t *old, uint16_t in, uint8_t range);


/*
 * signed saturation to 16-bit
 * unfortunately, no simple integer clip/sat instr in C906 (rv64imafdcv0p8xthead)
 */
inline int16_t dsp_ssat16(int32_t in)
{
	in = in > 32767 ? 32767 : in;
	in = in < -32768 ? -32768 : in;
	return in;
}

#endif


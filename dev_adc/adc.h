/*
 * adc.h - muxed adc interface. Must use custom kernel driver.
 * 09-05-25 E. Brombaugh
 */

#ifndef __adc__
#define __adc__

#include <stdint.h>

int8_t adc_init(char *device);
void adc_set_chl(uint8_t chl);
uint16_t adc_get_value(void);
void adc_deinit(void);

#endif
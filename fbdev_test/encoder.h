/*
 * encoder.h - non-blocking interface to encoder/button drivers
 * 09-02-25 E. Brombaugh
 */
#ifndef __encoder__
#define __encoder__

#include <stdint.h>

uint8_t encoder_init(void);
uint8_t encoder_poll(int16_t *enc_val, uint8_t *enc_btn);
void encoder_deinit(void);

#endif
/*
 * adc.c - muxed adc interface. Must use custom kernel driver.
 * 09-05-25 E. Brombaugh
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include "adc.h"

static int adcfd;

/*
 * prepare the ADC device for use
 */
int8_t adc_init(char *device)
{
	/* open the ADC device */
	adcfd = open (device, O_RDWR);
	if(adcfd < 0)
	{
		fprintf(stderr, "Couldn't open ADC device %s\n", device);
		return 1;
	}
	
	return 0;
}

/*
 * send value to driver to set the internal mux to ADC1 and external mux
 * to desired channel
 */
void adc_set_chl(uint8_t chl)
{
	chl = ((chl & 3)<<3) | 1;
	write(adcfd, &chl, 1);
}

/*
 * get value from driver or return 0xFFFF for error
 */
uint16_t adc_get_value(void)
{
	uint16_t result;
	if(read(adcfd, &result, sizeof(uint16_t)) !=2)
	{
		result = 0xffff; 
	}
	return result;
}

/*
 * close the device
 */
void adc_deinit(void)
{
	close(adcfd);
}

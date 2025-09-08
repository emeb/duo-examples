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
	/* load the kernel module (if not already) */
	if(system("lsmod | grep -q \"cv180x_saradc\"") == 0)
	{
		fprintf(stderr, "SARADC module already loaded.\n");
	}else{
		system("insmod cv180x_saradc.ko 2>/dev/null");
		fprintf(stderr, "SARADC module loaded.\n");
	}

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
 * send value to driver to set the int & ext muxes to ADC1
 */
void adc_set_chl(uint8_t exchl, uint8_t inchl)
{
	exchl = ((exchl & 3)<<3) | (inchl & 7);
	//exchl = ((exchl & 3)<<3) | 1;//(inchl & 7);
	write(adcfd, &exchl, 1);
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

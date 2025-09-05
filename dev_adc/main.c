/*
 * main.c - top level of adc device test utility
 * 09-04-25 E. Brombaugh
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <getopt.h>
#include <errno.h>
#include <sys/time.h>
#include "adc.h"

#define NUM_SMPLS 1024

/* version */
const char *swVersionStr = "V0.1";

/* build time */
const char *bdate = __DATE__;
const char *btime = __TIME__;

/*
 * top level
 */
int main(int argc, char **argv)
{
	extern char *optarg;
	int opt;
	int chl = 0, i, verbose = 0;
	int iret;
    char *devname = "/dev/cvi-saradc0";
	uint8_t inchl = 0;
	uint16_t data[NUM_SMPLS];
	struct timeval tv;
	unsigned long time_in_micros;
	
	/* parse options */
	while((opt = getopt(argc, argv, "+c:vVh")) != EOF)
	{
		switch(opt)
		{
			case 'c':
				inchl = atoi(optarg);
				break;
			
			case 'v':
				verbose = 1;
				break;
			
			case 'V':
				fprintf(stderr, "%s version %s\n", argv[0], swVersionStr);
				exit(0);
			
			case 'h':
			case '?':
				fprintf(stderr, "USAGE: %s [options] CHL\n", argv[0]);
				fprintf(stderr, "Version %s, %s %s\n", swVersionStr, bdate, btime);
				fprintf(stderr, "Options: -v enables verbose progress messages\n");
				fprintf(stderr, "         -c CHL specify channel (default = 1)\n");
				fprintf(stderr, "         -V prints the tool version\n");
				fprintf(stderr, "         -h prints this help\n");
				exit(1);
		}
	}

	/* open the ADC device */
	if(adc_init(devname))
	{
		fprintf(stderr, "Couldn't open ADC device %s\n", devname);
		return 1;
	}
	
	/* set the channel */
	adc_set_chl(inchl);
	
#if 1		
	/* get a conversion */
	if((data[0] = adc_get_value()) == 0xffff)
	{
		fprintf(stderr, "Error reading data\n");
	}
	else
	{
		fprintf(stdout, "Data = %d (0x%04X)\n", data[0], data[0]);
	}
#else
	/* get multiple conversions */
	gettimeofday(&tv,NULL);
	time_in_micros = 1000000 * tv.tv_sec + tv.tv_usec;
	for(int i = 0;i<NUM_SMPLS;i++)
	{
		read(adcfd, &data[i], sizeof(uint16_t));
	}
	gettimeofday(&tv,NULL);
	time_in_micros = (1000000 * tv.tv_sec + tv.tv_usec) - time_in_micros;
	
	/* results */
	for(int i = 0;i<NUM_SMPLS;i++)
	{
		fprintf(stdout, "% 2d: %d\n", i, data[i]);
	}
	fprintf(stdout, "Avg time / sample = %d us\n", time_in_micros / NUM_SMPLS);
#endif
	
	/* close it */
	adc_deinit();
	
	return 0;
}

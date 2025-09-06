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
#include <math.h>
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
	int nsamp = 1, i, verbose = 0;
	int iret;
    char *devname = "/dev/cvi-saradc0";
	uint8_t inchl = 1, exchl = 0;
	uint16_t data[NUM_SMPLS];
	struct timeval tv;
	unsigned long time_in_micros;
	
	/* parse options */
	while((opt = getopt(argc, argv, "+i:x:n:vVh")) != EOF)
	{
		switch(opt)
		{
			case 'i':
				inchl = atoi(optarg);
				break;
			
			case 'x':
				exchl = atoi(optarg);
				break;
			
			case 'n':
				nsamp = atoi(optarg);
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
				fprintf(stderr, "         -i CHL specify internal channel (default = 1)\n");
				fprintf(stderr, "         -x CHL specify external channel (default = 0)\n");
				fprintf(stderr, "         -n SMPLS specify num samples (default = 1)\n");
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
	adc_set_chl(exchl, inchl);
	
	/* get multiple conversions */
	gettimeofday(&tv,NULL);
	time_in_micros = 1000000 * tv.tv_sec + tv.tv_usec;
	for(int i = 0;i<nsamp;i++)
	{
		data[i] = adc_get_value();
	}
	gettimeofday(&tv,NULL);
	time_in_micros = (1000000 * tv.tv_sec + tv.tv_usec) - time_in_micros;
	
	/* results */
	float mean = 0.0F;
	for(int i = 0;i<nsamp;i++)
	{
		if(verbose)
			fprintf(stdout, "% 2d: %d\n", i, data[i]);
		mean += (float)data[i];
	}
	mean = mean / (float)nsamp;
	fprintf(stdout, "Avg time / sample = %d us\n", time_in_micros / NUM_SMPLS);
	fprintf(stdout, "Avg value = %f\n", mean);
	float var = 0.0F;
	mean *= mean;
	for(int i = 0;i<nsamp;i++)
	{
		var += ((float)data[i]*(float)data[i]) - mean;
	}
	var = var /(float)nsamp;
	fprintf(stdout, "Std. Dev = %f\n", sqrtf(var));
	
	/* close it */
	adc_deinit();
	
	return 0;
}

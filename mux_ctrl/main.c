/*
 * main.c - top level of ADC mux control utility
 * 08-30-25 E. Brombaugh
 */

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <errno.h>
#include <wiringx.h>

#define MUXA_PIN 16
#define MUXB_PIN 17

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
    
	/* parse options */
	while((opt = getopt(argc, argv, "+vVh")) != EOF)
	{
		switch(opt)
		{
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
				fprintf(stderr, "         -V prints the tool version\n");
				fprintf(stderr, "         -h prints this help\n");
				exit(1);
		}
	}
    if (optind < argc)
	{
		/* mux channel is first non-option argument */
		chl = atoi(argv[optind]) & 3;
    }
	else
	{
		printf("Must specify CHL.\n");
		exit(1);
	}
	
	if(verbose)
        printf("CHL: %d\n", chl);
	
	if(verbose)
		printf("Setup wiringX...\n");
	
	if(wiringXSetup("milkv_duo", NULL) == -1)
	{
        wiringXGC();
		if(verbose)
			printf("wiringXSetup() failed\n");
        return -1;
    }
	
	if(verbose)
		printf("Checking GPIO pins...\n");
	
    if(wiringXValidGPIO(MUXA_PIN) != 0)
	{
		if(verbose)
			printf("Invalid GPIO %d\n", MUXA_PIN);
		return -2;
    }
    if(wiringXValidGPIO(MUXB_PIN) != 0)
	{
		if(verbose)
			printf("Invalid GPIO %d\n", MUXB_PIN);
		return -2;
    }
	
	if(verbose)
		printf("Assigning direction...\n");
	
	pinMode(MUXA_PIN, PINMODE_OUTPUT);
    pinMode(MUXB_PIN, PINMODE_OUTPUT);

	if(verbose)
		printf("Assigning values...\n");
	
	digitalWrite(MUXA_PIN, (chl & 1) ? HIGH : LOW);
	digitalWrite(MUXB_PIN, (chl & 2) ? HIGH : LOW);

	if(verbose)
		printf("Done.\n");
	
	return 0;
}

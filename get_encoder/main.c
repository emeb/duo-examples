/*
 * main.c - top level of get_encoder utility
 * 09-01-25 E. Brombaugh
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <errno.h>
#include <getopt.h>
#include <ctype.h>
#include <signal.h>
#include <limits.h>
#include "encoder.h"

/* version */
const char *swVersionStr = "V0.1";

/* build time */
const char *bdate = __DATE__;
const char *btime = __TIME__;

static volatile sig_atomic_t stop = 0;

static void interrupt_handler(int sig)
{
	printf("\ninterrupt!\n");
	stop = 1;
}

/*
 * top level
 */
int main(int argc, char **argv)
{
	extern char *optarg;
	int opt;
	int verbose = 0;
    
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
	
	if(encoder_init())
	{
		fprintf(stderr, "Error initializing encoder.\n");
		exit(1);
	}
	
	if(verbose)
		printf("Setup open device...\n");
	
	signal(SIGINT, interrupt_handler);
	signal(SIGTERM, interrupt_handler);
	
	/* wait for events and process them */
	int count = 0;	// tracking time
	int16_t enc_val;
	uint8_t enc_btn;
	while(!stop)
	{
		if(encoder_poll(&enc_val, &enc_btn))
		{
			printf("Rot = %d, Btn = %d\n", enc_val, enc_btn);
		}
		count++;
	}

	encoder_deinit();
	
	return 0;
}

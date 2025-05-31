/*
 * main.c - top level of aic3101 codec config utility
 * 03-21-25 E. Brombaugh
 */

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <errno.h>
#include "codec_aic3101.h"

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
	int bus = 1, i, verbose = 0, type = 0, dump = 0;
	int iret;
    
	/* parse options */
	while((opt = getopt(argc, argv, "b:devVh")) != EOF)
	{
		switch(opt)
		{
			case 'b':
				/* bus */
				bus = atoi(optarg);
				break;
				
			case 'd':
				/* register dump */
				dump = 1;
				break;
				
			case 'e':
				/* type */
				type = 1;
				break;
				
			case 'v':
				verbose = 1;
				break;
			
			case 'V':
				fprintf(stderr, "%s version %s\n", argv[0], swVersionStr);
				exit(0);
			
			case 'h':
			case '?':
				fprintf(stderr, "USAGE: %s [options]\n", argv[0]);
				fprintf(stderr, "Version %s, %s %s\n", swVersionStr, bdate, btime);
				fprintf(stderr, "Options: -b <I2C bus num>    Default: %d\n", bus);
				fprintf(stderr, "         -d dump I2C registers after setting\n");
				fprintf(stderr, "         -e Ext MCLK gen (default Int w/ PLL)\n");
				fprintf(stderr, "         -v enables verbose progress messages\n");
				fprintf(stderr, "         -V prints the tool version\n");
				fprintf(stderr, "         -h prints this help\n");
				exit(1);
		}
	}
	
	if(verbose)
		printf("Configuring with %s MCLK profile\n", type ? "external" : "internal");
	return codec_aic3101(verbose, bus, type, dump);
}

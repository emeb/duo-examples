/*
 * main.c - top level of tst_pffft for cv1800b
 * 09-16-25 E. Brombaugh
 */
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <math.h>
#include "pffft.h"

#define ITER 100

int main(int argc, char **argv)
{
	int cnt = ITER;
	struct timeval tv;
	unsigned long time_in_micros;
	PFFFT_Setup *pffft_setup;
	float *in, *work, *out;
	float flops, tavg;
	int fftsz = 1024;
	
	if(argc > 1)
		fftsz = atoi(argv[1]);
	
	/* init a transform */
	pffft_setup = pffft_new_setup(fftsz, PFFFT_COMPLEX);
	if(!pffft_setup)
	{
		fprintf(stderr, "Unsupported FFT size %d\n", fftsz);
		exit(1);
	}
	
	flops = 5.0F*fftsz*log((float)fftsz)/M_LN2;
	printf("FFT size %d => %f float ops\n", fftsz, flops);
	
	/* create arrays */
	in = (float *)malloc(sizeof(float)*2*fftsz);
	work = (float *)malloc(sizeof(float)*2*fftsz);
	out = (float *)malloc(sizeof(float)*2*fftsz);
	
	/* init the input array with some data */
	srand48(345353);
	for(int i=0;i<fftsz*2;i++)
		in[i] = drand48();
	
	/* get start time */
	gettimeofday(&tv,NULL);
		
	/* do a bunch */
	while(cnt--)
	{
		pffft_transform(pffft_setup, in, out, work, PFFFT_FORWARD);
	}
		
	/* get time difference */
	time_in_micros = 1000000 * tv.tv_sec + tv.tv_usec;
	gettimeofday(&tv,NULL);
	time_in_micros = (1000000 * tv.tv_sec + tv.tv_usec) - time_in_micros;
	tavg = (float)time_in_micros / (float)ITER;
	
	printf("Average time = %f us/xfrm\n", tavg);
	printf("%f MFLOPS\n", flops/tavg );
	
	/* cleanup */
	free(out);
	free(work);
	free(in);
	pffft_destroy_setup(pffft_setup);

}
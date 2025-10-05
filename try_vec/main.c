/*
 * try_vec.c - try out vectorizing ideas on fragments from r8 fft
 * 09-25-25 E. Brombaugh
 */
 
#include <getopt.h>
#include <sys/time.h>
#include "main.h"
#include "func0.h"
#include "func1.h"

/* version */
const char *swVersionStr = "V0.1";

/* build time */
const char *bdate = __DATE__;
const char *btime = __TIME__;

/* array of possible tests */
const snippet *snfuncs[] =
{
	&func0,
	&func1,
};

/*
 * utility function
 */
void dump_vec(vfloat32m1_t v, size_t vl)
{
	float tmp[4];
	vse32_v_f32m1(tmp, v, vl);
	printf("%f %f %f %f\n", tmp[0], tmp[1], tmp[2], tmp[3]);
}

/*
 * top-level test harness
 */
int main(int argc, char **argv)
{
	extern char *optarg;
	int opt;
	int len = 64, i, verbose = 0, result = 1;
	int max_idx = sizeof(snfuncs) / sizeof(snippet *), func_idx = 0;
	float *t_in;		// array of test input data
	float *r_out;		// array of reference output
	float *u_out;		// array of UUT output
	struct timeval tv1, tv2;
	unsigned long time_scl, time_vec;
	
	/* parse options */
	while((opt = getopt(argc, argv, "f:l:vVh")) != EOF)
	{
		switch(opt)
		{
			case 'f':
				/* function to test */
				func_idx = atoi(optarg);
				break;

			case 'l':
				/* length of test */
				len = atoi(optarg);
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
				fprintf(stderr, "Options: -f <function> Default: %d\n", func_idx);
				fprintf(stderr, "         -l <length>   Default: %d\n", len);
				fprintf(stderr, "         -v enables verbose progress messages\n");
				fprintf(stderr, "         -V prints the tool version\n");
				fprintf(stderr, "         -h prints this help\n");
				exit(1);
		}
	}
	
	/* make sure we're not trying to run an illegal snippet */
	if(func_idx >= max_idx)
	{
		fprintf(stderr, "Illegal function\n");
		goto t_err;
	}
	
	/* create all the data arrays */
	if(!(t_in = malloc(len * 2 * sizeof(float))))
		goto t_err;
	if(!(r_out = malloc(len * 2 * sizeof(float))))
		goto r_err;
	if(!(u_out = malloc(len * 2 * sizeof(float))))
		goto u_err;
	
	/* fill the test array */
	for(i=0;i<len;i++)
	{
		t_in[2*i] = (float)i;
		t_in[2*i + 1] = (float)i * -1.0f;
	}
	
	/* announce */
	fprintf(stderr, "Testing Func %d: %s\n", func_idx, snfuncs[func_idx]->name);
	
	/* init the funcs */
	if(snfuncs[func_idx]->init(len))
	{
		fprintf(stderr, "Init of Func %d failed\n");
		goto f_err;
	}
	
	/* reference */
	memcpy(r_out, t_in, len * 2 * sizeof(float));
	gettimeofday(&tv1, NULL);
	snfuncs[func_idx]->scalar(r_out, len);
	gettimeofday(&tv2, NULL);
	time_scl = 1000000 * tv1.tv_sec + tv1.tv_usec;
	time_scl = (1000000 * tv2.tv_sec + tv2.tv_usec) - time_scl;
	
	/* uut */
	memcpy(u_out, t_in, len * 2 * sizeof(float));
	gettimeofday(&tv1, NULL);
	snfuncs[func_idx]->vector(u_out, len);
	gettimeofday(&tv2, NULL);
	time_vec = 1000000 * tv1.tv_sec + tv1.tv_usec;
	time_vec = (1000000 * tv2.tv_sec + tv2.tv_usec) - time_vec;
	
	/* check results */
	float i_err, q_err;
	float i_acc = 0, q_acc = 0;
	for(i=0;i<len;i++)
	{
		i_err = r_out[2*i] - u_out[2*i];
		q_err = r_out[2*i+1] - u_out[2*i+1];
		
		if(verbose)
		{
			fprintf(stdout, "%5d: (% 7.2f,% 7.2f) - (% 7.2f,% 7.2f) = (% 7.2f,% 7.2f)\n",
				i,
				r_out[2*i], r_out[2*i+1],
				u_out[2*i], u_out[2*i+1],
				i_err, q_err
			);
		}
		
		i_acc += i_err;
		q_acc += q_err;
	}
	fprintf(stderr, "Accum. Err = (% 7.2f,% 7.2f)\n", i_acc, q_acc);
	fprintf(stderr, "Scalar time: %ld us\n", time_scl);
	fprintf(stderr, "Vector time: %ld us\n", time_vec);
	fprintf(stderr, "Speedup: %5.2f %%\n", 100.0f *(((float)time_scl / (float)time_vec) - 1.0f));
	
	/* clean up */
	result = 0;
f_err:
	free(u_out);
u_err:
	free(r_out);
r_err:
	free(t_in);
t_err:
	return result;
	
}

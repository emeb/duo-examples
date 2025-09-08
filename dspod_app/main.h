/*
 * main.h - stuff that's needed 
 * 08-20-20 E. Brombaugh
 */

#ifndef __main__
#define __main__

#include <stdint.h>
#include <math.h>

extern int sample_rate;
extern int exit_program;
extern int verbose;
void mixer_set(long vol);

#endif
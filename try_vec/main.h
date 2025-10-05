/*
 * main.h - stuff used by everyone for try_vec
 * 10-05-25 E. Brombaugh
 */

#ifndef __main__
#define __main__

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <riscv_vector.h>

typedef struct
{
	char *name;
	int (*init)(int len);
	void (*scalar)(float *pSrc, int len);
	void (*vector)(float *pSrc, int len);
} snippet;

void dump_vec(vfloat32m1_t v, size_t vl);

#endif

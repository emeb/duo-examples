/*
 * func0.c - original scalar/vector stuff yanked from radix8 fft
 * 10-5-2025 E. Brombaugh
 */

#include "func0.h"

/*
 * init not used in this snippet
 */
int init_func0(int len)
{
	return 0;
}

/*
 * this is the original non-vector code
 */
void scalar_func0(float *pSrc, int fftLen)
{
	uint32_t i1, i2, i3, i4, i5, i6, i7, i8;
	uint32_t n1, n2;

	float r1, r2, r3, r4, r5, r6, r7, r8;
	float t1, t2;
	float s1, s2, s3, s4, s5, s6, s7, s8;
	const float C81 = 0.70710678118f;

	n2 = fftLen;

	n1 = n2;
	n2 = n2 >> 3;
	i1 = 0;

	do
	{
		printf("%d\n", i1);
		
		i2 = i1 + n2;
		i3 = i2 + n2;
		i4 = i3 + n2;
		i5 = i4 + n2;
		i6 = i5 + n2;
		i7 = i6 + n2;
		i8 = i7 + n2;
		r1 = pSrc[2 * i1] + pSrc[2 * i5];
		r5 = pSrc[2 * i1] - pSrc[2 * i5];
		r2 = pSrc[2 * i2] + pSrc[2 * i6];
		r6 = pSrc[2 * i2] - pSrc[2 * i6];
		r3 = pSrc[2 * i3] + pSrc[2 * i7];
		r7 = pSrc[2 * i3] - pSrc[2 * i7];
		r4 = pSrc[2 * i4] + pSrc[2 * i8];
		r8 = pSrc[2 * i4] - pSrc[2 * i8];
#if 0
		t1 = r1 - r3;
		r1 = r1 + r3;
		r3 = r2 - r4;
		r2 = r2 + r4;
		pSrc[2 * i1] = r1 + r2;
		pSrc[2 * i5] = r1 - r2;
		r1 = pSrc[2 * i1 + 1] + pSrc[2 * i5 + 1];
		s5 = pSrc[2 * i1 + 1] - pSrc[2 * i5 + 1];
		r2 = pSrc[2 * i2 + 1] + pSrc[2 * i6 + 1];
		s6 = pSrc[2 * i2 + 1] - pSrc[2 * i6 + 1];
		s3 = pSrc[2 * i3 + 1] + pSrc[2 * i7 + 1];
		s7 = pSrc[2 * i3 + 1] - pSrc[2 * i7 + 1];
		r4 = pSrc[2 * i4 + 1] + pSrc[2 * i8 + 1];
		s8 = pSrc[2 * i4 + 1] - pSrc[2 * i8 + 1];
		t2 = r1 - s3;
		r1 = r1 + s3;
		s3 = r2 - r4;
		r2 = r2 + r4;
		pSrc[2 * i1 + 1] = r1 + r2;
		pSrc[2 * i5 + 1] = r1 - r2;
		pSrc[2 * i3]     = t1 + s3;
		pSrc[2 * i7]     = t1 - s3;
		pSrc[2 * i3 + 1] = t2 - r3;
		pSrc[2 * i7 + 1] = t2 + r3;
		r1 = (r6 - r8) * C81;
		r6 = (r6 + r8) * C81;
		r2 = (s6 - s8) * C81;
		s6 = (s6 + s8) * C81;
		t1 = r5 - r1;
		r5 = r5 + r1;
		r8 = r7 - r6;
		r7 = r7 + r6;
		t2 = s5 - r2;
		s5 = s5 + r2;
		s8 = s7 - s6;
		s7 = s7 + s6;
		pSrc[2 * i2]     = r5 + s7;
		pSrc[2 * i8]     = r5 - s7;
		pSrc[2 * i6]     = t1 + s8;
		pSrc[2 * i4]     = t1 - s8;
		pSrc[2 * i2 + 1] = s5 - r7;
		pSrc[2 * i8 + 1] = s5 + r7;
		pSrc[2 * i6 + 1] = t2 - r8;
		pSrc[2 * i4 + 1] = t2 + r8;
#else
		pSrc[2 * i1] = r1;
		pSrc[2 * i3] = r2;
		pSrc[2 * i5] = r3;
		pSrc[2 * i7] = r4;
		
		pSrc[2 * i2] = r5;
		pSrc[2 * i4] = r6;
		pSrc[2 * i6] = r7;
		pSrc[2 * i8] = r8;
#endif

		i1 += n1;
	} while (i1 < fftLen);
}

/*
 * this is the vectorized version - does it match?
 */
void vector_func0(float *pSrc, int fftLen)
{
	uint32_t i1, i2, i3, i4, i5, i6, i7, i8;
	uint32_t n1, n2;

	float r1, r2, r3, r4, r5, r6, r7, r8;
	float t1, t2;
	float s1, s2, s3, s4, s5, s6, s7, s8;
	const float C81 = 0.70710678118f;

	n2 = fftLen;

	n1 = n2;
	n2 = n2 >> 3;
	i1 = 0;

	do
	{
		printf("%d\n", i1);
		
		size_t vl = vsetvl_e32m1(4);
		vfloat32m1_t va = vlse32_v_f32m1(pSrc+2*i1, 2*n2*sizeof(float), vl);
		vfloat32m1_t vb = vlse32_v_f32m1(pSrc+2*(i1+n2*vl), 2*n2*sizeof(float), vl);
		vfloat32m1_t vc = vfadd_vv_f32m1(va, vb, vl);
		vfloat32m1_t vd = vfsub_vv_f32m1(va, vb, vl);
		vsse32_v_f32m1(pSrc + 2*i1, 2*2*n2*sizeof(float), vc, vl);
		vsse32_v_f32m1(pSrc + 2*(i1+n2), 2*2*n2*sizeof(float), vd, vl);

#if 0
#if 0
		t1 = r1 - r3;
		r1 = r1 + r3;
		r3 = r2 - r4;
		r2 = r2 + r4;
		
		pSrc[2 * i1] = r1 + r2;
		pSrc[2 * i5] = r1 - r2;
		r1 = pSrc[2 * i1 + 1] + pSrc[2 * i5 + 1];
		s5 = pSrc[2 * i1 + 1] - pSrc[2 * i5 + 1];
		r2 = pSrc[2 * i2 + 1] + pSrc[2 * i6 + 1];
		s6 = pSrc[2 * i2 + 1] - pSrc[2 * i6 + 1];
		s3 = pSrc[2 * i3 + 1] + pSrc[2 * i7 + 1];
		s7 = pSrc[2 * i3 + 1] - pSrc[2 * i7 + 1];
		r4 = pSrc[2 * i4 + 1] + pSrc[2 * i8 + 1];
		s8 = pSrc[2 * i4 + 1] - pSrc[2 * i8 + 1];
		t2 = r1 - s3;
		r1 = r1 + s3;
		s3 = r2 - r4;
		r2 = r2 + r4;
		pSrc[2 * i1 + 1] = r1 + r2;
		pSrc[2 * i5 + 1] = r1 - r2;
		pSrc[2 * i3]     = t1 + s3;
		pSrc[2 * i7]     = t1 - s3;
		pSrc[2 * i3 + 1] = t2 - r3;
		pSrc[2 * i7 + 1] = t2 + r3;
		r1 = (r6 - r8) * C81;
		r6 = (r6 + r8) * C81;
		r2 = (s6 - s8) * C81;
		s6 = (s6 + s8) * C81;
		t1 = r5 - r1;
		r5 = r5 + r1;
		r8 = r7 - r6;
		r7 = r7 + r6;
		t2 = s5 - r2;
		s5 = s5 + r2;
		s8 = s7 - s6;
		s7 = s7 + s6;
		pSrc[2 * i2]     = r5 + s7;
		pSrc[2 * i8]     = r5 - s7;
		pSrc[2 * i6]     = t1 + s8;
		pSrc[2 * i4]     = t1 - s8;
		pSrc[2 * i2 + 1] = s5 - r7;
		pSrc[2 * i8 + 1] = s5 + r7;
		pSrc[2 * i6 + 1] = t2 - r8;
		pSrc[2 * i4 + 1] = t2 + r8;
#else
		pSrc[2 * i1] = r1;
		pSrc[2 * i3] = r2;
		pSrc[2 * i5] = r3;
		pSrc[2 * i7] = r4;
		
		pSrc[2 * i2] = r5;
		pSrc[2 * i4] = r6;
		pSrc[2 * i6] = r7;
		pSrc[2 * i8] = r8;
#endif
#endif
		i1 += n1;
	} while (i1 < fftLen);
}

/*
 * bundle them up
 */
const snippet func0 = 
{
	"Radix 8 stuff",
	init_func0,
	scalar_func0,
	vector_func0,
};


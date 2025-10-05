/*
 * func1.c - test of complex mult
 * 10-5-2025 E. Brombaugh
 */

#include "func1.h"

#define PI (4.0F*atanf(1))

static float *twiddles;

/*
 * init not used in this snippet
 */
int init_func1(int len)
{
	if(!(twiddles = malloc(2*len*sizeof(float))))
		return 1;
	
	for(int i=0;i<len;i++)
	{
		twiddles[2*i] = cosf((float)i * PI * 2.0F / (float)len);
		twiddles[2*i+1] = sinf((float)i * PI * 2.0F / (float)len);
	}
	
	return 0;
}

/*
 * this is the original non-vector code
 */
void scalar_func1(float *pSrc, int len)
{
	for(int i = 0 ; i < len ; i++)
	{
		float re = pSrc[2*i] * twiddles[2*i] - pSrc[2*i+1] * twiddles[2*i+1];
		float im = pSrc[2*i] * twiddles[2*i+1] + pSrc[2*i+1] * twiddles[2*i];
		pSrc[2*i] = re;
		pSrc[2*i+1] = im;
	}
}

/*
 * this is the vectorized version - does it match?
 */
void vector_func1(float *pSrc, int len)
{
	size_t i = 0;
	while(i < len)
	{
		/* how many lanes to use this pass */
		size_t vl = vsetvl_e32m1(len - i);
		
		/* load real & imag w/ stride of 2 for interleaved */
		vfloat32m1_t va = vlse32_v_f32m1(pSrc + 2*i, 2*sizeof(float), vl);
		vfloat32m1_t vb = vlse32_v_f32m1(pSrc + 2*i + 1, 2*sizeof(float), vl);
		vfloat32m1_t vc = vlse32_v_f32m1(twiddles + 2*i, 2*sizeof(float), vl);
		vfloat32m1_t vd = vlse32_v_f32m1(twiddles + 2*i + 1, 2*sizeof(float), vl);
		
		/* multiplies */
		vfloat32m1_t vac = vfmul_vv_f32m1(va, vc, vl);
		vfloat32m1_t vbd = vfmul_vv_f32m1(vb, vd, vl);
		vfloat32m1_t vad = vfmul_vv_f32m1(va, vd, vl);
		vfloat32m1_t vbc = vfmul_vv_f32m1(vb, vc, vl);
		
		/* sums */
		vfloat32m1_t vre = vfsub_vv_f32m1(vac, vbd, vl);
		vfloat32m1_t vim = vfadd_vv_f32m1(vad, vbc, vl);
		
		/* store real & imag w/ stride for 2 for interleaved */
		vsse32_v_f32m1(pSrc + 2*i, 2*sizeof(float), vre, vl);
		vsse32_v_f32m1(pSrc + 2*i + 1, 2*sizeof(float), vim, vl);
		
		i += vl;
	}
}

/*
 * bundle them up
 */
const snippet func1 = 
{
	"complex mult",
	init_func1,
	scalar_func1,
	vector_func1,
};


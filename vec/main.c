/*
 * vector tests
 */
#include <riscv_vector.h>
#include <stdio.h>
#include <stdlib.h>

static inline float vector_dot_product(const float *a, const float *b, size_t n)
{
	if (n == 0)
		return 0.0f;

	/* vlmax = min(n, implementation VLmax) */
	size_t vlmax = vsetvl_e32m1(n);

	/* vector accumulator initialized to zero (uses vlmax lanes) */
	vfloat32m1_t vacc = vfmv_v_f_f32m1(0.0f, vlmax);

	size_t i = 0;
	while (i < n) {
		size_t vl = vsetvl_e32m1(n - i);
		vfloat32m1_t va = vle32_v_f32m1(a + i, vl);
		vfloat32m1_t vb = vle32_v_f32m1(b + i, vl);
		/* vacc += va * vb */
		vacc = vfmacc_vv_f32m1(vacc, va, vb, vl);
		i += vl;
	}

	/* Reduce accumulator by storing lanes to memory and summing them */
	float *tmp = (float *)malloc(vlmax * sizeof(float));

	/* store vacc lanes to tmp (vl = vlmax) */
	vse32_v_f32m1(tmp, vacc, vlmax);

	float result = 0.0f;
	for (size_t j = 0; j < vlmax; ++j) result += tmp[j];

	free(tmp);
	return result;
}

int main() {
	size_t vl = vsetvl_e32m1(8);
	printf("Test VL: %zu\n", vl);
	
	const float a[8] = {1,1,1,1,1,1,1,1};
	const float b[8] = {2,2,2,2,2,2,2,2};
	printf("Test dot: %f\n", vector_dot_product(a, b, 8));

	return 0;
}

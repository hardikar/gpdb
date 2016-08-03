#include <math.h>


#define CHECKFLOATVAL(val, inf_is_valid, zero_is_valid)			\
do {															\
	if (isinf(val) && !(inf_is_valid))							\
		("value out of range: overflow");				\
	if ((val) == 0.0 && !(zero_is_valid))						\
		("value out of range: underflow");				\
} while(0)

float
float8mul(float arg1, float arg2)
{
	float		result;

	result = arg1 * arg2;

	CHECKFLOATVAL(result, isinf(arg1) || isinf(arg2),
				  arg1 == 0 || arg2 == 0);
	return result;
}


#include <math.h>

#define true 1
#define false 0

#define CHECKFLOATVAL(val, inf_is_valid, zero_is_valid)			\
do {															\
	if (isinf(val) && !(inf_is_valid))							\
		("value out of range: overflow");				\
	if ((val) == 0.0 && !(zero_is_valid))						\
		("value out of range: underflow");				\
} while(0)

typedef double float8;

#define Datum float8

#define PG_RETURN_FLOAT8(result) return (Datum) result
#define PG_RETURN_BOOL(result) return (Datum) result
#define PG_RETURN_INT32(result) return (Datum) result

#define PG_FUNCTION_ARGS float8 in_arg_0, float8 in_arg_1
#define PG_GETARG_FLOAT8(i) ((float8) in_arg_##i)


#define INFO    17
#define elog  elog_start(__FILE__, __LINE__, __func__), elog_finish

extern void elog_start(const char *filename, int lineno, const char *funcname);
extern void elog_finish(int elevel, const char *fmt, ...);


extern void foo(void);

Datum
 float8pl(PG_FUNCTION_ARGS)
{
  float8    arg1 = PG_GETARG_FLOAT8(0);
  float8    arg2 = PG_GETARG_FLOAT8(1);
  float8    result;

  result = arg1 + arg2;

  elog(INFO, "Calling float8pl");
  foo();

  CHECKFLOATVAL(result, isinf(arg1) || isinf(arg2), true);
  PG_RETURN_FLOAT8(result);
}

Datum
float8mi(PG_FUNCTION_ARGS)
{
  float8    arg1 = PG_GETARG_FLOAT8(0);
  float8    arg2 = PG_GETARG_FLOAT8(1);
  float8    result;

  result = arg1 - arg2;

  CHECKFLOATVAL(result, isinf(arg1) || isinf(arg2), true);
  PG_RETURN_FLOAT8(result);
}

Datum
float8mul(PG_FUNCTION_ARGS)
{
  float8    arg1 = PG_GETARG_FLOAT8(0);
  float8    arg2 = PG_GETARG_FLOAT8(1);
  float8    result;

  result = arg1 * arg2;

  CHECKFLOATVAL(result, isinf(arg1) || isinf(arg2),
          arg1 == 0 || arg2 == 0);
  PG_RETURN_FLOAT8(result);
}

Datum
float8div(PG_FUNCTION_ARGS)
{
  float8    arg1 = PG_GETARG_FLOAT8(0);
  float8    arg2 = PG_GETARG_FLOAT8(1);
  float8    result;

//  if (arg2 == 0.0)
//    ereport(ERROR,
//        (errcode(ERRCODE_DIVISION_BY_ZERO),
//         errmsg("division by zero")));

  result = arg1 / arg2;

  CHECKFLOATVAL(result, isinf(arg1) || isinf(arg2), arg1 == 0);
  PG_RETURN_FLOAT8(result);
}


/*
 *    float8{eq,ne,lt,le,gt,ge}   - float8/float8 comparison operations
 */
static int
float8_cmp_internal(float8 a, float8 b)
{
  /*
   * We consider all NANs to be equal and larger than any non-NAN. This is
   * somewhat arbitrary; the important thing is to have a consistent sort
   * order.
   */
  if (isnan(a))
  {
    if (isnan(b))
      return 0;     /* NAN = NAN */
    else
      return 1;     /* NAN > non-NAN */
  }
  else if (isnan(b))
  {
    return -1;        /* non-NAN < NAN */
  }
  else
  {
    if (a > b)
      return 1;
    else if (a < b)
      return -1;
    else
      return 0;
  }
}


Datum
float8eq(PG_FUNCTION_ARGS)
{
  float8    arg1 = PG_GETARG_FLOAT8(0);
  float8    arg2 = PG_GETARG_FLOAT8(1);

  PG_RETURN_BOOL(float8_cmp_internal(arg1, arg2) == 0);
}

Datum
float8ne(PG_FUNCTION_ARGS)
{
  float8    arg1 = PG_GETARG_FLOAT8(0);
  float8    arg2 = PG_GETARG_FLOAT8(1);

  PG_RETURN_BOOL(float8_cmp_internal(arg1, arg2) != 0);
}

Datum
float8lt(PG_FUNCTION_ARGS)
{
  float8    arg1 = PG_GETARG_FLOAT8(0);
  float8    arg2 = PG_GETARG_FLOAT8(1);

  PG_RETURN_BOOL(float8_cmp_internal(arg1, arg2) < 0);
}

Datum
float8le(PG_FUNCTION_ARGS)
{
  float8    arg1 = PG_GETARG_FLOAT8(0);
  float8    arg2 = PG_GETARG_FLOAT8(1);

  PG_RETURN_BOOL(float8_cmp_internal(arg1, arg2) <= 0);
}

Datum
float8gt(PG_FUNCTION_ARGS)
{
  float8    arg1 = PG_GETARG_FLOAT8(0);
  float8    arg2 = PG_GETARG_FLOAT8(1);

  PG_RETURN_BOOL(float8_cmp_internal(arg1, arg2) > 0);
}

Datum
float8ge(PG_FUNCTION_ARGS)
{
  float8    arg1 = PG_GETARG_FLOAT8(0);
  float8    arg2 = PG_GETARG_FLOAT8(1);

  PG_RETURN_BOOL(float8_cmp_internal(arg1, arg2) >= 0);
}

Datum
btfloat8cmp(PG_FUNCTION_ARGS)
{
  float8    arg1 = PG_GETARG_FLOAT8(0);
  float8    arg2 = PG_GETARG_FLOAT8(1);

  PG_RETURN_INT32(float8_cmp_internal(arg1, arg2));
}

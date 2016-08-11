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
typedef unsigned int uint32;
typedef long int int64;
typedef char bool;

#define Datum int64
typedef union Datum_U
{
  Datum d;

  float8 f8;

  void *ptr;
} Datum_U;

static inline uint32 DatumGetUInt32(Datum d) { return (uint32) d; }
static inline Datum UInt32GetDatum(uint32 ui32) { return (Datum) ui32; }
static inline bool DatumGetBool(Datum d) { return ((bool)d) != 0; }
static inline Datum BoolGetDatum(bool b) { return (b ? 1 : 0); }
static inline float8 DatumGetFloat8(Datum d) { Datum_U du; du.d = d; return du.f8; }
static inline Datum Float8GetDatum(float8 f) { Datum_U du; du.f8 = f; return du.d; }

#define PG_RETURN_FLOAT8(x)  return Float8GetDatum(x)
#define PG_RETURN_BOOL(x)  return BoolGetDatum(x)
#define PG_RETURN_UINT32(x)  return UInt32GetDatum(x)

#define PG_FUNCTION_ARGS Datum in_arg_0, Datum in_arg_1
#define PG_GETARG_FLOAT8(i) (DatumGetFloat8 (in_arg_##i))


#define INFO    17
#define elog  elog_start(__FILE__, __LINE__, __func__), elog_finish

extern void elog_start(const char *filename, int lineno, const char *funcname);
extern void elog_finish(int elevel, const char *fmt, ...);


Datum
 float8pl(PG_FUNCTION_ARGS)
{
  float8    arg1 = PG_GETARG_FLOAT8(0);
  float8    arg2 = PG_GETARG_FLOAT8(1);
  float8    result;

  result = arg1 + arg2;

  elog(INFO, "Calling float8pl");

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

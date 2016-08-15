#include <math.h>

#define true 1
#define false 0

typedef double float8;
typedef int int32;
typedef long int int64;
typedef char bool;

#define Datum int64
typedef union Datum_U
{
  Datum d;

  float8 f8;

  void *ptr;
} Datum_U;

static inline Datum BoolGetDatum(bool b) { return (b ? 1 : 0); }
static inline int32 DatumGetInt32(Datum d) { return (int32) d; } 

#define PG_RETURN_FLOAT8(x)  return Float8GetDatum(x)
#define PG_RETURN_BOOL(x)  return BoolGetDatum(x)
#define PG_RETURN_UINT32(x)  return UInt32GetDatum(x)

#define PG_FUNCTION_ARGS Datum in_arg_0, Datum in_arg_1
#define PG_GETARG_INT32(i) (DatumGetInt32 (in_arg_##i))


#define INFO    17
#define elog  elog_start(__FILE__, __LINE__, __func__), elog_finish

extern void elog_start(const char *filename, int lineno, const char *funcname);
extern void elog_finish(int elevel, const char *fmt, ...);

Datum
int4le(PG_FUNCTION_ARGS)
{
    int32   arg1 = PG_GETARG_INT32(0);
    int32   arg2 = PG_GETARG_INT32(1);

    PG_RETURN_BOOL(arg1 <= arg2);
}

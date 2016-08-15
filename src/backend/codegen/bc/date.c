#include <math.h>

#define true 1
#define false 0

typedef double float8;
typedef int int32;
typedef long int int64;
typedef char bool;

#define Datum int64
typedef int32 DateADT;
typedef double Timestamp;
typedef union Datum_U
{
  Datum d;

  float8 f8;

  void *ptr;
} Datum_U;

static inline Datum BoolGetDatum(bool b) { return (b ? 1 : 0); }
static inline int64 DatumGetInt64(Datum d) { return (int64) d; }
static inline int32 DatumGetInt32(Datum d) { return (int32) d; } 

#define PG_RETURN_FLOAT8(x)  return Float8GetDatum(x)
#define PG_RETURN_BOOL(x)  return BoolGetDatum(x)
#define PG_RETURN_UINT32(x)  return UInt32GetDatum(x)

#define DatumGetDateADT(X)	  ((DateADT) DatumGetInt32(X))
#define DatumGetTimestamp(X)  ((Timestamp) DatumGetInt64(X))

#define PG_FUNCTION_ARGS Datum in_arg_0, Datum in_arg_1
#define PG_GETARG_INT32(i) (DatumGetInt32 (in_arg_##i))
#define PG_GETARG_DATEADT(i) (DatumGetDateADT(in_arg_##i))
#define PG_GETARG_TIMESTAMP(i) (DatumGetTimestamp(in_arg_##i))


#define INFO    17
#define elog  elog_start(__FILE__, __LINE__, __func__), elog_finish

extern void elog_start(const char *filename, int lineno, const char *funcname);
extern void elog_finish(int elevel, const char *fmt, ...);

#define SECS_PER_DAY	8640
static Timestamp
date2timestamp(DateADT dateVal)
{
	Timestamp	result;

#ifdef HAVE_INT64_TIMESTAMP
	/* date is days since 2000, timestamp is microseconds since same... */
	result = dateVal * USECS_PER_DAY;
	/* Date's range is wider than timestamp's, so must check for overflow */
	if (result / USECS_PER_DAY != dateVal)
		ereport(ERROR,
				(errcode(ERRCODE_DATETIME_VALUE_OUT_OF_RANGE),
				 errmsg("date out of range for timestamp")));
#else
	/* date is days since 2000, timestamp is seconds since same... */
	result = dateVal * (double) SECS_PER_DAY;
#endif

	return result;
}

int
timestamp_cmp_internal(Timestamp dt1, Timestamp dt2)
{
#ifdef HAVE_INT64_TIMESTAMP
	return (dt1 < dt2) ? -1 : ((dt1 > dt2) ? 1 : 0);
#else

	/*
	 * When using float representation, we have to be wary of NaNs.
	 *
	 * We consider all NANs to be equal and larger than any non-NAN. This is
	 * somewhat arbitrary; the important thing is to have a consistent sort
	 * order.
	 */
	if (isnan(dt1))
	{
		if (isnan(dt2))
			return 0;			/* NAN = NAN */
		else
			return 1;			/* NAN > non-NAN */
	}
	else if (isnan(dt2))
	{
		return -1;				/* non-NAN < NAN */
	}
	else
	{
		if (dt1 > dt2)
			return 1;
		else if (dt1 < dt2)
			return -1;
		else
			return 0;
	}
#endif
}


Datum
date_le(PG_FUNCTION_ARGS)
{
	DateADT		dateVal1 = PG_GETARG_DATEADT(0);
	DateADT		dateVal2 = PG_GETARG_DATEADT(1);

	PG_RETURN_BOOL(dateVal1 <= dateVal2);
}

Datum
date_le_timestamp(PG_FUNCTION_ARGS)
{
	DateADT		dateVal = PG_GETARG_DATEADT(0);
	Timestamp	dt2 = PG_GETARG_TIMESTAMP(1);
	Timestamp	dt1;

	dt1 = date2timestamp(dateVal);

	PG_RETURN_BOOL(timestamp_cmp_internal(dt1, dt2) <= 0);
}


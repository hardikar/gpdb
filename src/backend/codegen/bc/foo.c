#define INFO    17
#define elog  elog_start(__FILE__, __LINE__, __func__), elog_finish

extern void elog_start(const char *filename, int lineno, const char *funcname);
extern void elog_finish(int elevel, const char *fmt,...);

extern int extern_int;

int bar = 10;

void foo() {
  elog(INFO, "foo %d\n", extern_int);
  elog(INFO, "bar %d\n", bar++);
}

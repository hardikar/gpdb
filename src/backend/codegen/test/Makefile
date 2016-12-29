#-------------------------------------------------------------------------
#
# Makefile--
#    Codegen unit tests
#
#-------------------------------------------------------------------------

subdir = src/backend/codegen
top_builddir = ../../../..
include $(top_builddir)/src/Makefile.global

override CXXFLAGS := -I $(top_builddir)/$(subdir)/include $(CXXFLAGS)
override CXXFLAGS := $(CODEGEN_CXXFLAGS) $(CXXFLAGS)

TARGETS=codegen_framework_unittest \
        codegen_pg_func_generator_unittest \
        clang_compiler_unittest \
        instance_method_wrappers_unittest \
        gp_codegen_utils_unittest \
        codegen_utils_unittest

include $(top_builddir)/src/backend/mock.mk

codegen_framework_unittest.t:

codegen_pg_func_generator_unittest.t:

clang_compiler_unittest.t:

instance_method_wrappers_unittest.t:

gp_codegen_utils_unittest.t:

codegen_utils_unittest.t:


# Hooks into GPDB unittest system
all: $(patsubst %,%.t,$(TARGETS)) gtest_main.a gtest-all.o

GPCODEGEN_LIBPATH = $(top_builddir)/$(subdir)
CODEGEN_TEST_LIBS += -L$(GPCODEGEN_LIBPATH) -lgpcodegen

CODEGEN_OBJFILES=$(top_srcdir)/$(subdir)/objfiles.txt
CODEGEN_OBJECTS=$(addprefix $(top_srcdir)/, \
                 $(shell test -f $(CODEGEN_OBJFILES) && cat $(CODEGEN_OBJFILES)))

# The test target depends on $(OBJFILES) which would update files including mocks.
# Note that gpcodegen unit tests don't use the linkers wrap functionality. So there
# is no need to include that step here.
%.t: $(OBJFILES) $(CMOCKERY_OBJS) $(MOCK_OBJS) %.o gtest_main.a
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $(call BACKEND_OBJS, $(top_srcdir)/$(subdir)/$*.o $(patsubst $(MOCK_DIR)/%_mock.o,$(top_builddir)/src/%.o, $^)) $(filter-out %/objfiles.txt %/gpcodegen_mock.o, $^) $(MOCK_LIBS) $(CODEGEN_OBJECTS) $(CODEGEN_TEST_LIBS) -o $@

.PHONY:
%-check: %.t
	LD_LIBRARY_PATH="${GPCODEGEN_LIBPATH}:${LD_LIBRARY_PATH}" ./$*.t


# Compilation for gtest
GTEST_DIR = $(top_builddir)/gpAux/extensions/googletest/googletest

gtest-all.o: INCLUDES += -I$(GTEST_DIR)/ -I$(GTEST_DIR)/src -I$(GTEST_DIR)/include 
gtest-all.o:
	$(CXX) $(INCLUDES) $(CXXFLAGS) -c $(GTEST_DIR)/src/gtest-all.cc

gtest_main.a : gtest-all.o
	$(AR) -rv $@ $^

clean:
	rm -f *.o *.d *.a

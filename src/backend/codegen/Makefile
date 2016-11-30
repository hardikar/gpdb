#-------------------------------------------------------------------------
#
# Makefile--
#    Makefile for codegen wrappers
#
#-------------------------------------------------------------------------

subdir = src/backend/codegen
top_builddir = ../../..
include $(top_builddir)/src/Makefile.global

override CXXFLAGS := -I $(top_builddir)/$(subdir)/include $(CXXFLAGS)
override CXXFLAGS := $(CODEGEN_CXXFLAGS) $(CXXFLAGS)

# Objects that go into SUBSYS.o
OBJS = codegen_wrapper.o 

include $(top_srcdir)/src/backend/common.mk

# Additionaly build the libgpcodegen shared library
all install installdirs clean distclean:
	$(MAKE) -f Makefile.gpcodegen $@

//---------------------------------------------------------------------------
//  Greenplum Database
//  Copyright 2016 Pivotal Software, Inc.
//
//  @filename:
//    gp_codegen_utils.cc
//
//  @doc:
//    Object that extends the functionality of CodegenUtils by adding GPDB
//    specific functionality and utilities to aid in the runtime code generation
//    for a LLVM module
//
//  @test:
//    Unittests in tests/code_generator_unittest.cc
//
//
//---------------------------------------------------------------------------

#include "codegen/utils/gp_codegen_utils.h"

namespace gpcodegen {
  // Global elog_ostream matching C++'s cout
elog_ostream eout;

void elog_ostream::write_impl(const char *ptr, size_t size) {
  elog(INFO, "%.*s>", static_cast<int>(size), ptr);
}
}  // namespace gpcodegen

// EOF

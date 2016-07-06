//---------------------------------------------------------------------------
//  Greenplum Database
//  Copyright 2016 Pivotal Software, Inc.
//
//  @filename:
//    code_generator_unittest.cc
//
//  @doc:
//    Unit tests for codegen_manager.cc
//
//  @test:
//
//---------------------------------------------------------------------------

#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <ctime>
#include <initializer_list>
#include <limits>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "gtest/gtest.h"

#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/APInt.h"
#include "llvm/IR/Argument.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constant.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Value.h"
#include "llvm/IR/Verifier.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/Support/Casting.h"

#include "codegen/codegen_wrapper.h"
#include "codegen/utils/gp_codegen_utils.h"

extern "C" {
#define Assert(...)

#include "utils/elog.h"
#include "utils/palloc.h"
#include "utils/memutils.h"
}

extern bool codegen_validate_functions;
namespace gpcodegen {

// Test environment to handle global per-process initialization tasks for all
// tests.
class GpCodegenUtilsTestEnvironment : public ::testing::Environment {
 public:
  virtual void SetUp() {
    ASSERT_EQ(InitCodegen(), 1);
    MemoryContextInit();
  }
};

class GpCodegenUtilsTest : public ::testing::Test {
 protected:
  virtual void SetUp() {
    codegen_validate_functions = true;
    codegen_utils_.reset(new GpCodegenUtils("test_module"));
  }

  std::unique_ptr<GpCodegenUtils> codegen_utils_;
};

TEST_F(GpCodegenUtilsTest, TestTryCatch) {
  PG_TRY();
  {
    printf("In main code\n");
    elog(ERROR, "BOO");
  }
  PG_CATCH();
  {
    printf("In exception\n");
  }
  PG_END_TRY();
  printf("At the end\n");
  EXPECT_TRUE(true);
}

}  // namespace gpcodegen

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  AddGlobalTestEnvironment(new gpcodegen::GpCodegenUtilsTestEnvironment);
  return RUN_ALL_TESTS();
}


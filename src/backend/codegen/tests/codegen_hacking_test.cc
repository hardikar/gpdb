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
#include "llvm/Support/Casting.h"

#include "codegen/utils/codegen_utils.h"
#include "codegen/utils/utility.h"
#include "codegen/codegen_manager.h"
#include "codegen/codegen_wrapper.h"
#include "codegen/codegen_interface.h"
#include "codegen/base_codegen.h"


namespace gpcodegen {

// Test environment to handle global per-process initialization tasks for all
// tests.
class CodegenManagerTestEnvironment : public ::testing::Environment {
 public:
  virtual void SetUp() {
    ASSERT_EQ(InitCodegen(), 1);
  }
};

class CodegenManagerTest : public ::testing::Test {
 protected:
  virtual void SetUp() {
    manager_.reset(new CodegenManager("CodegenManagerTest"));
  }

  template <typename ClassType, typename FuncType>
  void EnrollCodegen(FuncType reg_func, FuncType* ptr_to_chosen_func) {
    ClassType* code_gen = new ClassType(reg_func, ptr_to_chosen_func);
    ASSERT_TRUE(reg_func == *ptr_to_chosen_func);
    ASSERT_TRUE(manager_->EnrollCodeGenerator(
        CodegenFuncLifespan_Parameter_Invariant,
        code_gen));
  }

  std::unique_ptr<CodegenManager> manager_;
};

TEST_F(CodegenManagerTest, TestGetters) {
  ASSERT_TRUE(true);
}

}  // namespace gpcodegen

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  AddGlobalTestEnvironment(new gpcodegen::CodegenManagerTestEnvironment);
  return RUN_ALL_TESTS();
}

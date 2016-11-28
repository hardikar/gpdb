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

extern "C" {
#include "postgres.h"  // NOLINT(build/include)
#undef newNode  // undef newNode so it doesn't have name collision with llvm
#include "utils/elog.h"
#undef elog
#define elog(...)
}

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
#include "llvm/IR/InstIterator.h"
#include "llvm/Support/Casting.h"
#include "llvm/IR/ModuleSlotTracker.h"


#include "codegen/utils/codegen_utils.h"
#include "codegen/utils/gp_codegen_utils.h"
#include "codegen/utils/utility.h"
#include "codegen/codegen_manager.h"
#include "codegen/codegen_wrapper.h"
#include "codegen/codegen_interface.h"
#include "codegen/base_codegen.h"

#include <iostream>

using gpcodegen::GpCodegenUtils;
namespace gpcodegen {

class TestEnvironment : public ::testing::Environment {
 public:
  virtual void SetUp() {
    ASSERT_EQ(InitCodegen(), 1);
  }
};

class Test : public ::testing::Test {
 protected:
  virtual void SetUp() {
    codegen_utils_.reset(new GpCodegenUtils("main_module"));
  }

  std::unique_ptr<GpCodegenUtils> codegen_utils_;
};

using FnType = int (*) (int);

std::map<llvm::Value*, std::string> value_map;
int counter = 0;

std::string& getName(llvm::Value* v) {
  auto it = value_map.find(v);
  std::string* name;
  if (it == value_map.end()) {
    value_map[v] = "v" + std::to_string(counter++);
    name = &value_map[v];
  } else {
    name = &it->second;
  }
  return *name;
}

void print_instruction(llvm::Instruction* inst) {
  std::string& name = getName(inst);

  switch(inst->getOpcode()) {
    case llvm::Instruction::Add:
      std::cout << "llvm::Value* " << name << " = ";
      std::cout << "CreateAdd(";
      for (auto o = inst->op_begin(), e = inst->op_end(); o != e; o++) {
        o->get()

      }
      std::cout << ")" << std::endl;
      break;
    case llvm::Instruction::Ret:
      std::cout << "CreateRet(";
      std::cout << ")" << std::endl;
      break;
    default:
      //std::cout<<"Unsupported" << std::endl;
      break;
  }
  //std::cout<<"llvm::Value* "
}

TEST_F(Test, TestMain) {
      llvm::Function *fn = codegen_utils_->CreateFunction<FnType>("fn");
      auto irb = codegen_utils_->ir_builder();


      llvm::BasicBlock *entry_block = codegen_utils_->CreateBasicBlock(
          "entry_block", fn);
      irb->SetInsertPoint(entry_block);
      llvm::Value *arg = ArgumentByPosition(fn, 0);
      llvm::Value *ret = irb->CreateAdd(arg, codegen_utils_->GetConstant(1));
      irb->CreateRet(ret);

      codegen_utils_->module()->dump();

      for (llvm::inst_iterator I = llvm::inst_begin(fn), E = llvm::inst_end(fn); I != E; ++I) {
        print_instruction(&*I);
      }



      //EXPECT_TRUE(codegen_utils_->PrepareForExecution(
      //    CodegenUtils::OptimizationLevel::kNone, true));

      //EXPECT_EQ(5, codegen_utils_->GetFunctionPointer<FnType>("fn") (4));
      EXPECT_TRUE(true);
}

}  // namespace gpcodegen

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  AddGlobalTestEnvironment(new gpcodegen::TestEnvironment);
  return RUN_ALL_TESTS();
}


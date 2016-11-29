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
#include <map>

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
#include <codegen/utils/clang_compiler.h>
#include "llvm/Support/raw_ostream.h"

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

llvm::Value* __c_to_llvm(gpcodegen::CodegenUtils* codegen_utils,
                         const llvm::Twine& source_code,
                         llvm::Type* return_type,
                         const std::map<std::string, llvm::Value*>& args) {
  std::unique_ptr<gpcodegen::ClangCompiler> clang_compiler;
  clang_compiler.reset(new gpcodegen::ClangCompiler(codegen_utils));

  std::string function_name = "func"; // TODO: auto generate this
  std::vector<llvm::Type*> argument_types;
  std::vector<llvm::Value*> arguments;

  std::string header;
  header.append(ScalarCppTypeFromLLVMType(*return_type));
  header.push_back(' ');
  header.append(function_name);
  header.push_back('(');

  for(auto it = args.begin(), end = args.end(); it != end; ) {
    const std::string& name = it->first;
    llvm::Value* value = it->second;
    const std::string& type = ScalarCppTypeFromLLVMType(*value->getType());
    argument_types.push_back(value->getType());
    arguments.push_back(value);

    header.append(type);
    header.push_back(' ');
    header.append(name);

    if (++it != end) {
      header.append(", ");
    }
  }
  header.push_back(')');


  const llvm::Twine& full_code = llvm::Twine("extern \"C\" {\n") +
                                 header +
                                 "\n{\n" +
                                 source_code +
                                 "\n}\n}\n";
  llvm::FunctionType* function_type =
      llvm::FunctionType::get(return_type,
                              argument_types,
                              false /* is_var_arg */);

  clang_compiler->CompileCppSource(full_code);
  llvm::Function* function =
      llvm::Function::Create(function_type,
                             llvm::GlobalValue::ExternalLinkage,
                             function_name,
                             codegen_utils->module());
  function->addFnAttr(llvm::Attribute::AttrKind::AlwaysInline);

  llvm::Value* return_value =
      codegen_utils->ir_builder()->CreateCall(function, arguments);

  std::cout << full_code.str();
  std::cout << "================" << std::endl << std::endl;
  return return_value;
}

TEST_F(Test, TestMain) {
      llvm::Function *fn = codegen_utils_->CreateFunction<FnType>("fn");
      auto irb = codegen_utils_->ir_builder();


      llvm::BasicBlock *entry_block = codegen_utils_->CreateBasicBlock(
          "entry_block", fn);
      irb->SetInsertPoint(entry_block);
      llvm::Value *arg = ArgumentByPosition(fn, 0);

      //for (llvm::inst_iterator I = llvm::inst_begin(fn), E = llvm::inst_end(fn); I != E; ++I) {
      //  print_instruction(&*I);
      //}

      llvm::Value *ret = __c_to_llvm(
          codegen_utils_.get(),
          "return foo + 1;",
          codegen_utils_->GetType<int>(),
          {{"foo", arg}});
      irb->CreateRet(ret);

      std::string dump;
      llvm::raw_string_ostream os(dump);

      codegen_utils_->PrintUnderlyingModules(os);
      os.flush();

      std::cout << dump << std::endl;


      EXPECT_TRUE(codegen_utils_->PrepareForExecution(
          CodegenUtils::OptimizationLevel::kNone, true));

      EXPECT_EQ(5, codegen_utils_->GetFunctionPointer<FnType>("fn") (4));
      EXPECT_TRUE(true);
}

}  // namespace gpcodegen

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  AddGlobalTestEnvironment(new gpcodegen::TestEnvironment);
  return RUN_ALL_TESTS();
}


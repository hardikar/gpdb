//---------------------------------------------------------------------------
//  Greenplum Database
//  Copyright (C) 2016 Pivotal Software, Inc.
//
//  @filename:
//    codegen_manager.cpp
//
//  @doc:
//    Implementation of a code generator manager
//
//---------------------------------------------------------------------------

extern "C" {
#include <utils/elog.h>
}

#include <cstdint>
#include <string>

#include "codegen/utils/clang_compiler.h"
#include "codegen/utils/utility.h"
#include "codegen/utils/instance_method_wrappers.h"
#include "codegen/utils/gp_codegen_utils.h"
#include "codegen/slot_getattr_codegen.h"
#include "codegen/codegen_interface.h"

#include "codegen/codegen_manager.h"
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

using gpcodegen::CodegenManager;
using gpcodegen::SlotGetAttrCodegen;

CodegenManager::CodegenManager(const std::string& module_name) {
  module_name_ = module_name;
  codegen_utils_.reset(new gpcodegen::GpCodegenUtils(module_name));
}

bool CodegenManager::EnrollCodeGenerator(
    CodegenFuncLifespan funcLifespan, CodegenInterface* generator) {
  // Only CodegenFuncLifespan_Parameter_Invariant is supported as of now
  assert(funcLifespan == CodegenFuncLifespan_Parameter_Invariant);
  assert(nullptr != generator);
  enrolled_code_generators_.emplace_back(generator);
  return true;
}

unsigned int CodegenManager::GenerateCode() {
  unsigned int success_count = 0;
  for (std::unique_ptr<CodegenInterface>& generator :
      enrolled_code_generators_) {
    success_count += generator->GenerateCode(codegen_utils_.get());
  }
  // Generate code for shared modules
  SlotGetAttrCodegen::GenerateSlotGetAttr(codegen_utils_.get());
  return success_count;
}

unsigned int CodegenManager::PrepareGeneratedFunctions() {
  unsigned int success_count = 0;

  // If no generator registered, just return with success count as 0
  if (enrolled_code_generators_.empty()) {
    return success_count;
  }


  // Call GpCodegenUtils to compile entire module
  bool compilation_status = codegen_utils_->PrepareForExecution(
      gpcodegen::GpCodegenUtils::OptimizationLevel::kDefault, true);

  if (!compilation_status) {
    return success_count;
  }

  // On successful compilation, go through all generator and swap
  // the pointer so compiled function get called
  gpcodegen::GpCodegenUtils* codegen_utils = codegen_utils_.get();
  for (std::unique_ptr<CodegenInterface>& generator :
      enrolled_code_generators_) {
    success_count += generator->SetToGenerated(codegen_utils);
  }
  return success_count;
}

void CodegenManager::NotifyParameterChange() {
  // no support for parameter change yet
  assert(false);
}

bool CodegenManager::InvalidateGeneratedFunctions() {
  // no support for invalidation of generated function
  assert(false);
  return false;
}

const std::string& CodegenManager::GetExplainString() {
  return explain_string_;
}

void CodegenManager::AccumulateExplainString() {
  explain_string_.clear();
  llvm::raw_string_ostream out(explain_string_);
  codegen_utils_->PrintUnderlyingModules(out);
}

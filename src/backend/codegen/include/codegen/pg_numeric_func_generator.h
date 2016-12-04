//---------------------------------------------------------------------------
//  Greenplum Database
//  Copyright (C) 2016 Pivotal Software, Inc.
//
//  @filename:
//    pg_numeric_func_generator.h
//
//  @doc:
//    Base class for numeric functions to generate code
//
//---------------------------------------------------------------------------

#ifndef GPDB_PG_NUMERIC_FUNC_GENERATOR_H_  // NOLINT(build/header_guard)
#define GPDB_PG_NUMERIC_FUNC_GENERATOR_H_

#include "codegen/utils/gp_codegen_utils.h"
#include "codegen/pg_func_generator.h"
#include "codegen/pg_func_generator_interface.h"

#include "llvm/IR/Constant.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Value.h"

extern "C" {
#include "postgres.h"  // NOLINT(build/include)
#include "c.h"  // NOLINT(build/include)
#include "utils/numeric.h"
}

namespace llvm {
class Value;
}

namespace gpcodegen {

/** \addtogroup gpcodegen
 *  @{
 */

class GpCodegenUtils;
struct PGFuncGeneratorInfo;

/**
 * @brief Class with Static member function to generate code for numeric
 *        operators.
 **/

class PGNumericFuncGenerator {
 public:
  template<typename CType>
  static bool CreateIntFloatAvgAccum(
      gpcodegen::GpCodegenUtils* codegen_utils,
      const gpcodegen::PGFuncGeneratorInfo& pg_func_info,
      llvm::Value** llvm_out_value);

  static bool CreateIntFloatAvgAmalg(
      gpcodegen::GpCodegenUtils* codegen_utils,
      const gpcodegen::PGFuncGeneratorInfo& pg_func_info,
      llvm::Value** llvm_out_value);

 private:
  static
  void CreateVarlenSizeCheck(gpcodegen::GpCodegenUtils* codegen_utils,
                             llvm::Value* llvm_ptr,
                             llvm::Value* llvm_size,
                             llvm::Value** llvm_out_cond);

  static
  void CreatePallocTransdata(gpcodegen::GpCodegenUtils* codegen_utils,
                             llvm::Value* llvm_in_transdata_ptr,
                             llvm::Value** llvm_out_trandata_ptr);
};

template<typename CType>
bool PGNumericFuncGenerator::CreateIntFloatAvgAccum(
    gpcodegen::GpCodegenUtils* codegen_utils,
    const gpcodegen::PGFuncGeneratorInfo& pg_func_info,
    llvm::Value** llvm_out_value) {
  // TODO(nikos): Can we figure if we need to detoast during generation?
  llvm::Function* llvm_pg_detoast_datum = codegen_utils->
      GetOrRegisterExternalFunction(pg_detoast_datum, "pg_detoast_datum");

  auto irb = codegen_utils->ir_builder();

  llvm::Value* llvm_in_transdata_ptr =
      irb->CreateCall(llvm_pg_detoast_datum, {pg_func_info.llvm_args[0]});
  llvm::Value* llvm_newval = codegen_utils->CreateCast<float8, CType>(
      pg_func_info.llvm_args[1]);

  llvm::Value* llvm_transdata_ptr;
  CreatePallocTransdata(codegen_utils,
                        llvm_in_transdata_ptr,
                        &llvm_transdata_ptr);

  llvm::Value* llvm_transdata_sum_ptr = codegen_utils->GetPointerToMember(
      llvm_transdata_ptr, &IntFloatAvgTransdata::sum);
  llvm::Value* llvm_transdata_count_ptr = codegen_utils->GetPointerToMember(
      llvm_transdata_ptr, &IntFloatAvgTransdata::count);

  irb->CreateStore(irb->CreateFAdd(
      irb->CreateLoad(llvm_transdata_sum_ptr),
      llvm_newval),
                   llvm_transdata_sum_ptr);

  irb->CreateStore(irb->CreateAdd(
      irb->CreateLoad(llvm_transdata_count_ptr),
      codegen_utils->GetConstant<int64>(1)),
                   llvm_transdata_count_ptr);

  *llvm_out_value = llvm_transdata_ptr;
  return true;
}

/** @} */
}  // namespace gpcodegen

#endif  // GPDB_PG_NUMERIC_FUNC_GENERATOR_H_

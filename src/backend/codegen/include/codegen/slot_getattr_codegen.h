//---------------------------------------------------------------------------
//  Greenplum Database
//  Copyright (C) 2016 Pivotal Software, Inc.
//
//  @filename:
//    slot_getattr_codegen.h
//
//  @doc:
//    Contains slot_getattr generator
//
//---------------------------------------------------------------------------

#ifndef GPCODEGEN_SLOT_GETATTR_CODEGEN_H_  // NOLINT(build/header_guard)
#define GPCODEGEN_SLOT_GETATTR_CODEGEN_H_

#include "codegen/codegen_wrapper.h"
#include "codegen/utils/gp_codegen_utils.h"

namespace gpcodegen {

/** \addtogroup gpcodegen
 *  @{
 */

class SlotGetAttrCodegen {
 public:
  static bool GenerateSlotGetAttr(
      gpcodegen::GpCodegenUtils* codegen_utils,
      const std::string& function_name,
      TupleTableSlot* slot,
      int max_attr,
      llvm::Function** out_func);

 private:
  static bool GenerateSlotGetAttrInternal(
      gpcodegen::GpCodegenUtils* codegen_utils,
      const std::string& function_name,
      TupleTableSlot* slot,
      int max_attr,
      llvm::Function** out_func);
};

/** @} */

}  // namespace gpcodegen
#endif // GPCODEGEN_SLOT_GETATTR_CODEGEN_H_

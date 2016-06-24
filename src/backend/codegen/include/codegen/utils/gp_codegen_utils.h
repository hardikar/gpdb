//---------------------------------------------------------------------------
//  Greenplum Database
//  Copyright 2016 Pivotal Software, Inc.
//
//  @filename:
//    codegen_utils.h
//
//  @doc:
//    Object that manages runtime code generation for a single LLVM module.
//
//  @test:
//
//
//---------------------------------------------------------------------------

#ifndef GPCODEGEN_GP_CODEGEN_UTILS_H_  // NOLINT(build/header_guard)
#define GPCODEGEN_GP_CODEGEN_UTILS_H_

#include "codegen/utils/codegen_utils.h"

namespace gpcodegen {

class GpCodegenUtils : public CodegenUtils {
 public:
  /**
   * @brief Constructor.
   *
   * @param module_name A human-readable name for the module that this
   *        CodegenUtils will manage.
   **/
  explicit GpCodegenUtils(llvm::StringRef module_name)
    : CodegenUtils(module_name) {
  }

  ~GpCodegenUtils() {
  }
};

}  // namespace gpcodegen

#endif  // GPCODEGEN_GP_CODEGEN_UTILS_H
// EOF

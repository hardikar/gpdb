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

#include <string>

#include "codegen/codegen_wrapper.h"
#include "codegen/utils/gp_codegen_utils.h"

namespace gpcodegen {

/** \addtogroup gpcodegen
 *  @{
 */

class SlotGetAttrCodegen {
 public:
  /**
   * @brief Request code generation for the codepath slot_getattr >
   * _slot_getsomeattr > slot_deform_tuple for the given slot and max_attr
   *
   * @param codegen_utils Utilities for easy code generation
   * @param slot          Use the TupleDesc from this slot to generate
   * @param max_attr      Generate slot deformation up to this many attributes
   * @param out_func      Return the llvm::Function that will be generated
   *
   * @note This method does not actually do any code generation, but simply
   * caches the information necessary for code generation when
   * GenerateSlotGetAttr() is called. This way we can de-duplicate multiple
   * requests for multiple slot_getattr() implementation producing only one per
   * slot.
   *
   **/
  static void RequestSlotGetAttrGeneration(
      gpcodegen::GpCodegenUtils* codegen_utils,
      TupleTableSlot* slot,
      int max_attr,
      llvm::Function** out_func);

  /**
   * @brief Generate code for the codepath slot_getattr > _slot_getsomeattr >
   * slot_deform_tuple for the given slot and max_attr
   *
   * @param codegen_utils Utilities for easy code generation
   *
   * @return true on success generation; false otherwise
   *
   * Based on parameters given to RequestSlotGetAttrGeneration(), the maximum
   * max_attr is tracked for each slot. Then for each slot, we generate code for
   * slot_getattr() that deforms tuples in that slot up to the maximum max_attr.
   *
   * @note This is a wrapper around GenerateSlotGetAttrInternal that handles the
   * case when generation fails, and cleans up a possible broken function by
   * removing it from the module.
   *
   * TODO(shardikar, krajaraman) Remove this wrapper after a shared code
   * generation framework implementation is complete.
   */
  static bool GenerateSlotGetAttr(
		  gpcodegen::GpCodegenUtils* codegen_utils);

 private:
  /**
   * @brief Generate code for the codepath slot_getattr > _slot_getsomeattr >
   * slot_deform_tuple
   *
   * @param codegen_utils Utilities for easy code generation
   * @param slot          Use the TupleDesc from this slot to generate
   * @param max_attr      Generate slot deformation up to this many attributes
   * @param out_func      Return the llvm::Function that will be generated
   *
   * @return true on success generation; false otherwise
   *
   * @note Generate code for code path slot_getattr > * _slot_getsomeattrs >
   * slot_deform_tuple. slot_getattr() will eventually call slot_deform_tuple
   * (through _slot_getsomeattrs), which fetches all yet unread attributes of
   * the slot until the given attribute.
   *
   * This implementation does not support:
   *  (1) Attributes passed by reference
   *
   * If at execution time, we see any of the above types of attributes,
   * we fall backs to the regular function.
   **/
  static bool GenerateSlotGetAttrInternal(
      gpcodegen::GpCodegenUtils* codegen_utils,
      TupleTableSlot* slot,
      int max_attr,
      llvm::Function* out_func);


  /**
   * Map of slot and the information required for its code generation,
   * which contains max_attr (attributes to deform up to), and the llvm::Function
   * whose body needs to be populated.
   *
   * i.e slot -> { max_attr, function }
   */
  static std::unordered_map<uint64_t, std::pair<int, llvm::Function*> >
      function_cache;
};

/** @} */

}  // namespace gpcodegen
#endif  // GPCODEGEN_SLOT_GETATTR_CODEGEN_H_

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

#include "codegen/ExecVariableList_codegen.h"
#include "codegen/utils/codegen_utils.h"
#include "codegen/utils/utility.h"
#include "codegen/codegen_manager.h"
#include "codegen/codegen_wrapper.h"
#include "codegen/codegen_interface.h"
#include "codegen/base_codegen.h"

extern "C" {
#include "postgres.h"
#include "nodes/execnodes.h"
#include "nodes/pg_list.h"
}


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
  FormData_pg_attribute formdata_attributes[20];
  FormData_pg_attribute* attrs[20];
  for(int i=0; i<20; i++) {
    attrs[i] = &formdata_attributes[i];
    attrs[i]->attlen = 8;
    attrs[i]->attnum = i + 1;
    attrs[i]->attbyval = true;
    attrs[i]->attstorage = 'p';
    attrs[i]->attalign = 'd';
  }

  struct tupleDesc tupleDesc = {
    20, // natts = 20
    attrs, // attrs = 0x0000000119988668
    nullptr, // constr = 0x0000000000000000
    16397, // tdtypeid = 16397
    -1, // tdtypmod = -1
    -1, // tdqdtypmod = -1
    false, // tdhasoid = false
    -1 // reference count -1 if not counting
  };

  struct TupleTableSlot slot;
  slot.PRIVATE_tts_heaptuple = nullptr; // 0x00007fc644025238
  slot.PRIVATE_tts_values = {};
  slot.PRIVATE_tts_isnull = {};
  slot.tts_tupleDescriptor = &tupleDesc; // 0x0000000119988638
  slot.tts_buffer = 154;
  slot.tts_tableOid = 16396;
  struct TupleTableSlot output_slot;

  // We need a list of size = 2
  ListCell one, two;
  one.next = &two;
  List targetlist;
  targetlist.head = &one;
  targetlist.tail = &two;
  targetlist.length = 2;

  ASSERT_EQ(2, list_length(&targetlist));

  // ExprContext, we only need the scantuple
  struct ExprContext exprContext;
  exprContext.ecxt_scantuple = &slot;

  int pi_varSlotOffsets[] = { 8, 8 };
  int pi_varNumbers[] = { 1, 4 };

  struct ProjectionInfo proj_info = {
    T_ProjectionInfo, // type
    &targetlist, //pi_targetlist = 0x00007fc6440213e0
    &exprContext, //pi_exprContext = 0x00007fc644021230
    &output_slot, //pi_slot = 0x00007fc644008340
    nullptr, //pi_itemIsDone = 0x0000000000000000
    true, //pi_isVarList = true
    pi_varSlotOffsets, //pi_varSlotOffsets = 0x00007fc6440099f8
    pi_varNumbers, //pi_varNumbers = 0x00007fc644021e30
    0, //pi_lastInnerVar = 0
    0, //pi_lastOuterVar = 0
    4, //pi_lastScanVar = 4
    nullptr, //ExecVariableList_gen_info = {
  };

  ExecVariableListFn generated_function;

  ExecVariableListCodegen* code_gen =
      new ExecVariableListCodegen(ExecVariableList,
                                  &generated_function,
                                  &proj_info,
                                  &slot);
  ASSERT_TRUE(manager_->EnrollCodeGenerator(
      CodegenFuncLifespan_Parameter_Invariant, code_gen));
  ASSERT_EQ(1, manager_->GenerateCode());
  ASSERT_EQ(true, code_gen->IsGenerated());

  ASSERT_TRUE(true);
}

}  // namespace gpcodegen

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  AddGlobalTestEnvironment(new gpcodegen::CodegenManagerTestEnvironment);
  return RUN_ALL_TESTS();
}

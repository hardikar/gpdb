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
#include <iostream>

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

// For perf instrumentation
#include <unistd.h>
#include <linux/perf_event.h>
#include <sys/ioctl.h>
#include <asm/unistd.h>


}

long
perf_event_open(struct perf_event_attr *hw_event, pid_t pid,
               int cpu, int group_fd, unsigned long flags) {
   int ret;

   ret = syscall(__NR_perf_event_open, hw_event, pid, cpu,
                  group_fd, flags);
   return ret;
}

int
perf_setup() {
  struct perf_event_attr pe;
  long long count;
  int fd;

  memset(&pe, 0, sizeof(struct perf_event_attr));
  pe.type = PERF_TYPE_HARDWARE;
  pe.size = sizeof(struct perf_event_attr);
  pe.config = PERF_COUNT_HW_INSTRUCTIONS;
  pe.disabled = 1;
  pe.exclude_kernel = 1;
  pe.exclude_hv = 1;

  fd = perf_event_open(&pe, 0 /* pid */, -1 /* cpu */, -1 /* group_fd */, 0 /* flags */);

  return fd;
}

void
perf_reset(int fd) {
  ioctl(fd, PERF_EVENT_IOC_RESET, 0);
}

void
perf_start(int fd) {
  ioctl(fd, PERF_EVENT_IOC_ENABLE, 0);
}

void
perf_stop(int fd) {
  ioctl(fd, PERF_EVENT_IOC_DISABLE, 0);
}

long long
perf_get(int fd) {
  long long count; 
  read(fd, &count, sizeof(long long));
  return count; 
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
    fd = perf_setup(); 
  }

  template <typename ClassType, typename FuncType>
  void EnrollCodegen(FuncType reg_func, FuncType* ptr_to_chosen_func) {
    ClassType* code_gen = new ClassType(reg_func, ptr_to_chosen_func);
    ASSERT_TRUE(reg_func == *ptr_to_chosen_func);
    ASSERT_TRUE(manager_->EnrollCodeGenerator(
        CodegenFuncLifespan_Parameter_Invariant,
        code_gen));
  }

  int fd; 
  ExecVariableListFn generated_function;
  std::unique_ptr<CodegenManager> manager_;
};

void DummyExecVariableList(ProjectionInfo* proj_info,
                           Datum *values, bool *isnull) {

  std::cerr<<"In dummy"<<std::endl;
}


TEST_F(CodegenManagerTest, TestGetters) {
  FormData_pg_attribute formdata_attributes[20];
  memset(formdata_attributes, 0, 20*sizeof(FormData_pg_attribute));
  FormData_pg_attribute* attrs[20];
  for(int i=0; i<20; i++) {
    attrs[i] = &formdata_attributes[i];
    attrs[i]->attlen = 8;
    attrs[i]->attnum = i + 1;
    attrs[i]->attbyval = true;
    attrs[i]->attcacheoff = 8 * i;
    attrs[i]->attstorage = 'p';
    attrs[i]->attalign = 'd';
  }

  char tuple_data_bytes[256];
  memset(tuple_data_bytes, 0, sizeof(tuple_data_bytes));

  // TODO : check if the infomasks change
  struct HeapTupleHeaderData heaptuple_header_data = {
      { { 1072, 0, 0 } },
      { 0, 1 },
      21, // t_infomask2 = 2068
      2048, // t_infomask = 2304
      '\x18', // t_hoff = '\x18'
      {u'\xff'} // t_bits = ([0] = '\0')
  };
  // Place the header and data into tuple_data_bytes
  memcpy(tuple_data_bytes, &heaptuple_header_data, sizeof(heaptuple_header_data));
  *(int64*)(tuple_data_bytes + sizeof(heaptuple_header_data) + 0) =  (int64) 42;
  *(int64*)(tuple_data_bytes + sizeof(heaptuple_header_data) + 8) =  (int64) 342;
  *(int64*)(tuple_data_bytes + sizeof(heaptuple_header_data) + 16) = (int64) 83;
  *(int64*)(tuple_data_bytes + sizeof(heaptuple_header_data) + 24) = (int64) 383;

  struct HeapTupleData heaptuple = {
     136, //  t_len = 136
     { 0, 1 }, // t_self
     (HeapTupleHeaderData*)&tuple_data_bytes // t_data = 0x000000010ca0df78
  };

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

  Datum PRIVATE_tts_values[16];
  bool PRIVATE_tts_isnull[16];

  struct TupleTableSlot slot;
  memset(&slot, 0, sizeof(TupleTableSlot));
  slot.PRIVATE_tts_heaptuple = &heaptuple; // 0x00007fc644025238
  slot.PRIVATE_tts_values = PRIVATE_tts_values;
  slot.PRIVATE_tts_isnull = PRIVATE_tts_isnull;
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


  ExecVariableListCodegen* code_gen =
      new ExecVariableListCodegen(DummyExecVariableList,
                                  &generated_function,
                                  &proj_info,
                                  &slot);
  ASSERT_TRUE(manager_->EnrollCodeGenerator(
      CodegenFuncLifespan_Parameter_Invariant, code_gen));
  ASSERT_EQ(1, manager_->GenerateCode());
  ASSERT_EQ(true, code_gen->IsGenerated());
  ASSERT_EQ(1, manager_->PrepareGeneratedFunctions());

  Datum values[16];
  bool isnull[16];

  time_t start, stop;
  clock_t ticks;

  const long NUM_RUNS = 1;

  time(&start);
  perf_reset(fd); 
  perf_start(fd);
  for (long i=0; i<NUM_RUNS; i++) {
    slot.PRIVATE_tts_nvalid = 0;
    slot.PRIVATE_tts_off = 0;
    slot.PRIVATE_tts_flags &= ~TTS_VIRTUAL;
    //values[0] = 0;
    ExecVariableList(&proj_info, values, isnull);
    //generated_function(&proj_info, values, isnull);
    //ASSERT_EQ(42, values[0]);
  }
  perf_stop(fd); 

  ticks = clock();
  time(&stop);
  printf("Used %0.2f seconds of CPU time. \n", (double)ticks/CLOCKS_PER_SEC);
  printf("Finished in about %.0f seconds. \n", difftime(stop, start));
  printf("Number of instructions: %lld. \n", perf_get(fd));  

  ASSERT_EQ(42, values[0]);
  ASSERT_EQ(383, values[1]);

  std::cerr<<values[0]<<std::endl;
  std::cerr<<values[1]<<std::endl;
  std::cerr<<values[2]<<std::endl;
 
  //ASSERT_TRUE(false);
}

}  // namespace gpcodegen

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  AddGlobalTestEnvironment(new gpcodegen::CodegenManagerTestEnvironment);
  return RUN_ALL_TESTS();
}

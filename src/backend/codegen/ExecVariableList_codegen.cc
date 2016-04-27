//---------------------------------------------------------------------------
//  Greenplum Database
//  Copyright (C) 2016 Pivotal Software, Inc.
//
//  @filename:
//    codegen_utils.cpp
//
//  @doc:
//    Contains different code generators
//
//---------------------------------------------------------------------------
#include <cstdint>
#include <string>

#include "codegen/ExecVariableList_codegen.h"
#include "codegen/utils/clang_compiler.h"
#include "codegen/utils/utility.h"
#include "codegen/utils/instance_method_wrappers.h"
#include "codegen/utils/codegen_utils.h"

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

extern "C" {
#include "postgres.h"
#include "utils/elog.h"
#include "access/htup.h"
#include "nodes/execnodes.h"
#include "executor/tuptable.h"
}

using gpcodegen::ExecVariableListCodegen;

constexpr char ExecVariableListCodegen::kExecVariableListPrefix[];

class ElogWrapper {
 public:
  ElogWrapper(gpcodegen::CodegenUtils* codegen_utils) :
    codegen_utils_(codegen_utils) {
    SetupElog();
  }
  ~ElogWrapper() {
    TearDownElog();
  }

  template<typename... V>
  void CreateElog(
      llvm::Value* llvm_elevel,
      llvm::Value* llvm_fmt,
      V ... args ) {


    assert(NULL != llvm_elevel);
    assert(NULL != llvm_fmt);

    codegen_utils_->ir_builder()->CreateCall(
        llvm_elog_start_, {
            codegen_utils_->GetConstant(""), // Filename
            codegen_utils_->GetConstant(0),  // line number
            codegen_utils_->GetConstant("")  // function name
    });
    codegen_utils_->ir_builder()->CreateCall(
        llvm_elog_finish_, {
            llvm_elevel,
            llvm_fmt,
            args...
    });
  }
  template<typename... V>
  void CreateElog(
      int elevel,
      const char* fmt,
      V ... args ) {
    CreateElog(codegen_utils_->GetConstant(elevel),
               codegen_utils_->GetConstant(fmt),
               args...);
  }
 private:
  llvm::Function* llvm_elog_start_;
  llvm::Function* llvm_elog_finish_;

  gpcodegen::CodegenUtils* codegen_utils_;

  void SetupElog(){
    assert(codegen_utils_ != nullptr);
    llvm_elog_start_ = codegen_utils_->RegisterExternalFunction(elog_start);
    assert(llvm_elog_start_ != nullptr);
    llvm_elog_finish_ = codegen_utils_->RegisterExternalFunction(elog_finish);
    assert(llvm_elog_finish_ != nullptr);
  }

  void TearDownElog(){
    llvm_elog_start_ = nullptr;
    llvm_elog_finish_ = nullptr;
  }

};

ExecVariableListCodegen::ExecVariableListCodegen
(
    ExecVariableListFn regular_func_ptr,
    ExecVariableListFn* ptr_to_regular_func_ptr,
    ProjectionInfo* proj_info,
    TupleTableSlot* slot) :
    BaseCodegen(
        kExecVariableListPrefix,
        regular_func_ptr,
        ptr_to_regular_func_ptr),
        proj_info_(proj_info),
        slot_(slot){
}

int GetMax(int* numbers, int length) {
  int i, max=-1;
  for(i=0; i < length; i++){
    if ( numbers[i] > max ) {
      max = numbers[i];
    }
  }
  return max;
}


bool ExecVariableListCodegen::GenerateExecVariableList(
    gpcodegen::CodegenUtils* codegen_utils) {

  ElogWrapper elogwrapper(codegen_utils);
  COMPILE_ASSERT(sizeof(Datum) == sizeof(int64));

  /*
   * Only do codegen if all the elements in this target list are on the same tuple slot
   * This is an OK assumption for scan nodes, but might fail when joins are involved.
   */
  if ( NULL == proj_info_->pi_varSlotOffsets ) {
    // Being in this state is ridiculous - we fucked up!
    elog(DEBUG1, "Cannot codegen ExecVariableList because varSlotOffsets are null");
    return false;
  }
  for (int i = list_length(proj_info_->pi_targetlist) - 1; i > 0; i--){
    if (proj_info_->pi_varSlotOffsets[i] != proj_info_->pi_varSlotOffsets[i-1]){
      elog(DEBUG1, "Cannot codegen ExecVariableList because multiple slots to deform.");
      return false;
    }
  }

  // Find the max attribute in projInfo->pi_targetlist
  int max_attr = GetMax(proj_info_->pi_varNumbers, list_length(proj_info_->pi_targetlist));

  /* System attribute */
  if(max_attr <= 0)
  {
    elog(DEBUG1, "Cannot generate code for ExecVariableList because max_attr is negative (i.e., system attribute).");
    return false;
  }else if (max_attr > slot_->tts_tupleDescriptor->natts) {
    elog(DEBUG1, "Cannot generate code for ExecVariableList because max_attr is greater than natts.");
    return false;
  }

  /* So looks like we're going to generate code */
  llvm::Function* ExecVariableList_func = codegen_utils->
      CreateFunction<ExecVariableListFn>(
          GetUniqueFuncName());

  auto irb = codegen_utils->ir_builder();

  /* BasicBlock of function entry. */
  llvm::BasicBlock* entry_block = codegen_utils->CreateBasicBlock(
      "entry", ExecVariableList_func);
  /* BasicBlock for checking correct slot */
  llvm::BasicBlock* slot_check_block = codegen_utils->CreateBasicBlock(
      "slot_check", ExecVariableList_func);
  /* BasicBlock for checking tuple type. */
  llvm::BasicBlock* tuple_type_check_block = codegen_utils->CreateBasicBlock(
      "tuple_type_check", ExecVariableList_func);
  /* BasicBlock for heap tuple check. */
  llvm::BasicBlock* heap_tuple_check_block = codegen_utils->CreateBasicBlock(
      "heap_tuple_check", ExecVariableList_func);
  /* BasicBlock for null check block. */
  llvm::BasicBlock* null_check_block = codegen_utils->CreateBasicBlock(
      "null_check", ExecVariableList_func);
  /* BasicBlock for main. */
  llvm::BasicBlock* main_block = codegen_utils->CreateBasicBlock(
      "main", ExecVariableList_func);
  /* BasicBlock for fallback. */
  llvm::BasicBlock* fallback_block = codegen_utils->CreateBasicBlock(
      "fallback", ExecVariableList_func);

  // External functions
  llvm::Function* llvm__slot_getsomeattrs = codegen_utils->RegisterExternalFunction(_slot_getsomeattrs);
  llvm::Function* llvm_memset = codegen_utils->RegisterExternalFunction(memset);

  // Constants
  llvm::Value* llvm_max_attr = codegen_utils->GetConstant(max_attr);
  llvm::Value* llvm_slot = codegen_utils->GetConstant(slot_);

  // Method arguments
  llvm::Value* llvm_projInfo_arg = ArgumentByPosition(ExecVariableList_func, 0);
  llvm::Value* llvm_values_arg = ArgumentByPosition(ExecVariableList_func, 1);
  llvm::Value* llvm_isnull_arg = ArgumentByPosition(ExecVariableList_func, 2);

  /*
   * Entry Block
   */
  irb->SetInsertPoint(entry_block);
  irb->CreateBr(slot_check_block);

  irb->SetInsertPoint(slot_check_block);
  llvm::Value* llvm_econtext =
      irb->CreateLoad(codegen_utils->GetPointerToMember(
          llvm_projInfo_arg, &ProjectionInfo::pi_exprContext));
  llvm::Value* llvm_varSlotOffsets =
      irb->CreateLoad(codegen_utils->GetPointerToMember(
          llvm_projInfo_arg, &ProjectionInfo::pi_varSlotOffsets));

  // We want to fall back when ExecVariableList is called with a slot that's different from the
  // one we generated the function (eg HashJoin). We also assume only 1 slot and that the slot is
  // in a scan node ie from exprContext->ecxt_scantuple or varOffset = 0
  llvm::Value* llvm_slot_arg =
      irb->CreateLoad(codegen_utils->GetPointerToMember(
          llvm_econtext, &ExprContext::ecxt_scantuple));

  irb->CreateCondBr(
      irb->CreateICmpEQ(llvm_slot, llvm_slot_arg),
      tuple_type_check_block /* true */,
      fallback_block /* false */
  );

  /*
   * Tuple Type check
   * we fall back if we see a virtual tuple or mem tuple, but it's possible to partially handle those cases also
   */
  irb->SetInsertPoint(tuple_type_check_block);
  llvm::Value* llvm_slot_PRIVATE_tts_flags_ptr =
      codegen_utils->GetPointerToMember(
          llvm_slot, &TupleTableSlot::PRIVATE_tts_flags);
  llvm::Value* llvm_slot_PRIVATE_tts_memtuple =
      irb->CreateLoad(codegen_utils->GetPointerToMember(
          llvm_slot, &TupleTableSlot::PRIVATE_tts_memtuple));

  /* (slot->PRIVATE_tts_flags & TTS_VIRTUAL) != 0 */
  llvm::Value* llvm_tuple_is_virtual = irb->CreateICmpNE(
      irb->CreateAnd(
          irb->CreateLoad(llvm_slot_PRIVATE_tts_flags_ptr),
          codegen_utils->GetConstant(TTS_VIRTUAL)),
          codegen_utils->GetConstant(0));

  /* slot->PRIVATE_tts_memtuple != NULL */
  llvm::Value* llvm_tuple_has_memtuple = irb->CreateICmpNE(
      llvm_slot_PRIVATE_tts_memtuple, codegen_utils->GetConstant((MemTuple) NULL));

  /* Fallback if tuple is virtual or memtuple is null */
  irb->CreateCondBr(
      irb->CreateOr(llvm_tuple_is_virtual, llvm_tuple_has_memtuple),
      fallback_block /*true*/, heap_tuple_check_block /*false*/);

  /*
   * HeapTuple check
   * _slot_getsomeattrs: TupGetHeapTuple(slot) != NULL
   */
  irb->SetInsertPoint(heap_tuple_check_block);
  llvm::Value* llvm_slot_PRIVATE_tts_heaptuple =
      irb->CreateLoad(codegen_utils->GetPointerToMember(
          llvm_slot, &TupleTableSlot::PRIVATE_tts_heaptuple));
  llvm::Value* llvm_tuple_has_heaptuple = irb->CreateICmpNE(
      llvm_slot_PRIVATE_tts_heaptuple, codegen_utils->GetConstant((HeapTuple) NULL));
  irb->CreateCondBr(llvm_tuple_has_heaptuple, null_check_block /*true*/, fallback_block /*false*/);


  /*
   * null check
   * slot should not have any null attributes
   */
  irb->SetInsertPoint(null_check_block);

  /*
   * attno = HeapTupleHeaderGetNatts(tuple->t_data);
   * attno = Min(attno, attnum);
   */
  llvm::Value* llvm_heaptuple_t_data =
      irb->CreateLoad(codegen_utils->GetPointerToMember(
          llvm_slot_PRIVATE_tts_heaptuple,
          &HeapTupleData::t_data
      ));
  llvm::Value* llvm_heaptuple_t_data_t_infomask2 =
      irb->CreateLoad(codegen_utils->GetPointerToMember(
          llvm_heaptuple_t_data,
          &HeapTupleHeaderData::t_infomask2));
  llvm::Value* llvm_attno_t0 =
      irb->CreateZExt(
          irb->CreateAnd(llvm_heaptuple_t_data_t_infomask2,
                         codegen_utils->GetConstant<int16>(HEAP_NATTS_MASK)),
                         codegen_utils->GetType<int>());

  llvm::Value* llvm_attno =
      irb->CreateSelect(
          irb->CreateICmpSLT(llvm_attno_t0, llvm_max_attr),
          llvm_attno_t0,
          llvm_max_attr
      );

  // slot's PRIVATE variables
  llvm::Value* llvm_slot_PRIVATE_tts_isnull /* bool* */=
      irb->CreateLoad(codegen_utils->GetPointerToMember(
          llvm_slot, &TupleTableSlot::PRIVATE_tts_isnull));
  llvm::Value* llvm_slot_PRIVATE_tts_values /* Datum* */=
      irb->CreateLoad(codegen_utils->GetPointerToMember(
          llvm_slot, &TupleTableSlot::PRIVATE_tts_values));
  llvm::Value* llvm_slot_PRIVATE_tts_nvalid_ptr /* int* */=
      codegen_utils->GetPointerToMember(llvm_slot, &TupleTableSlot::PRIVATE_tts_nvalid);

  // Start of slot_deform_tuple

  llvm::Value* llvm_heaptuple_t_data_t_hoff = irb->CreateLoad(
      codegen_utils->GetPointerToMember(llvm_heaptuple_t_data,
                                        &HeapTupleHeaderData::t_hoff));
  llvm::Value* llvm_heaptuple_t_data_t_infomask =
      irb->CreateLoad( codegen_utils->GetPointerToMember(
          llvm_heaptuple_t_data, &HeapTupleHeaderData::t_infomask));

  /* llvm_heap_tuple_has_null != 0 means we have nulls  */
  irb->CreateCondBr(
      irb->CreateICmpNE(
          irb->CreateAnd(
              llvm_heaptuple_t_data_t_infomask,
              codegen_utils->GetConstant<uint16>(HEAP_HASNULL)),
              codegen_utils->GetConstant<uint16>(0)),
              fallback_block, /* true */
              main_block /* false */
  );

  /*
   * Main block
   * Finally we try to codegen slot_deform_tuple
   */
  irb->SetInsertPoint(main_block);

  /* Find the start of input data byte array */
  // tp - ptr to tuple data
  llvm::Value* llvm_tuple_data_ptr = irb->CreateInBoundsGEP(
      llvm_heaptuple_t_data, {llvm_heaptuple_t_data_t_hoff});

  int off = 0;
  TupleDesc tupleDesc = slot_->tts_tupleDescriptor;
  Form_pg_attribute* att = tupleDesc->attrs;

  for (int attnum = 0; attnum < max_attr; ++attnum) {
    Form_pg_attribute thisatt = att[attnum];
    off = att_align(off, thisatt->attalign);

    // If any thisatt is varlen
    if (thisatt->attlen < 0) {
      /* We don't support variable length attributes */
      return false;
    }

    // values[attnum] = fetchatt(thisatt, tp = off) {{{
    llvm::Value* llvm_next_t_data_ptr =
        irb->CreateInBoundsGEP(llvm_tuple_data_ptr,
                               {codegen_utils->GetConstant(off)});

    llvm::Value* llvm_next_values_ptr =
        irb->CreateInBoundsGEP(llvm_slot_PRIVATE_tts_values,
                               {codegen_utils->GetConstant(attnum)});

    llvm::Value* llvm_colVal = nullptr;
    if( thisatt->attbyval) {
      // Load the value from the calculated input address.
      switch(thisatt->attlen)
      {
        case sizeof(char):
              // Read 1 byte at next_address_load
              llvm_colVal = irb->CreateLoad(llvm_next_t_data_ptr);
        // store colVal into out_values[attnum]
        break;
        case sizeof(int16):
              llvm_colVal = irb->CreateLoad(
                  codegen_utils->GetType<int16>(),irb->CreateBitCast(
                      llvm_next_t_data_ptr, codegen_utils->GetType<int16*>()));
        break;
        case sizeof(int32):
              llvm_colVal = irb->CreateLoad(
                  codegen_utils->GetType<int32>(),
                  irb->CreateBitCast(llvm_next_t_data_ptr,
                                     codegen_utils->GetType<int32*>()));
        break;
        case sizeof(int64):  /* Size of Datum */
              llvm_colVal = irb->CreateLoad(
                  codegen_utils->GetType<int64>(), irb->CreateBitCast(
                      llvm_next_t_data_ptr, codegen_utils->GetType<int64*>()));
        break;
        default:
          /* Manager will clean up the incomplete generated code. */
          return false;
      }
    } else {
      // We do not support values by reference
      return false;
      //llvm_colVal = irb->CreateLoad(irb->CreateBitCast(
      //    llvm_next_t_data_ptr, codegen_utils->GetType<Datum*>()));
    }

    irb->CreateStore(
        irb->CreateZExt(llvm_colVal, codegen_utils->GetType<Datum>()),
        llvm_next_values_ptr);

    // }}}

    // isnull[attnum] = false; {{{
    llvm::Value* llvm_next_isnull_ptr =
        irb->CreateInBoundsGEP(llvm_slot_PRIVATE_tts_isnull,
                               {codegen_utils->GetConstant(attnum)});
    irb->CreateStore(
        codegen_utils->GetConstant<bool>(false),
        llvm_next_isnull_ptr);
    // }}}

    off += thisatt->attlen;
  }


  // slot->PRIVATE_tts_off = off;
  llvm::Value* llvm_slot_PRIVATE_tts_off_ptr /* long* */=
      codegen_utils->GetPointerToMember(
          llvm_slot, &TupleTableSlot::PRIVATE_tts_off);
  irb->CreateStore(codegen_utils->GetConstant<long>(off),
                   llvm_slot_PRIVATE_tts_off_ptr);

  // slot->PRIVATE_tts_slow = slow;
  llvm::Value* llvm_slot_PRIVATE_tts_slow_ptr /* bool* */=
      codegen_utils->GetPointerToMember(
          llvm_slot, &TupleTableSlot::PRIVATE_tts_slow);
  irb->CreateStore(codegen_utils->GetConstant<bool>(false),
                   llvm_slot_PRIVATE_tts_slow_ptr);


  // End of slot_deform_tuple

  /*
   * The loop unrolling magic
   * Note this implements the code from ExecVariableList that copies the contents
   * of the isnull & values in the slot and copies them to output variables
   */
  int  *varNumbers = proj_info_->pi_varNumbers;
  for (int i = list_length(proj_info_->pi_targetlist) - 1; i >= 0; i--)
  {
    //  	values[i] = slot_getattr(varSlot, varNumbers[i] -1 +1, &(isnull[i]));
    //  	*isnull = slot->PRIVATE_tts_isnull[attnum-1];
    //  	*(&(isnull[i])) = slot->PRIVATE_tts_isnull[ varNumbers[i] - 1];
    //  	values[i] = varSlot->PRIVATE_tts_values[varNumbers[i] -1];

    llvm::Value* llvm_isnull_from_slot_val =
        irb->CreateLoad(irb->CreateInBoundsGEP(llvm_slot_PRIVATE_tts_isnull,
                                               {codegen_utils->GetConstant(varNumbers[i] - 1)}));
    llvm::Value* llvm_isnull_ptr =
        irb->CreateInBoundsGEP(llvm_isnull_arg, {codegen_utils->GetConstant(i)});
    irb->CreateStore(llvm_isnull_from_slot_val, llvm_isnull_ptr);

    llvm::Value* llvm_value_from_slot_val =
        irb->CreateLoad(irb->CreateInBoundsGEP(llvm_slot_PRIVATE_tts_values,
                                               {codegen_utils->GetConstant(varNumbers[i] - 1)}));
    llvm::Value* llvm_values_ptr =
        irb->CreateInBoundsGEP(llvm_values_arg, {codegen_utils->GetConstant(i)});
    irb->CreateStore(llvm_value_from_slot_val, llvm_values_ptr);
  }


  // TODO: we can avoid this {{{
  //  for (; attno < attnum; attno++)
  // {
  //   slot->PRIVATE_tts_values[attno] = (Datum) 0;
  //   slot->PRIVATE_tts_isnull[attno] = true;
  // }

  codegen_utils->ir_builder()->CreateCall(
      llvm_memset, {
          irb->CreateBitCast(
              irb->CreateInBoundsGEP(llvm_slot_PRIVATE_tts_values, {llvm_attno}),
              codegen_utils->GetType<void*>()),
              codegen_utils->GetConstant(0),
              irb->CreateMul(codegen_utils->GetConstant(sizeof(Datum)),
                             irb->CreateZExtOrTrunc(irb->CreateSub(llvm_max_attr, llvm_attno), codegen_utils->GetType<size_t>()))});

  codegen_utils->ir_builder()->CreateCall(
      llvm_memset, {
          irb->CreateBitCast(
              irb->CreateInBoundsGEP(llvm_slot_PRIVATE_tts_isnull, {llvm_attno}),
              codegen_utils->GetType<void*>()),
              codegen_utils->GetConstant((int) true),
              irb->CreateMul(codegen_utils->GetConstant(sizeof(Datum)),
                             irb->CreateZExtOrTrunc(irb->CreateSub(llvm_max_attr, llvm_attno), codegen_utils->GetType<size_t>()))});
  // }}}

  /* slot->PRIVATE_tts_nvalid = attnum; */
  irb->CreateStore(llvm_max_attr, llvm_slot_PRIVATE_tts_nvalid_ptr);

  /* TupSetVirtualTuple(slot); */
  irb->CreateStore(
      irb->CreateOr(
          irb->CreateLoad(llvm_slot_PRIVATE_tts_flags_ptr),
          codegen_utils->GetConstant<int>(TTS_VIRTUAL)),
          llvm_slot_PRIVATE_tts_flags_ptr);

  codegen_utils->ir_builder()->CreateRetVoid();

  /*
   * Fallback block
   */
  irb->SetInsertPoint(fallback_block);
  llvm::PHINode* llvm_error = irb->CreatePHI(codegen_utils->GetType<int>(), 4);
  llvm_error->addIncoming(codegen_utils->GetConstant(0), slot_check_block);
  llvm_error->addIncoming(codegen_utils->GetConstant(1), tuple_type_check_block);
  llvm_error->addIncoming(codegen_utils->GetConstant(2), heap_tuple_check_block);
  llvm_error->addIncoming(codegen_utils->GetConstant(3), null_check_block);

  elogwrapper.CreateElog(DEBUG1, "Falling back to regular ExecVariableList, reason = %d", llvm_error);

  codegen_utils->CreateFallback<ExecVariableListFn>(
      codegen_utils->RegisterExternalFunction(GetRegularFuncPointer()),
      ExecVariableList_func);
  return true;
}


bool ExecVariableListCodegen::GenerateCodeInternal(CodegenUtils* codegen_utils) {
  bool isGenerated = GenerateExecVariableList(codegen_utils);

  if (isGenerated)
  {
    elog(DEBUG1, "ExecVariableList was generated successfully!");
    return true;
  }
  else
  {
    elog(DEBUG1, "ExecVariableList generation failed!");
    return false;
  }
}

#include <llvm/IR/Module.h>
#include <llvm/Analysis/CallGraph.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/Transforms/Utils/Cloning.h>

#include <iostream>
#include <cstdint>
#include <cstdio>
#include <cstdarg>

#include "codegen/utils/gp_codegen_utils.h"
#include "codegen/utils/utility.h"

extern "C" {
#include "postgres.h"
#include "fmgr.h"
}

using namespace llvm;

using DoublesFn = double (*) (double, double);
using AddInts = int (*) (int, int);

using PGFunc = Datum (*) (PG_FUNCTION_ARGS);


extern llvm::Module* makeLLVMModule(llvm::Module* mod);

int main() {
  bool init_ok = gpcodegen::CodegenUtils::InitializeGlobal();
  std::unique_ptr<gpcodegen::GpCodegenUtils> codegen_utils(
   new gpcodegen::GpCodegenUtils("codegen_load_module_unittest"));

  llvm::Module*  module = codegen_utils->module();
  auto irb = codegen_utils->ir_builder();

  makeLLVMModule(module);
  llvm::Type* fcinfo_type = module->getTypeByName("struct.FunctionCallInfoData");

  Function* test_fn = codegen_utils->CreateFunction<PGFunc>("test_fn");
  BasicBlock* main_block = codegen_utils->CreateBasicBlock("main", test_fn);
  irb->SetInsertPoint(main_block);
  Value* fcinfo = irb->CreateBitCast(
      gpcodegen::ArgumentByPosition(test_fn, 0),
      fcinfo_type->getPointerTo());

  // Call return a + b here
  llvm::Function* int4pl = codegen_utils->module()->getFunction("int4pl");
  irb->CreateRet(irb->CreateCall(int4pl, {fcinfo}));


  bool boo = codegen_utils->PrepareForExecution(gpcodegen::CodegenUtils::OptimizationLevel::kDefault,
                                     false);
  PGFunc fn = codegen_utils->GetFunctionPointer<PGFunc>("test_fn");

  FunctionCallInfoData fcinfo_data;
  fcinfo_data.arg[0] = Int64GetDatum(12);
  fcinfo_data.arg[1] = Int64GetDatum(20);
  std::cout<<DatumGetInt64(fn(&fcinfo_data))<<std::endl;
  std::cout<<"END"<<std::endl;
  return 0;
}

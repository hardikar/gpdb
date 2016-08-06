#include <llvm/IR/Module.h>
#include <llvm/Analysis/CallGraph.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/Transforms/Utils/Cloning.h>

#include <iostream>
#include <cstdint>

#include "codegen/utils/codegen_utils.h"
#include "codegen/utils/utility.h"

using namespace llvm;

extern unsigned char builtins_bc[];
extern unsigned int builtins_bc_len;

typedef double (*AddDoubles)(double, double);


int main()
{
  bool init_ok = gpcodegen::CodegenUtils::InitializeGlobal();
  std::unique_ptr<gpcodegen::CodegenUtils> codegen_utils(
   new gpcodegen::CodegenUtils("codegen_load_module_unittest"));
  auto irb = codegen_utils->ir_builder();

  SMDiagnostic error;
  llvm::StringRef strref = { (const char*) builtins_bc, builtins_bc_len };
  llvm::MemoryBufferRef bufref = {strref, "builtins_bc"};
  std::unique_ptr<Module> builtins_module = llvm::parseIR(bufref, error, *codegen_utils->context());

  Function* add_once_fn = builtins_module->getFunction("float8pl");

  add_once_fn->dump();

  Function* add_twice = codegen_utils->CreateFunction<AddDoubles>("add_twice");
  BasicBlock* main_block = codegen_utils->CreateBasicBlock("main", add_twice);
  irb->SetInsertPoint(main_block);
  Value* arg0 = gpcodegen::ArgumentByPosition(add_twice, 0);
  Value* arg1 = gpcodegen::ArgumentByPosition(add_twice, 1);
  CallInst* add_one = irb->CreateCall(add_once_fn, {arg0, arg1});
  CallInst* add_two = irb->CreateCall(add_once_fn, {arg0, arg1});
  irb->CreateRet(irb->CreateFAdd(add_one, add_two));

  codegen_utils->module()->dump();

  //InlineFunctionInfo info;
  //InlineFunction(CallSite(add_one), info);
  //InlineFunction(CallSite(add_two), info);

  codegen_utils->module()->dump();
  //bool boo = codegen_utils->PrepareForExecution(gpcodegen::CodegenUtils::OptimizationLevel::kDefault,
  //                                   false);
  //AddThreeFn fn = codegen_utils->GetFunctionPointer<AddThreeFn>("addThree");
  //std::cout<<fn(12)<<std::endl;
  std::cout<<"END"<<std::endl;
  return 0;
}

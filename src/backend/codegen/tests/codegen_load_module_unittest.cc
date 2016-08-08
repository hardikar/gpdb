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
typedef int (*AddInts)(int, int);

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

  Function* add_once_fn = codegen_utils->InsertAlienFunction(builtins_module->getFunction("int4pl"), true);

  add_once_fn->dump();

  Function* add_twice = codegen_utils->CreateFunction<AddInts>("add_twice");
  BasicBlock* main_block = codegen_utils->CreateBasicBlock("main", add_twice);
  irb->SetInsertPoint(main_block);
  Value* arg0 = gpcodegen::ArgumentByPosition(add_twice, 0);
  Value* arg1 = gpcodegen::ArgumentByPosition(add_twice, 1);
  CallInst* add_one = irb->CreateCall(add_once_fn, {arg0, arg1});
  irb->CreateRet(add_one);
  //CallInst* add_two = irb->CreateCall(add_once_fn, {arg0, arg1});
  //irb->CreateRet(irb->CreateAdd(add_one, add_two));

  codegen_utils->module()->dump();

  //codegen_utils->InlineFunction(add_one);
  //codegen_utils->InlineFunction(add_two);

  codegen_utils->module()->dump();
  bool boo = codegen_utils->PrepareForExecution(gpcodegen::CodegenUtils::OptimizationLevel::kDefault,
                                     false);
  AddInts fn = codegen_utils->GetFunctionPointer<AddInts>("add_twice");
  std::cout<<fn(12, 10)<<std::endl;
  std::cout<<"END"<<std::endl;
  return 0;
}

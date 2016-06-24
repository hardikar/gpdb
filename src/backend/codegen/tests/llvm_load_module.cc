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

typedef int (* AddThreeFn) (int);

int main()
{
  bool init_ok = gpcodegen::CodegenUtils::InitializeGlobal();
  std::unique_ptr<gpcodegen::CodegenUtils> codegen_utils(
   new gpcodegen::CodegenUtils("add"));
  auto irb = codegen_utils->ir_builder();

  SMDiagnostic error;
  std::unique_ptr<Module> m = parseIRFile("tests/test.bc", error,
                                          codegen_utils->context());

  Function* add_one_fn = m->getFunction("addOne");
  Function* add_two_fn = m->getFunction("addTwo");

  add_one_fn->dump();
  add_two_fn->dump();

  Function* add_three_fn = codegen_utils->CreateFunction<AddThreeFn>("addThree");
  BasicBlock* main_block = codegen_utils->CreateBasicBlock("main", add_three_fn);
  irb->SetInsertPoint(main_block);
  Value* arg0 = gpcodegen::ArgumentByPosition(add_three_fn, 0);
  CallInst* add_one = irb->CreateCall(add_one_fn, {arg0});
  CallInst* add_two = irb->CreateCall(add_two_fn, {add_one});
  irb->CreateRet(irb->CreateAdd(add_one, add_two));

  codegen_utils->module()->dump();

  InlineFunctionInfo info;
  InlineFunction(CallSite(add_one), info);
  InlineFunction(CallSite(add_two), info);
  codegen_utils->addModule(m);

  codegen_utils->module()->dump();
  bool boo = codegen_utils->PrepareForExecution(gpcodegen::CodegenUtils::OptimizationLevel::kDefault,
                                     false);
  AddThreeFn fn = codegen_utils->GetFunctionPointer<AddThreeFn>("addThree");
  std::cout<<fn(12)<<std::endl;
  std::cout<<"END"<<std::endl;
  return 0;
}

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

using namespace llvm;

typedef double (*DoublesFn)(double, double);
typedef int (*AddInts)(int, int);

extern llvm::Module* makeLLVMModule(llvm::Module* mod);

extern "C" {
//extern std::unique_ptr<llvm::Module> builtins_module;
//void elog_start(const char *filename, int lineno, const char *funcname) {
//  std::cout<<("elog_start called") << std::endl;
//}
//void elog_finish(int elevel, const char *fmt,...) {
//  std::cout<<("elog_end called") << std::endl;
//  va_list args;
//  va_start(args, fmt);
//  vprintf(fmt, args);
//  va_end(args);
//}
}

int main() {
  bool init_ok = gpcodegen::CodegenUtils::InitializeGlobal();
  std::unique_ptr<gpcodegen::GpCodegenUtils> codegen_utils(
   new gpcodegen::GpCodegenUtils("codegen_load_module_unittest"));

  makeLLVMModule(codegen_utils->module());

  auto irb = codegen_utils->ir_builder();

  Function* test_fn = codegen_utils->CreateFunction<AddInts>("test_fn");
  BasicBlock* main_block = codegen_utils->CreateBasicBlock("main", test_fn);
  irb->SetInsertPoint(main_block);
  Value* arg0 = gpcodegen::ArgumentByPosition(test_fn, 0);
  Value* arg1 = gpcodegen::ArgumentByPosition(test_fn, 1);

  // Call return a + b here
  llvm::Function* int4pl = codegen_utils->module()->getFunction("int4pl");
  irb->CreateRet(irb->CreateCall(int4pl, {arg0, arg1}));


  bool boo = codegen_utils->PrepareForExecution(gpcodegen::CodegenUtils::OptimizationLevel::kDefault,
                                     false);
  AddInts fn = codegen_utils->GetFunctionPointer<AddInts>("test_fn");
  std::cout<<fn(12, 10)<<std::endl;
  std::cout<<"END"<<std::endl;
  return 0;
}

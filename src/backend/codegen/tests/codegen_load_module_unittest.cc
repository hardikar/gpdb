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

extern "C" {
extern std::unique_ptr<llvm::Module> builtins_module;
void elog_start(const char *filename, int lineno, const char *funcname) {
  std::cout<<("elog_start called") << std::endl;
}
void elog_finish(int elevel, const char *fmt,...) {
  std::cout<<("elog_end called") << std::endl;
  va_list args;
  va_start(args, fmt);
  vprintf(fmt, args);
  va_end(args);
}
}

int main() {
  bool init_ok = gpcodegen::CodegenUtils::InitializeGlobal();
  std::unique_ptr<gpcodegen::GpCodegenUtils> codegen_utils(
   new gpcodegen::GpCodegenUtils("codegen_load_module_unittest"));
  auto irb = codegen_utils->ir_builder();

  // Looks like we don't need to register these at all.
  // llvm::RuntimeDyldImpl::resolveExternalSymbols resolves all these correct to
  // symbols defined in the current executable

  //codegen_utils->GetOrRegisterExternalFunction(elog_start, "_elog_start");
  //codegen_utils->GetOrRegisterExternalFunction(elog_finish, "_elog_finish");

  std::cout << "================================================================================" << std::endl;
  //builtins_module->dump();
  std::cout << "================================================================================" << std::endl;

  //builtins_module->getFunction("float8pl")->getFunctionType()->dump();

  Function* float8pl = codegen_utils->InsertAlienFunction(builtins_module->getFunction("float8pl"), true);
  Function* float8mi = codegen_utils->InsertAlienFunction(builtins_module->getFunction("float8mi"), true);
  Function* float8mul = codegen_utils->InsertAlienFunction(builtins_module->getFunction("float8mul"), true);
  Function* float8div = codegen_utils->InsertAlienFunction(builtins_module->getFunction("float8div"), true);

  Function* test_fn = codegen_utils->CreateFunction<DoublesFn>("test_fn");
  BasicBlock* main_block = codegen_utils->CreateBasicBlock("main", test_fn);
  irb->SetInsertPoint(main_block);
  Value* arg0 = codegen_utils->CreateCppTypeToDatumCast(gpcodegen::ArgumentByPosition(test_fn, 0), false);
  Value* arg1 = codegen_utils->CreateCppTypeToDatumCast(gpcodegen::ArgumentByPosition(test_fn, 1), false);

  // arg0 + arg1 - arg0 * arg1
  Value* _1 = irb->CreateCall(float8pl, {arg0, arg1});
  Value* _2 = irb->CreateCall(float8mul, {arg0, arg1});
  Value* ret = irb->CreateCall(float8mi, {_1, _2});
  irb->CreateRet(codegen_utils->CreateDatumToCppTypeCast<double>(ret));

  //std::cout << "================================================================================" << std::endl;
  ////codegen_utils->module()->dump();
  //std::cout << "================================================================================" << std::endl;

  ////codegen_utils->InlineFunction(add_one);
  ////codegen_utils->InlineFunction(add_two);
  //codegen_utils->Optimize(gpcodegen::CodegenUtils::OptimizationLevel::kDefault, gpcodegen::CodegenUtils::SizeLevel::kNormal, false);

  //std::cout << "================================================================================" << std::endl;
  ////codegen_utils->module()->dump();
  //std::cout << "================================================================================" << std::endl;

  bool boo = codegen_utils->PrepareForExecution(gpcodegen::CodegenUtils::OptimizationLevel::kDefault,
                                     false);
  DoublesFn fn = codegen_utils->GetFunctionPointer<DoublesFn>("test_fn");
  std::cout<<fn(12, 10)<<std::endl;
  std::cout<<"END"<<std::endl;
  return 0;
}

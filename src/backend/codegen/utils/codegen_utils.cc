//---------------------------------------------------------------------------
//  Greenplum Database
//  Copyright 2016 Pivotal Software, Inc.
//
//  @filename:
//    codegen_utils.cc
//
//  @doc:
//    Object that manages runtime code generation for a single LLVM module.
//
//  @test:
//    Unittests in tests/code_generator_unittest.cc
//
//
//---------------------------------------------------------------------------

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <string>
#include <utility>
#include <iostream>

#include "codegen/utils/codegen_utils.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Analysis/TargetLibraryInfo.h"
#include "llvm/Analysis/TargetTransformInfo.h"
#include "llvm/ExecutionEngine/ExecutionEngine.h"
// DO NOT REMOVE: including the MCJIT.h header forces the MCJIT engine to be
// linked in when using static libraries.
#include "llvm/ExecutionEngine/MCJIT.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/GlobalValue.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Value.h"
#include "llvm/Support/CodeGen.h"
#include "llvm/Support/Host.h"
#include "llvm/Support/TargetRegistry.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetOptions.h"
#include "llvm/Transforms/IPO.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Value.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/Bitcode/ReaderWriter.h"

namespace llvm { class FunctionType; }

namespace gpcodegen {

namespace {

// Almost-trivial conversion from Codegen enum for optimization level to the
// LLVM equivalent.
inline llvm::CodeGenOpt::Level OptLevelCodegenToLLVM(
    const CodegenUtils::OptimizationLevel codegen_opt_level) {
  switch (codegen_opt_level) {
    case CodegenUtils::OptimizationLevel::kNone:
      return llvm::CodeGenOpt::None;
    case CodegenUtils::OptimizationLevel::kLess:
      return llvm::CodeGenOpt::Less;
    case CodegenUtils::OptimizationLevel::kDefault:
      return llvm::CodeGenOpt::Default;
    case CodegenUtils::OptimizationLevel::kAggressive:
      return llvm::CodeGenOpt::Aggressive;
    default: {
      std::fprintf(stderr,
                   "FATAL: Unrecognized CodegenUtils::OptimizationLevel\n");
      std::exit(1);
    }
  }
}

}  // namespace

constexpr char CodegenUtils::kExternalVariableNamePrefix[];
constexpr char CodegenUtils::kExternalFunctionNamePrefix[];


extern "C" {
char builtins_bc[1];
unsigned int builtins_bc_len = 1;

// TODO: Why does this need to be in extern "C" ?
std::unique_ptr<llvm::Module> builtins_module;
}

CodegenUtils::CodegenUtils(llvm::StringRef module_name)
    : context_(llvm::getGlobalContext()),
      ir_builder_(context_),
      module_(new llvm::Module(module_name, context_)),
      external_variable_counter_(0),
      external_function_counter_(0) {
  //CopyGlobalsFrom(builtins_module.get());
}

//std::unique_ptr<llvm::Module> parseLazyIRModule(const char* buffer,
//                                                llvm::SMDiagnostic &Err,
//                                                llvm::LLVMContext &Context) {
//  return getLazyIRModule(std::move(buffer)), Err, Context);
//}

bool LoadBuiltinsModule() {
  llvm::SMDiagnostic error;
  llvm::StringRef strref = { (const char*) builtins_bc, builtins_bc_len };
  std::unique_ptr<llvm::MemoryBuffer> buffer =
      llvm::MemoryBuffer::getMemBuffer(strref, "builtins_bc", false);

  // If successful, this moves Buffer. On error, this *does not* move Buffer.
  llvm::ErrorOr<std::unique_ptr<llvm::Module>> module_or_err =
    llvm::getLazyBitcodeModule(std::move(buffer), llvm::getGlobalContext());
  if (std::error_code ec = module_or_err.getError() ) {
    // TODO: There was an err. Handle it!
    return false;
  }

  builtins_module = std::move(module_or_err.get());

  //builtins_module = llvm::getLazyIRFileModule("/Users/shardikar/workspace/codegen/gpdb/src/backend/codegen/CMakeFiles/builtins.dir/builtins.bc",
  //                                            error, llvm::getGlobalContext());
  return (builtins_module.get() == nullptr);
}

bool CodegenUtils::InitializeGlobal() {
  // These LLVM initialization routines return true if there is no native
  // target available, false if initialization was OK. So, if all 3 return
  // false, then initialization is fine.
  return !llvm::InitializeNativeTarget()
         && !llvm::InitializeNativeTargetAsmPrinter()
         && !llvm::InitializeNativeTargetAsmParser();
}

bool CodegenUtils::Optimize(const OptimizationLevel generic_opt_level,
                             const SizeLevel size_level,
                             const bool optimize_for_host_cpu) {
  // This method's implementation is loosely based on the LLVM "opt"
  // command-line tool ('tools/opt/opt.cpp' in the LLVM source).

  if (module_.get() == nullptr) {
    // No Module to optimize (probably already compiled).
    return false;
  }

  // Get info about the target machine.
  llvm::Triple native_triple(llvm::sys::getProcessTriple());

  std::string error_string;
  const llvm::Target* native_target = llvm::TargetRegistry::lookupTarget(
      native_triple.getTriple(),
      error_string);
  if (native_target == nullptr) {
    // Couldn't find this platform in the TargetRegistry.
    return false;
  }

  std::unique_ptr<llvm::TargetMachine> native_target_machine(
      native_target->createTargetMachine(
          native_triple.getTriple(),
          optimize_for_host_cpu ? llvm::sys::getHostCPUName() : "",
          // CPU features string is empty, so feature flags will be
          // automatically inferred based on CPU model.
          "",
          llvm::TargetOptions(),
          llvm::Reloc::Default,
          llvm::CodeModel::Default,
          OptLevelCodegenToLLVM(generic_opt_level)));
  if (!native_target_machine) {
    // Couldn't create TargetMachine.
    return false;
  }

  // Set the module's triple and data layout to that of the target machine.
  module_->setTargetTriple(native_triple.getTriple());
  module_->setDataLayout(native_target_machine->createDataLayout());

  // Module-level and function-level pass managers to hold the transformation
  // passes that will be run over code.
  //
  // TODO(chasseur): The new pass-manager infrastructure is a work-in-progress
  // and doesn't yet have an easy way to automatically populate all the passes
  // from a given optimization level, so we use the legacy PassManager classes
  // instead. Once the new PassManagers are a bit more mature, we will probably
  // want to switch to using them.
  llvm::legacy::PassManager pass_manager;
  llvm::legacy::FunctionPassManager function_pass_manager(module_.get());

  // Add TargetLibraryInfo pass (provides information about built-in functions
  // and vectorizability for the target machine.).
  llvm::TargetLibraryInfoImpl target_library_info(native_triple);
  pass_manager.add(new llvm::TargetLibraryInfoWrapperPass(target_library_info));

  // Add internal analysis passes from the target machine.
  pass_manager.add(llvm::createTargetTransformInfoWrapperPass(
      native_target_machine->getTargetIRAnalysis()));
  if (generic_opt_level > OptimizationLevel::kNone) {
    function_pass_manager.add(llvm::createTargetTransformInfoWrapperPass(
        native_target_machine->getTargetIRAnalysis()));
  }

  // PassManagerBuilder handles automatically populating our pass managers with
  // the standard set of optimizations at each optimization level.
  llvm::PassManagerBuilder pass_builder;
  pass_builder.OptLevel = static_cast<unsigned>(generic_opt_level);
  pass_builder.SizeLevel = static_cast<unsigned>(size_level);

  // Add an inlining pass at -O1 and above.
  if (generic_opt_level > OptimizationLevel::kNone) {
    pass_builder.Inliner = llvm::createFunctionInliningPass(
        static_cast<unsigned>(generic_opt_level),
        static_cast<unsigned>(size_level));
  }

  // Add vectorization passes at -O2 and above.
  if (generic_opt_level > OptimizationLevel::kLess) {
    pass_builder.LoopVectorize = true;
    pass_builder.SLPVectorize = true;
  }

  // Have the PassManagerBuilder actually fill in the pass managers.
  pass_builder.populateFunctionPassManager(function_pass_manager);
  pass_builder.populateModulePassManager(pass_manager);

  // Run function-level passes.
  function_pass_manager.doInitialization();
  for (llvm::Function &function : *module_) {
    function_pass_manager.run(function);
  }
  function_pass_manager.doFinalization();

  // Run module-level passes.
  pass_manager.run(*module_);

  return true;
}

bool CodegenUtils::PrepareForExecution(const OptimizationLevel cpu_opt_level,
                                        const bool optimize_for_host_cpu) {
  if (engine_.get() != nullptr) {
    // This method was already called successfully.
    return false;
  }

  if (module_.get() == nullptr) {
    // No Module to compile.
    return false;
  }

  llvm::EngineBuilder builder(std::move(module_));
  builder.setEngineKind(llvm::EngineKind::JIT);
  builder.setOptLevel(OptLevelCodegenToLLVM(cpu_opt_level));
  if (optimize_for_host_cpu) {
    builder.setMCPU(llvm::sys::getHostCPUName());
  }

  engine_.reset(builder.create());
  if (engine_.get() == nullptr) {
    return false;
  }

  // Add auxiliary modules generated by companion tools to the ExecutionEngine.
  for (std::unique_ptr<llvm::Module>& auxiliary_module : auxiliary_modules_) {
    engine_->addModule(std::move(auxiliary_module));
  }

  // Map global variables (i.e. pointer constants) and registered external
  // functions to their actual locations in memory.
  //
  // Note that on OSX, C symbol names all have a leading underscore prepended to
  // them. We must replicate this when adding global mappings to the
  // ExecutionEngine so that names are properly resolved.
  for (const std::pair<const std::string,
                       const std::uint64_t>& external_global_variable
       : external_global_variables_) {
    engine_->addGlobalMapping(
#ifdef __APPLE__
        std::string(1, '_') + external_global_variable.first,
#else  // !__APPLE__
        external_global_variable.first,
#endif
        external_global_variable.second);
  }

  // Map registered external functions to their actual locations in memory.
  for (const std::pair<const std::uint64_t,
                       const std::string>& external_function
       : external_functions_) {
    engine_->addGlobalMapping(
#ifdef __APPLE__
        std::string(1, '_') + external_function.second,
#else  // !__APPLE__
        external_function.second,
#endif
        external_function.first);
  }

  return true;
}

void CodegenUtils::PrintUnderlyingModules(llvm::raw_ostream& out) {
  // Print the main module
  out << "==== MAIN MODULE ====" << "\n";
  out.flush();
  module()->print(out, nullptr);

  // Print auxiliary modules
  out << "==== AUXILIARY MODULES ====" << "\n";
  out.flush();
  for (std::unique_ptr<llvm::Module>& auxiliary_module : auxiliary_modules_) {
      auxiliary_module->print(out, nullptr);
  }
  out << "==== END MODULES ====" << "\n\n";
  out.flush();
}

llvm::GlobalVariable* CodegenUtils::AddExternalGlobalVariable(
    llvm::Type* type,
    const void* address) {
  external_global_variables_.emplace_back(
      GenerateExternalVariableName(),
      reinterpret_cast<std::uint64_t>(address));

  return new llvm::GlobalVariable(*module_,
                                  type,
                                  false,
                                  llvm::GlobalValue::ExternalLinkage,
                                  nullptr,
                                  external_global_variables_.back().first);
}

bool CodegenUtils::CopyGlobalsFrom(const llvm::Module* ext_module) {
  // Create an entry if it doesn't already exist
  llvm::ValueToValueMapTy& vmap = vmap_by_loaded_module[ext_module];

  assert(&ext_module->getContext() == context());
  for (llvm::Module::const_global_iterator i = ext_module->global_begin(), e = ext_module->global_end(); i!=e; ++i) {
    llvm::GlobalVariable *GV = new llvm::GlobalVariable(*module(),
                             i->getValueType(),
                             i->isConstant(), // isConstant
                             i->getLinkage(),
                             // TODO: Make sure this *actually* works, as in where are the Constants stores?
                             i->hasInitializer()? (llvm::Constant*) i->getInitializer() : nullptr,
                             i->getName().str());
    GV->copyAttributesFrom(&*i);
    vmap[&*i] = GV;
  }
  return true;
}

llvm::Function* CodegenUtils::InsertAlienFunction(llvm::Function* function, bool recursive) {
  assert(nullptr != function && nullptr != function->getParent());
  llvm::Function* new_func = nullptr;
  llvm::ValueToValueMapTy& vmap = vmap_by_loaded_module[function->getParent()];
  if (function->isDeclaration()) {
    new_func = module()->getFunction(function->getName());
    // If the modules already contains this func, we're good!
    if (!new_func) {
      // Fake clone this function if it's just a declaration
      new_func = llvm::Function::Create(
          function->getFunctionType(),
          function->getLinkage(),
          function->getName().str(),
          module());
    }
  } else {
    // We need to do this in case we lazy load the module
    function->materialize();
    llvm::ValueToValueMapTy _vmap;
    _vmap.insert(vmap.begin(), vmap.end());
    if (recursive) {
      for (llvm::inst_iterator
           I = llvm::inst_begin(function), E = llvm::inst_end(function); I != E; ++I) {
        llvm::CallSite call_site(&*I);
        if (call_site) {
          llvm::Function* called_func = call_site.getCalledFunction();
          assert(nullptr != called_func);
          if (called_func->getParent() != module()) {
            llvm::Function* called_func_clone = InsertAlienFunction(called_func, recursive);
            _vmap.insert(std::make_pair(called_func, called_func_clone));
          }
        }
      }
    }
    // Finally clone this function
    new_func = llvm::CloneFunction(function, _vmap, false);
    // Tell LLVM we'd like this inlined if possible
    new_func->addAttribute(llvm::AttributeSet::FunctionIndex,
                           llvm::Attribute::AttrKind::AlwaysInline);
    module()->getFunctionList().push_back(new_func);
  }
  return new_func;
}


void CodegenUtils::CheckFunctionType(
    const std::string& function_name,
    const llvm::FunctionType* expected_function_type) {
  // Look up function in the ExecutionEngine if PrepareForExecution() has
  // already been called, or in the Module if it has not.
  const llvm::Function* function
      = engine_ ? engine_->FindFunctionNamed(function_name.c_str())
                : module_->getFunction(function_name);

  if (function != nullptr) {
    assert(expected_function_type == function->getFunctionType());
  }
}

std::string CodegenUtils::GenerateExternalVariableName() {
  char print_buffer[sizeof(kExternalVariableNamePrefix)
                    + (sizeof(unsigned) << 1) + 1] = {};
  int chars_printed = std::snprintf(print_buffer,
                                    sizeof(print_buffer),
                                    "%s%X",
                                    kExternalVariableNamePrefix,
                                    ++external_variable_counter_);
  assert(static_cast<std::size_t>(chars_printed) < sizeof(print_buffer));
  return std::string(print_buffer);
}

std::string CodegenUtils::GenerateExternalFunctionName() {
  // Prefix for generated names is only 10 chars. Up to 16 chars (including
  // null-terminator) are typically stored inline for C++ standard library
  // implementations that use the short-string optimization for std::string
  // (e.g. LLVM libc++ and GNU libstdc++ from GCC 5 or higher). This gives us
  // 5 chars worth of space that we can use for unique names without strings
  // ever having to allocate external storage on the heap. Since we number using
  // hexadecimal characters, this means we can register up to 1M (2^20)
  // external functions without ever touching external memory for strings. That
  // ought to be enough for anybody. ;)

  char print_buffer[sizeof(kExternalFunctionNamePrefix)
                    + (sizeof(unsigned) << 1) + 1] = {};
  int chars_printed = std::snprintf(print_buffer,
                                    sizeof(print_buffer),
                                    "%s%X",
                                    kExternalFunctionNamePrefix,
                                    ++external_function_counter_);
  assert(static_cast<std::size_t>(chars_printed) < sizeof(print_buffer));
  return std::string(print_buffer);
}

llvm::Value* CodegenUtils::GetPointerToMemberImpl(
    llvm::Value* base_ptr,
    llvm::Type* cast_type,
    const std::size_t cumulative_offset) {
  // Sanity checks: base_ptr must be non-NULL.
  assert(base_ptr != nullptr);

  // Either this is a trivial invocation of GetPointerToMember() with zero
  // pointer-to-member arguments, or we are actually doing something meaningful
  // and base_ptr should be a pointer-to-struct represented as i8*.
  assert(((cast_type == nullptr) && (cumulative_offset == 0u))
         || (base_ptr->getType()->isPointerTy()
             && base_ptr->getType()->getPointerElementType()->isIntegerTy(8)));

  llvm::Value* offset_pointer = base_ptr;
  if (cumulative_offset != 0u) {
    // Use the GEP (get element pointer) instruction to do the address
    // computation in LLVM.
    offset_pointer = ir_builder_.CreateInBoundsGEP(
        GetType<char>(),  // Use LLVM i8 type as the basis for array indexing.
        base_ptr,
        GetConstant(cumulative_offset));
  }

  if (cast_type == nullptr) {
    return offset_pointer;
  } else {
    // Cast the pointer to the appropriate type.
    return ir_builder_.CreateBitCast(offset_pointer, cast_type);
  }
}

llvm::Value* CodegenUtils::CreateIntrinsicInstrCall(
    llvm::Intrinsic::ID Id,
    llvm::ArrayRef<llvm::Type*> Tys,
    llvm::Value* arg0,
    llvm::Value* arg1) {
  llvm::Function* llvm_intr_func = llvm::Intrinsic::getDeclaration(module(),
                                                                   Id,
                                                                   Tys);
  return ir_builder()->CreateCall(llvm_intr_func, {arg0, arg1});
}

}  // namespace gpcodegen

// EOF

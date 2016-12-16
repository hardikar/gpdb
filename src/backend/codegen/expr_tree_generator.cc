//---------------------------------------------------------------------------
//  Greenplum Database
//  Copyright (C) 2016 Pivotal Software, Inc.
//
//  @filename:
//    expr_tree_generator.cc
//
//  @doc:
//    Base class for expression tree to generate code
//
//---------------------------------------------------------------------------
#include <cassert>
#include <memory>

#include "codegen/const_expr_tree_generator.h"
#include "codegen/expr_tree_generator.h"
#include "codegen/op_expr_tree_generator.h"
#include "codegen/var_expr_tree_generator.h"

extern "C" {
#include "postgres.h"  // NOLINT(build/include)
#include "nodes/execnodes.h"
#include "utils/elog.h"
#include "nodes/nodes.h"
}

using namespace gpcodegen;


class BoolExprTreeGenerator : public ExprTreeGenerator {
 public:
  static bool VerifyAndCreateExprTree(
        const ExprState* expr_state,
        ExprTreeGeneratorInfo* gen_info,
        std::unique_ptr<ExprTreeGenerator>* expr_tree) {
    bool supported_tree = true;
    std::vector<std::unique_ptr<ExprTreeGenerator>> expr_tree_arguments;

    BoolExpr* bool_expr = reinterpret_cast<BoolExpr*>(expr_state->expr);
    expr_tree->reset(nullptr);

    ListCell* larg = NULL;
    BoolExprState* bool_state = (BoolExprState*)(expr_state);
    foreach(larg, bool_state->args) {
      ExprState *argstate = (ExprState*) lfirst(larg);

      std::unique_ptr<ExprTreeGenerator> arg(nullptr);
      supported_tree &= ExprTreeGenerator::VerifyAndCreateExprTree(argstate,
                                                                   gen_info,
                                                                   &arg);
      expr_tree_arguments.push_back(std::move(arg));
    }
    expr_tree->reset(new BoolExprTreeGenerator(expr_state,
                                               std::move(expr_tree_arguments)));
    return supported_tree;
  }

  bool GenerateCode(gpcodegen::GpCodegenUtils* codegen_utils,
                    const ExprTreeGeneratorInfo& gen_info,
                    llvm::Value** llvm_out_value,
                    llvm::Value* const llvm_isnull_ptr) {
    auto irb = codegen_utils->ir_builder();

    std::vector<llvm::Value*> llvm_arguments;
    std::vector<llvm::Value*> llvm_arguments_isNull;

    for (auto& arg : arguments_) {
      llvm::Value* llvm_arg_isnull_ptr = irb->CreateAlloca(
          codegen_utils->GetType<bool>(), nullptr, "isNull");
      llvm::Value* llvm_arg = nullptr;
      irb->CreateStore(codegen_utils->GetConstant<bool>(false),
                       llvm_arg_isnull_ptr);
      arg->GenerateCode(codegen_utils,
                       gen_info,
                       &llvm_arg,
                       llvm_arg_isnull_ptr);
      llvm_arguments.push_back(llvm_arg);
      llvm_arguments_isNull.push_back(irb->CreateLoad(llvm_arg_isnull_ptr));
    }

    llvm::Value* llvm_agg = nullptr;
    for (auto& llvm_arg : llvm_arguments) {
      if (nullptr == llvm_agg ) {
        llvm_agg = llvm_arg;
        continue;
      }
      llvm_agg = irb->CreateAnd(llvm_agg, llvm_arg);
    }
    *llvm_out_value = llvm_agg;

    return true;
  }

 protected:
  BoolExprTreeGenerator(
      const ExprState* expr_state,
      std::vector<std::unique_ptr<ExprTreeGenerator>>&& arguments)
    : ExprTreeGenerator(expr_state, ExprTreeNodeType::kBool),
      arguments_(std::move(arguments)){

  }

 private:
  std::vector<std::unique_ptr<ExprTreeGenerator>> arguments_;
};

bool ExprTreeGenerator::VerifyAndCreateExprTree(
    const ExprState* expr_state,
    ExprTreeGeneratorInfo* gen_info,
    std::unique_ptr<ExprTreeGenerator>* expr_tree) {
  assert(nullptr != expr_state &&
         nullptr != expr_state->expr &&
         nullptr != expr_tree);

  if (!(IsA(expr_state, FuncExprState) ||
      IsA(expr_state, ExprState) ||
      IsA(expr_state, BoolExprState))) {
    elog(DEBUG1, "Input expression state type (%d) is not supported",
         expr_state->type);
    return false;
  }
  expr_tree->reset(nullptr);
  bool supported_expr_tree = false;

  switch (nodeTag(expr_state->expr)) {
    case T_OpExpr: {
      supported_expr_tree = OpExprTreeGenerator::VerifyAndCreateExprTree(
          expr_state, gen_info, expr_tree);
      break;
    }
    case T_Var: {
      supported_expr_tree = VarExprTreeGenerator::VerifyAndCreateExprTree(
          expr_state, gen_info, expr_tree);
      break;
    }
    case T_Const: {
      supported_expr_tree = ConstExprTreeGenerator::VerifyAndCreateExprTree(
          expr_state, gen_info, expr_tree);
      break;
    }
    case T_BoolExpr: {
      supported_expr_tree = BoolExprTreeGenerator::VerifyAndCreateExprTree(
          expr_state, gen_info, expr_tree);
      break;
    }
    default : {
      supported_expr_tree = false;
      elog(DEBUG1, "Unsupported expression tree %d found",
           nodeTag(expr_state->expr));
    }
  }
  assert((!supported_expr_tree && nullptr == expr_tree->get()) ||
         (supported_expr_tree && nullptr != expr_tree->get()));
  return supported_expr_tree;
}

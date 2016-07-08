//---------------------------------------------------------------------------
//  Greenplum Database
//  Copyright (C) 2016 Pivotal Software, Inc.
//
//  @filename:
//    expr_tree_generator.h
//
//  @doc:
//    Base class for expression tree to generate code
//
//---------------------------------------------------------------------------
#ifndef GPCODEGEN_EXPR_TREE_GENERATOR_H_  // NOLINT(build/header_guard)
#define GPCODEGEN_EXPR_TREE_GENERATOR_H_

#include <string>
#include <vector>
#include <memory>
#include <utility>
#include <unordered_map>

#include "codegen/utils/codegen_utils.h"
#include "codegen/codegen_wrapper.h"

#include "llvm/IR/Value.h"

typedef struct ExprContext ExprContext;
typedef struct ExprState ExprState;
typedef struct Expr Expr;
typedef struct OpExpr OpExpr;
typedef struct Var Var;
typedef struct Const Const;

namespace gpcodegen {

/** \addtogroup gpcodegen
 *  @{
 */

enum class ExprTreeNodeType {
  kConst = 0,
  kVar = 1,
  kOperator = 2
};

struct ExprTreeGeneratorInfo {
  ExprContext* econtext;

  llvm::Function* llvm_main_func;
  llvm::BasicBlock* llvm_error_block;

  llvm::Function* llvm_slot_getattr_func;

  // Members that will be updated by ExprTreeGenerator
  int16_t max_attr;

  ExprTreeGeneratorInfo(
    ExprContext* econtext,
    llvm::Function* llvm_main_func,
    llvm::BasicBlock* llvm_error_block,
    llvm::Function* llvm_slot_getattr_func,
    int16_t max_attr) :
      econtext(econtext),
      llvm_main_func(llvm_main_func),
      llvm_error_block(llvm_error_block),
      llvm_slot_getattr_func(llvm_slot_getattr_func),
      max_attr(max_attr) {
  }
};

class ExprTreeGenerator {
 public:
  virtual ~ExprTreeGenerator() = default;

  /**
   * @brief Verify if we support the given expression tree, and create an
   * 		instance of ExprTreeGenerator if supported.
   *
   * @param expr_state  Expression state from expression tree.
   * @param econtext    Respective expression context.
   * @param expr_tree   Hold the new instance of expression tree class.
   *
   * @return true when it can codegen otherwise it return false.
   **/
  static bool VerifyAndCreateExprTree(
      const ExprState* expr_state,
      ExprTreeGeneratorInfo* gen_info,
      std::unique_ptr<ExprTreeGenerator>* expr_tree);

  /**
   * @brief Generate the code for given expression.
   *
   * @param codegen_utils   Utility to easy code generation.
   * @param econtext        Respective expression context.
   * @param llvm_main_func  Current function for which we are generating code
   * @param llvm_isnull_arg Set to true if current expr is null
   * @param llvm_out_value  Store the expression results
   *
   * @return true when it generated successfully otherwise it return false.
   **/
  virtual bool GenerateCode(gpcodegen::CodegenUtils* codegen_utils,
                            const ExprTreeGeneratorInfo& gen_info,
                            llvm::Value* llvm_isnull_ptr,
                            llvm::Value** value) = 0;


  /**
   * @return Expression state
   **/
  const ExprState* expr_state() { return expr_state_; }

 protected:
  /**
   * @brief Constructor.
   *
   * @param expr_state Expression state
   * @param node_type   Type of the ExprTreeGenerator
   **/
  ExprTreeGenerator(const ExprState* expr_state,
                    ExprTreeNodeType node_type) :
                      expr_state_(expr_state) {}

 private:
  const ExprState* expr_state_;
  DISALLOW_COPY_AND_ASSIGN(ExprTreeGenerator);
};


/** @} */
}  // namespace gpcodegen

#endif  // GPCODEGEN_EXPR_TREE_GENERATOR_H_

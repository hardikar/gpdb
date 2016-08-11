#include <sstream>
#include <iostream>
#include <fstream>
#include <string>

#include "clang/AST/AST.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/ASTConsumers.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include "llvm/Support/raw_ostream.h"
#include "clang/Rewrite/Frontend/Rewriters.h"
#include "clang/Rewrite/Frontend/FrontendActions.h"
#include "clang/Lex/Preprocessor.h"

#define NUM_PARAMS  5

void dumpString(std::string& content, std::string filename) {
    std::ofstream ofs(filename);
    ofs << content;
    ofs.close();
}

class MyASTVisitor : public clang::RecursiveASTVisitor<MyASTVisitor> {
public:
  MyASTVisitor(clang::Rewriter &R) : TheRewriter(R) {}

  bool VisitStmt(clang::Stmt *S) {
    if (clang::ArraySubscriptExpr *array_sub = clang::dyn_cast<clang::ArraySubscriptExpr>(S) ){

      //std::cerr << "ArraySub" << std::endl;

      clang::Expr* base = array_sub->getBase()->IgnoreCasts();
      clang::Expr* idx = array_sub->getIdx();

      int idx_value = -1;

      if (clang::IntegerLiteral* integer = clang::dyn_cast<clang::IntegerLiteral>(idx)) {
        idx_value = integer->getValue().getZExtValue();
        //std::cerr << idx_value << std::endl;
      }

      assert(clang::isa<clang::MemberExpr>(base));

      if (clang::MemberExpr* member_expr = clang::dyn_cast<clang::MemberExpr>(base)) {

        clang::Expr* member_expr_base = member_expr->getBase()->IgnoreCasts();
        clang::ValueDecl* member_expr_member_decl = member_expr->getMemberDecl();

        //std::cerr << " member_expr_member_decl : " << member_expr_member_decl->getNameAsString() << std::endl;

        if (clang::DeclRefExpr* decl_ref_expr = clang::dyn_cast<clang::DeclRefExpr>(member_expr_base)){
          //std::cerr << " member_expr_base : " << decl_ref_expr->getDecl()->getNameAsString() << std::endl;

          std::string new_parm_name = "_" + std::to_string(idx_value);
          std::string new_parm_decl = "Datum "  "_" + std::to_string(idx_value);

          TheRewriter.ReplaceText(clang::SourceRange(array_sub->getLocStart(), array_sub->getLocEnd()),
                                  new_parm_name);
          parms_[idx_value] = new_parm_decl;
        }
      }
    }
    return true;
  }

  bool VisitDecl(clang::Decl *D) {
    if (clang::ParmVarDecl *parm_decl = clang::dyn_cast<clang::ParmVarDecl>(D) ){
      //std::cerr << "Found parm : " << parm_decl->getNameAsString() << std::endl;
      // Save the parameter declaration
      parm_decl_ = parm_decl;
    }
    return true;
  }

  bool Finalize() {
    std::stringstream ss;
    for(size_t i = 0; i < NUM_PARAMS; ++i)
    {
      if (!parms_[i].empty()) {
        if(i != 0)
          ss << ", ";
        ss << parms_[i];
      }
    }

    TheRewriter.ReplaceText(parm_decl_->getSourceRange(), ss.str());
    std::cerr << "Replace text" << ss.str() << std::endl;
  }
private:
  std::string parms_[NUM_PARAMS];
  clang::ParmVarDecl *parm_decl_ = nullptr;
  clang::Rewriter &TheRewriter;
};

class MyASTConsumer : public clang::ASTConsumer{
 public:
  MyASTConsumer(clang::Rewriter &R): R(R) {}

  bool HandleTopLevelDecl(clang::DeclGroupRef DR) override {
    //std::cerr << "Handling each declgroup" << std::endl;
    for (clang::DeclGroupRef::iterator b = DR.begin(), e = DR.end(); b != e; ++b) {
       // Traverse the declaration using our AST visitor.
      if (clang::FunctionDecl *func_decl = clang::dyn_cast<clang::FunctionDecl>(*b) ){
        if (func_decl->hasBody() &&
            func_decl->getNumParams() == 1 &&
            func_decl->getParamDecl(0)->getType().getAsString() == "FunctionCallInfo") {
          std::cerr << "Converting function " << func_decl->getNameAsString() << std::endl;
          MyASTVisitor Visitor(R);
          // TODO : A FunctionDecl is only visited once, so we need to iterate
          // through all the declarations and update them
          Visitor.TraverseDecl(*b);
          Visitor.Finalize();
        }
      }
       //(*b)->dump();
    }
    return true;
  }
 private:
  clang::Rewriter &R;
};

class MyFrontendAction : public clang::ASTFrontendAction {

 public:
  std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(clang::CompilerInstance &CI,
                                                  StringRef file) override {
     //std::cerr << "** Creating AST consumer for: " << file << "\n";
     TheRewriter.setSourceMgr(CI.getSourceManager(), CI.getLangOpts());
     return llvm::make_unique<MyASTConsumer>(TheRewriter);
   }

  void EndSourceFileAction() override {
    clang::SourceManager &SM = TheRewriter.getSourceMgr();
    //llvm::errs() << "** EndSourceFileAction for: "
    //             << SM.getFileEntryForID(SM.getMainFileID())->getName() << "\n";

    // Now emit the rewritten buffer.
    std::string str;
    llvm::raw_string_ostream os(str);
    TheRewriter.getEditBuffer(SM.getMainFileID()).write(os);

    std::cerr << "============================================================" << std::endl;
    dumpString(os.str(), "int_final.c");
   }

 private:
  clang::Rewriter TheRewriter;
};


class MyRewriteMacrosAction : public clang::RewriteMacrosAction {
 public:
  MyRewriteMacrosAction(std::string &str) : str_(str) {
  }
  void ExecuteAction() override {
    clang::CompilerInstance &CI = getCompilerInstance();
    llvm::raw_string_ostream OS(str_);
    clang::RewriteMacrosInInput(CI.getPreprocessor(), &OS);
  }
  std::string& str_;
};



int main(int argc, char **argv) {
  if (argc > 1) {
    std::ifstream ifs(argv[1]);
    std::string content((std::istreambuf_iterator<char>(ifs) ),
                  (std::istreambuf_iterator<char>()));
    //std::cerr << content;
    std::cerr << "START ============================================================" << std::endl;

    std::vector<std::string> args = {
        "-xc",
        "-I/usr/include",
        "-I/usr/local/include",
        "-I/Users/shardikar/workspace/codegen/gpdb/src/include",
        "-I/Users/shardikar/workspace/codegen/gpdb/src/backend/codegen/include",
        "-I/opt/llvm-3.7.1-debug/include"
    };

    clang::tooling::runToolOnCodeWithArgs(new MyFrontendAction, content, args);
    std::cerr << "DONE ACTION ============================================================" << std::endl;
  }
}

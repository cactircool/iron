#ifndef ABED3B35_167F_4041_A3C5_35C76E569834
#define ABED3B35_167F_4041_A3C5_35C76E569834

#include "Expr.hpp"

namespace iron {

    struct UnExpr : public Expr {
        std::string oper;
        bool right;
        Expr *arg;

        UnExpr(Node *node) : Expr(node) {
            right = node->value.find("left") != 0;
            oper = node->children[0]->value;
            arg = dynamic_cast<Expr *>(ASTNode::parse(node->children[1]));
        }
        UnExpr(std::string oper, Expr *arg, bool right) : Expr(nullptr), oper(oper), right(right), arg(arg) {}

        std::string print() const override {
            if (!right) return oper + " " + arg->print();
            return arg->print() + " " + oper;
        }

        llvm::Value *codegen(LLVMInfo info) override {
            return nullptr;
        }

        llvm::Type *getType(LLVMInfo info) const override {
            return nullptr;
        }
    };

}

#endif /* ABED3B35_167F_4041_A3C5_35C76E569834 */

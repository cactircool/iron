#ifndef F118C08C_9CFE_42B0_A086_98E028E2899B
#define F118C08C_9CFE_42B0_A086_98E028E2899B

#include "ASTNode.hpp"

namespace iron {

    struct TypeCall;

    struct Expr : public ASTNode {
        TypeCall *exprType;

        Expr(Node *node) : ASTNode(node) {}

        void setExprType(TypeCall *exprType) {
            this->exprType = exprType;
        }

        llvm::Type *getType(LLVMInfo info) const override;
    };

}

#endif /* F118C08C_9CFE_42B0_A086_98E028E2899B */

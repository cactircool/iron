#ifndef C06E873E_6E74_42DF_A3BE_62722237E776
#define C06E873E_6E74_42DF_A3BE_62722237E776

#include "Expr.hpp"

namespace iron {

    struct Return : public Expr {
        Expr *arg;

        Return(Node *node) : Expr(node) {
            arg = dynamic_cast<Expr *>(ASTNode::parse(node->children[0]));
            setExprType(arg->exprType);
        }
        Return(Expr *arg, Node *node) : Expr(node) {
            this->arg = arg;
            setExprType(arg->exprType);
        }

        std::string print() const override {
            return "ret " + arg->print();
        }

        llvm::Value *codegen(LLVMInfo info) override {
            llvm::Type *retType = info.builder->GetInsertBlock()->getParent()->getReturnType();
            auto *var = info.builder->CreateAlloca(retType, nullptr);
            // arg->codegen(info)
            if (retType->isIntegerTy()) {
                auto *cast = info.builder->CreateIntCast(arg->codegen(info), var->getAllocatedType(), true);
                info.builder->CreateStore(cast, var);
            } else {
                auto *cast = info.builder->CreateCast(llvm::Instruction::CastOps::ZExt, arg->codegen(info), var->getAllocatedType());
                info.builder->CreateStore(cast, var, false);
            }
            return info.builder->CreateRet(info.builder->CreateLoad(var->getAllocatedType(), var));
        }

        llvm::Type *getType(LLVMInfo info) const override {
            return arg->getType(info);
        }

        ASTNode * rewrite(std::map<std::string, TypeCall *> gens, std::map<std::string, Decl *> vars) override {
            return new Return(dynamic_cast<Expr *>(arg->rewrite(gens, vars)), nd);
        }
    };

} 

#endif /* C06E873E_6E74_42DF_A3BE_62722237E776 */

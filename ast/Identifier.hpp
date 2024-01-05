#ifndef CA968778_26E4_4C96_85D1_9FD058D16AC5
#define CA968778_26E4_4C96_85D1_9FD058D16AC5

#include "Decl.hpp"
#include "Expr.hpp"
#include "Variable.hpp"
#include "llvm/IR/Instructions.h"

namespace iron {

    struct Identifier : public Expr {
        Decl *id;
        Identifier(Node *node) : Expr(node) {
            if (node->children.size() == 1) {
                id = Decl::getDecl(node->children[0]->value);
            } else {
                std::string stack = "";
                for (auto *child : node->children) {
                    stack += (!stack.empty() ? "::" : "") + (find("datatype", child) ? child->children[0]->children[0]->value : child->children[0]->value);
                    if (auto *lab = find("label", child)) {
                        stack += "[" + lab->children[0]->value + "]";
                    }
                }

                id = Decl::getDecl(stack);
            }

            setExprType(dynamic_cast<Variable *>(id)->type);
        }
        Identifier(Decl *id) : Expr(nullptr), id(id) {}

        std::string print() const override {
            return id->name;
        }

        llvm::Value *codegen(LLVMInfo info) override {
            if (auto *var = dynamic_cast<Variable *>(id)) {
                return info.builder->CreateLoad(var->alloc->getAllocatedType(), var->alloc, var->isVolatile);
            }
            // function references
            return nullptr;
        }

        llvm::Type *getType(LLVMInfo info) const override {
            if (auto *var = dynamic_cast<Variable *>(id)) {
                llvm::AllocaInst *inst = llvm::dyn_cast<llvm::AllocaInst>(id->codegen(info));
                return inst->getAllocatedType();
            }
            return nullptr;
        }
    };

}

#endif /* CA968778_26E4_4C96_85D1_9FD058D16AC5 */

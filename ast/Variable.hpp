#ifndef BDB4ED70_03F4_471C_800F_2FB8383C7C8E
#define BDB4ED70_03F4_471C_800F_2FB8383C7C8E

#include "TypeCall.hpp"
#include "Expr.hpp"
#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/Support/Alignment.h"
#include "llvm/Support/AtomicOrdering.h"
#include "llvm/Support/raw_ostream.h"
#include "ASTNode.hpp"

namespace iron {

    struct Variable : public Decl {
        bool isGlobal = false;
        bool isVolatile;
        bool isConst;
        TypeCall *type;
        Expr *value;

        static std::map<std::string, llvm::AllocaInst *> allocs;
        llvm::AllocaInst *alloc;

        Variable(Node *node) : Decl(node) {
            if (auto *mods = find("mods", node)) {
                for (auto *child : mods->children) {
                    if (child->value == "vol")
                        isVolatile = true;
                    if (child->value == "const")
                        isConst = true;
                }
            }

            if (auto *name = find("name", node)) 
                setName(Decl::printId(name->children[0]), name->children.size() > 1);

            if (auto *t = find("type", node)) {
                type = new TypeCall(t->children[0]);
            }

            if (auto *v = find("value", node))
                value = dynamic_cast<Expr *>(ASTNode::parse(v->children[0]));
        }

        Variable(std::string name) : Decl(nullptr) {
            setName(name);
        }

        Variable(TypeCall *tc, Node *node) : Decl(node), type(tc) {
            setName(Decl::genName());
        }

        Variable(std::string name, TypeCall *tc, Node *node) : Decl(node), type(tc) {
            setName(name);
        }

        std::string print() const override {
            if (!value)
                return "let " + name + ":" + type->print();
            return "let " + name + ":" + type->print() + " = " + value->print();
        }

        llvm::Value *codegen(LLVMInfo info) override {
            if (allocs.find(name) != allocs.end())
                return allocs[name];

            llvm::Value *val = value ? value->codegen(info) : nullptr;
            llvm::Type *t = getType(info);

            if (isGlobal) {
                auto *glob = new llvm::GlobalVariable(*info._module, t, isConst, llvm::GlobalVariable::LinkageTypes::ExternalLinkage, nullptr, name);
                
                auto *ft = llvm::FunctionType::get(llvm::Type::getVoidTy(*info.context), false);
                llvm::Function *globInit = llvm::Function::Create(ft, llvm::Function::LinkageTypes::ExternalLinkage, Decl::genName(), *info._module);
                
                llvm::BasicBlock *block = llvm::BasicBlock::Create(*info.context, "entry", globInit);
                llvm::IRBuilder<> builder(block);
                if (value) builder.CreateAlignedStore(val, glob, llvm::Align());
                builder.CreateRetVoid();
                
                return glob;
            }


            auto *a = info.builder->CreateAlloca(t, nullptr, name);
            if (value) info.builder->CreateStore(val, a);
            allocs[name] = a;
            alloc = a;
            return a;
        }

        llvm::Type *getType(LLVMInfo info) const override {
            return type->getType(info);
        }

        ASTNode *rewrite(std::map<std::string, TypeCall *> gens, std::map<std::string, Decl *> vars) override {
            Variable *copy = new Variable(Decl::genName());
            copy->isConst = isConst;
            copy->isGlobal = isGlobal;
            copy->isVolatile = isVolatile;
            vars[name] = copy;
            copy->type = dynamic_cast<TypeCall *>(type->rewrite(gens, vars));
            if (value)
                copy->value = dynamic_cast<Expr *>(value->rewrite(gens, vars));

            // TypeCall is done, so all that's left is nothing until you make the BinExpr and UnExpr classes

            return copy;
        }
    };

    std::map<std::string, llvm::AllocaInst *> Variable::allocs = {};

}

#endif /* BDB4ED70_03F4_471C_800F_2FB8383C7C8E */

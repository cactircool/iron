#ifndef CCDE879B_5ED1_41F8_8740_A1960C76E8C8
#define CCDE879B_5ED1_41F8_8740_A1960C76E8C8

#include "Expr.hpp"
#include "TypeCall.hpp"
#include "Identifier.hpp"
#include "Variable.hpp"
#include "OperOverload.hpp"
#include "CallExpr.hpp"
#include "UnExpr.hpp"

namespace iron {

    struct BinExpr : public Expr {
        std::string oper;
        Expr *left;
        Expr *right;

        BinExpr(Node *node) : Expr(node) {
            node = node->children[0];
            oper = node->value;
            left = dynamic_cast<Expr *>(ASTNode::parse(node->children[0]));
            if (oper == ":")
                right = new TypeCall(node->children[1]);
            else {
                std::string c = Decl::scope;
                if (oper == ".")
                    Decl::scope = left->exprType->type->name;
                right = dynamic_cast<Expr *>(ASTNode::parse(node->children[1]));
                Decl::scope = c;
            }
        }

        std::string print() const override {
            return left->print() + " " + oper + " " + right->print();
        }

        llvm::Value *instr(LLVMInfo info, std::function<llvm::Value*(llvm::Value*, llvm::Value*)> a, std::function<llvm::Value*(llvm::Value*, llvm::Value*)> b) { \
            int lbits = std::stoi(left->exprType->type->name.substr(1));
            int rbits = std::stoi(right->exprType->type->name.substr(1));
            int bits = std::max(lbits, rbits) + 1;
            if (left->exprType->smatches(new TypeCall("i32")) && right->exprType->smatches(new TypeCall("i32"))) {
                if (lbits == rbits)
                    return a(left->codegen(info), right->codegen(info));
                
                auto *x = info.builder->CreateAlloca(llvm::IntegerType::getIntNTy(*info.context, bits * 2 + 1), nullptr, Decl::genName());
                auto *y = info.builder->CreateAlloca(llvm::IntegerType::getIntNTy(*info.context, bits * 2 + 1), nullptr, Decl::genName());
                
                bool x_vol = false;
                bool y_vol = false;

                if (auto *id = dynamic_cast<Identifier *>(left)) 
                    if (auto *var = dynamic_cast<Variable *>(id->id)) 
                        x_vol = var->isVolatile;
                info.builder->CreateStore(left->codegen(info), x, x_vol); 

                if (auto *id = dynamic_cast<Identifier *>(right)) 
                    if (auto *var = dynamic_cast<Variable *>(id->id)) 
                        y_vol = var->isVolatile;
                info.builder->CreateStore(right->codegen(info), y, y_vol);

                return a(info.builder->CreateLoad(x->getAllocatedType(), x, x_vol), info.builder->CreateLoad(y->getAllocatedType(), y, y_vol));
            } 
            else if (left->exprType->smatches(new TypeCall("f32")) && right->exprType->smatches(new TypeCall("f32"))) {
                if (lbits == rbits)
                    return b(left->codegen(info), right->codegen(info));

                bits--;
                bits *= 2;

                llvm::Type *ft;
                if (bits == 16)
                    ft = llvm::Type::getHalfTy(*info.context);
                else if (bits == 32)
                    ft = llvm::Type::getFloatTy(*info.context);
                else if (bits == 64)
                    ft = llvm::Type::getDoubleTy(*info.context);
                else if (bits == 128)
                    ft = llvm::Type::getFP128Ty(*info.context);
                else
                    ft = llvm::Type::getFloatTy(*info.context);
                
                auto *x = info.builder->CreateAlloca(ft, nullptr, Decl::genName());
                auto *y = info.builder->CreateAlloca(ft, nullptr, Decl::genName());
                
                bool x_vol = false;
                bool y_vol = false;

                if (auto *id = dynamic_cast<Identifier *>(left)) 
                    if (auto *var = dynamic_cast<Variable *>(id->id)) 
                        x_vol = var->isVolatile;
                info.builder->CreateStore(left->codegen(info), x, x_vol); 

                if (auto *id = dynamic_cast<Identifier *>(right)) 
                    if (auto *var = dynamic_cast<Variable *>(id->id)) 
                        y_vol = var->isVolatile;
                info.builder->CreateStore(right->codegen(info), y, y_vol);

                return b(info.builder->CreateLoad(x->getAllocatedType(), x, x_vol), info.builder->CreateLoad(y->getAllocatedType(), y, y_vol));
            }
            else if (left->exprType->smatches(new TypeCall("i32")) && right->exprType->smatches(new TypeCall("f32"))) {
                auto *l = left->codegen(info);
                auto *r = right->codegen(info);
                llvm::Value *castInst;
                if (bits == 16) 
                    castInst = info.builder->CreateSIToFP(l, llvm::Type::getFloatTy(*info.context));
                else if (bits == 32) 
                    castInst = info.builder->CreateSIToFP(l, llvm::Type::getDoubleTy(*info.context));
                else if (bits == 64) 
                    castInst = info.builder->CreateSIToFP(l, llvm::Type::getFP128Ty(*info.context));
                else if (bits == 128) 
                    castInst = info.builder->CreateSIToFP(l, llvm::Type::getFP128Ty(*info.context)); 
                else 
                    castInst = info.builder->CreateSIToFP(l, llvm::Type::getFloatTy(*info.context)); 
                return b(castInst, r);
            } 
            else if (right->exprType->smatches(new TypeCall("i32")) && left->exprType->smatches(new TypeCall("f32"))) {
                auto *l = left->codegen(info); 
                auto *r = right->codegen(info); 
                llvm::Value *castInst;
                if (bits == 16) 
                    castInst = info.builder->CreateSIToFP(r, llvm::Type::getFloatTy(*info.context));
                else if (bits == 32) 
                    castInst = info.builder->CreateSIToFP(r, llvm::Type::getDoubleTy(*info.context));
                else if (bits == 64) 
                    castInst = info.builder->CreateSIToFP(r, llvm::Type::getFP128Ty(*info.context));
                else if (bits == 128) 
                    castInst = info.builder->CreateSIToFP(r, llvm::Type::getFP128Ty(*info.context)); 
                else 
                    castInst = info.builder->CreateSIToFP(r, llvm::Type::getFloatTy(*info.context)); 
                return b(l, castInst);
            } 
            else if (left->exprType->smatches(new TypeCall("i32")) && right->exprType->smatches(new TypeCall("bf"))) {
                auto *val = info.builder->CreateSIToFP(left->codegen(info), llvm::Type::getBFloatTy(*info.context)); 
                return b(val, right->codegen(info));
            } 
            else if (right->exprType->smatches(new TypeCall("i32")) && left->exprType->smatches(new TypeCall("bf"))) {
                auto *val = info.builder->CreateSIToFP(right->codegen(info), llvm::Type::getBFloatTy(*info.context)); 
                return b(left->codegen(info), val);
            } 
            return nullptr; 
        }

        llvm::Value *codegen(LLVMInfo info) override {
            #define inst(aa, bb) return instr(info, [&](auto *a, auto *b) { return info.builder->aa(a, b); }, [&](auto *a, auto *b) { return info.builder->bb(a, b); });
            #define sinst(a) return info.builder->a(left->codegen(info), right->codegen(info));

            if (left->exprType->isPrimitive() && ((dynamic_cast<TypeCall *>(right) && dynamic_cast<TypeCall *>(right)->isPrimitive()) || right->exprType->isPrimitive())) {
                if (oper == "+")
                    inst(CreateAdd, CreateFAdd)
                if (oper == "-") 
                    inst(CreateSub, CreateFSub)
                if (oper == "*") 
                    inst(CreateMul, CreateFMul)
                if (oper == "/") 
                    inst(CreateSDiv, CreateFDiv)
                if (oper == "%")
                    inst(CreateSRem, CreateFRem)
                if (oper == "<<")
                    sinst(CreateShl)
                if (oper == ">>")
                    sinst(CreateLShr)
                if (oper == "&")
                    sinst(CreateAnd)
                if (oper == "|")
                    sinst(CreateOr)
                if (oper == "^")
                    sinst(CreateXor)
                if (oper == "in" || oper == ".." || oper == "..=") 
                    hurl(SEMANTIC, "Binary operator `" + oper + "` does not support primitive arguments on both sides", nd->toks.front(), "", true);
                if (oper == ":") {
                    if (dynamic_cast<TypeCall *>(right)->smatches(new TypeCall("f32")) && left->exprType->smatches(new TypeCall("i32"))) {
                        auto *l = left->codegen(info);
                        int bits = std::stoi(dynamic_cast<TypeCall *>(right)->type->name); 
                        if (bits == 16) 
                            return info.builder->CreateSIToFP(l, llvm::Type::getHalfTy(*info.context)); 
                        else if (bits == 32) 
                            return info.builder->CreateSIToFP(l, llvm::Type::getFloatTy(*info.context)); 
                        else if (bits == 64) 
                            return info.builder->CreateSIToFP(l, llvm::Type::getDoubleTy(*info.context)); 
                        else if (bits == 128) 
                            return info.builder->CreateSIToFP(l, llvm::Type::getFP128Ty(*info.context)); 
                        return info.builder->CreateSIToFP(l, llvm::Type::getFloatTy(*info.context)); 
                    }

                    if (dynamic_cast<TypeCall *>(right)->smatches(new TypeCall("i32")) && (left->exprType->smatches(new TypeCall("f32")) || left->exprType->smatches(new TypeCall("bf")))) {
                        return info.builder->CreateFPToSI(left->codegen(info), llvm::IntegerType::get(*info.context, std::stoi(dynamic_cast<TypeCall *>(right)->type->name.substr(1))));
                    }

                    if (dynamic_cast<TypeCall *>(right)->smatches(new TypeCall("bf")) && left->exprType->smatches(new TypeCall("i32"))) {
                        return info.builder->CreateSIToFP(left->codegen(info), llvm::Type::getBFloatTy(*info.context)); 
                    }
                }
                if (oper == "<")
                    inst(CreateICmpSLT, CreateFCmpULT)
                if (oper == ">")
                    inst(CreateICmpSGT, CreateFCmpUGT)
                if (oper == "<=")
                    inst(CreateICmpSLE, CreateFCmpULE)
                if (oper == ">=")
                    inst(CreateICmpSGE, CreateFCmpUGE)
                if (oper == "==")
                    inst(CreateICmpEQ, CreateFCmpUEQ)
                if (oper == "!=")
                    inst(CreateICmpNE, CreateFCmpUNE)
                if (oper == "||")
                    return info.builder->CreateLogicalOr(left->codegen(info), right->codegen(info));
                if (oper == "&&")
                    return info.builder->CreateLogicalAnd(left->codegen(info), right->codegen(info));
                if (oper == "=") {
                    if (auto *id = dynamic_cast<Identifier *>(left)) 
                        if (auto *var = dynamic_cast<Variable *>(id->id)) 
                            return info.builder->CreateStore(right->codegen(info), left->codegen(info), var->isVolatile);
                    return info.builder->CreateStore(right->codegen(info), left->codegen(info), false);
                }
                if (oper == "+=") {
                    auto *add = [&]()->llvm::Value * { inst(CreateAdd, CreateFAdd) }();
                    if (auto *id = dynamic_cast<Identifier *>(left)) 
                        if (auto *var = dynamic_cast<Variable *>(id->id)) 
                            return info.builder->CreateStore(right->codegen(info), left->codegen(info), var->isVolatile);
                    return info.builder->CreateStore(right->codegen(info), left->codegen(info), false);
                }
                if (oper == "-=") {
                    auto *add = [&]()->llvm::Value * { inst(CreateSub, CreateFSub) }();
                    if (auto *id = dynamic_cast<Identifier *>(left)) 
                        if (auto *var = dynamic_cast<Variable *>(id->id)) 
                            return info.builder->CreateStore(right->codegen(info), left->codegen(info), var->isVolatile);
                    return info.builder->CreateStore(right->codegen(info), left->codegen(info), false);
                }
                if (oper == "*=") {
                    auto *add = [&]()->llvm::Value * { inst(CreateMul, CreateFMul) }();
                    if (auto *id = dynamic_cast<Identifier *>(left)) 
                        if (auto *var = dynamic_cast<Variable *>(id->id)) 
                            return info.builder->CreateStore(right->codegen(info), left->codegen(info), var->isVolatile);
                    return info.builder->CreateStore(right->codegen(info), left->codegen(info), false);
                }
                if (oper == "/=") {
                    auto *add = [&]()->llvm::Value * { inst(CreateSDiv, CreateFDiv) }();
                    if (auto *id = dynamic_cast<Identifier *>(left)) 
                        if (auto *var = dynamic_cast<Variable *>(id->id)) 
                            return info.builder->CreateStore(right->codegen(info), left->codegen(info), var->isVolatile);
                    return info.builder->CreateStore(right->codegen(info), left->codegen(info), false);
                }
                if (oper == "%=") {
                    auto *add = [&]()->llvm::Value * { inst(CreateSRem, CreateFRem) }();
                    if (auto *id = dynamic_cast<Identifier *>(left)) 
                        if (auto *var = dynamic_cast<Variable *>(id->id)) 
                            return info.builder->CreateStore(right->codegen(info), left->codegen(info), var->isVolatile);
                    return info.builder->CreateStore(right->codegen(info), left->codegen(info), false);
                }
                if (oper == "<<=") {
                    auto *add = [&]()->llvm::Value * { sinst(CreateShl) }();
                    if (auto *id = dynamic_cast<Identifier *>(left)) 
                        if (auto *var = dynamic_cast<Variable *>(id->id)) 
                            return info.builder->CreateStore(right->codegen(info), left->codegen(info), var->isVolatile);
                    return info.builder->CreateStore(right->codegen(info), left->codegen(info), false);
                }
                if (oper == ">>=") {
                    auto *add = [&]()->llvm::Value * { sinst(CreateLShr) }();
                    if (auto *id = dynamic_cast<Identifier *>(left)) 
                        if (auto *var = dynamic_cast<Variable *>(id->id)) 
                            return info.builder->CreateStore(right->codegen(info), left->codegen(info), var->isVolatile);
                    return info.builder->CreateStore(right->codegen(info), left->codegen(info), false);
                }
                if (oper == "&=") {
                    auto *add = [&]()->llvm::Value * { sinst(CreateAnd) }();
                    if (auto *id = dynamic_cast<Identifier *>(left)) 
                        if (auto *var = dynamic_cast<Variable *>(id->id)) 
                            return info.builder->CreateStore(right->codegen(info), left->codegen(info), var->isVolatile);
                    return info.builder->CreateStore(right->codegen(info), left->codegen(info), false);
                }
                if (oper == "|=") {
                    auto *add = [&]()->llvm::Value * { sinst(CreateOr) }();
                    if (auto *id = dynamic_cast<Identifier *>(left)) 
                        if (auto *var = dynamic_cast<Variable *>(id->id)) 
                            return info.builder->CreateStore(right->codegen(info), left->codegen(info), var->isVolatile);
                    return info.builder->CreateStore(right->codegen(info), left->codegen(info), false);
                }
                if (oper == "^=") {
                    auto *add = [&]()->llvm::Value * { sinst(CreateXor) }();
                    if (auto *id = dynamic_cast<Identifier *>(left)) 
                        if (auto *var = dynamic_cast<Variable *>(id->id)) 
                            return info.builder->CreateStore(right->codegen(info), left->codegen(info), var->isVolatile);
                    return info.builder->CreateStore(right->codegen(info), left->codegen(info), false);
                }
                return nullptr;
            }

            if (oper == ".") {
                if (auto *call = dynamic_cast<CallExpr *>(right)) {
                    if (left->exprType->refs.size() > 0) {
                        call->args.insert(call->args.begin(), left);
                        call->argTypes.insert(call->argTypes.begin(), left->exprType);
                        auto *func = call->actual;
                        func->params.insert(func->params.begin(), new Variable("this", left->exprType, nd));
                    }
                    else {
                        auto *r = dynamic_cast<TypeCall *>(left->exprType->rewrite({}, {}));
                        r->refs.push_back("ref");
                        auto *v = new Variable(Decl::genName(), left->exprType, nd);
                        v->codegen(info);
                        call->args.insert(call->args.begin(), new UnExpr("&", new Identifier(v), false));
                        call->argTypes.insert(call->argTypes.begin(), r);
                        auto *func = call->actual;
                        func->params.insert(func->params.begin(), new Variable("this", r, nd));
                    }
                    return call->codegen(info);
                }

                else if (auto *id = dynamic_cast<Identifier *>(right)) {
                    for (size_t i = 0; i < left->exprType->type->members.size(); i++) {
                        auto [mods, member] = left->exprType->type->members[i];
                        if (member->name == id->id->name) {
                            auto *v = left->codegen(info);
                            if (left->exprType->refs.size() > 0) {
                                v = info.builder->CreateLoad(v->getType(), v);
                            }
                            auto *a = info.builder->CreateStructGEP(left->exprType->getType(info), v, i);
                            return info.builder->CreateLoad(id->id->getType(info), a);
                        }
                    }
                }
            }

            auto *overload = OperOverload::findOverload(oper, { left->exprType, right->exprType });
            if (!overload) {
                hurl(SEMANTIC, oper + " operator overload for args: (" + left->exprType->print() + ", " + right->exprType->print() + ") is not defined", nd->toks.front(), "", true);
            }

            llvm::Function *overloadFunc = info._module->getFunction(overload->name);
            return info.builder->CreateCall(overloadFunc, { left->codegen(info), right->codegen(info) });
        }

        llvm::Type *getType(LLVMInfo info) const override {
            auto *overload = OperOverload::findOverload(oper, { left->exprType, right->exprType });
            if (!overload) {
                hurl(SEMANTIC, oper + " operator overload for args: (" + left->exprType->print() + ", " + right->exprType->print() + ") is not defined", nd->toks.front(), "", true);
            }
            return overload->ret->getType(info);
        }
    };

}

#endif /* CCDE879B_5ED1_41F8_8740_A1960C76E8C8 */

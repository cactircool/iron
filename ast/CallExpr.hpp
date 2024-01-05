#ifndef FAA013FE_5806_40A1_92A3_4D83F27C8432
#define FAA013FE_5806_40A1_92A3_4D83F27C8432

#include "Expr.hpp"
#include "Function.hpp"
#include "Literal.hpp"

#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Instructions.h"

namespace iron {

    struct CallExpr : public Expr {
        std::string callName;
        Function *call;
        Function *actual;
        std::string callType;
        std::vector<Expr *> args;
        std::vector<TypeCall *> argTypes;

        CallExpr(Node *node) : Expr(node) {
            if (auto *argsNode = find("args", node)) {
                callType = argsNode->children[0]->value;
                for (size_t i = 1; i < argsNode->children.size(); i++) {
                    args.push_back(dynamic_cast<Expr *>(ASTNode::parse(argsNode->children[i])));
                    argTypes.push_back(args.back()->exprType);
                }
            }

            if (auto *name = find("name", node)) {
                Node *copy = name;
                name = name->children[0];
                bool isFull = true;

                if (name->children.size() <= 1) {
                    name = copy;
                    isFull = false;
                }

                std::string stack = "";
                for (auto *child : name->children) {
                    stack += (!stack.empty() ? "::" : "") + child->children[0]->value;
                    if (auto *lab = find("label", child)) {
                        stack += "[" + lab->children[0]->value + "]";
                    }
                }

                call = dynamic_cast<Function *>(Decl::getDecl(stack, true, [&](Decl *decl) {
                    auto *f = dynamic_cast<Function *>(decl);
                    return f && f->matches(argTypes);
                }));

                callName = stack;

                if (!call && !isSystemFunc()) {
                    hurl(SEMANTIC, "No function matching both name and arguments found", node->toks.front(), "", true);
                }

                if (!isSystemFunc()) {
                    std::map<std::string, TypeCall *> genMap;
                    for (size_t i = 0; i < call->params.size(); i++)
                        call->params[i]->type->fillGenerics(argTypes[i], genMap, args[i]->nd);

                    actual = dynamic_cast<Function *>(call->rewrite(genMap, {}));
                    setExprType(actual->ret);

                    if (rewrittenFunctions.find(call->name) == rewrittenFunctions.end())
                        rewrittenFunctions.insert({ call->name, {} });
                    rewrittenFunctions[call->name].push_back(actual);
                } else {
                    setExprType(getSystemRetType());
                }
            }
        }
        CallExpr(std::string callName, std::string callType, std::vector<Expr *> args, std::vector<TypeCall *> argTypes, Node *node)
            : Expr(node), callName(callName), callType(callType), args(args), argTypes(argTypes) {
            call = dynamic_cast<Function *>(Decl::getDecl(callName, true, [&](Decl *decl) {
                auto *f = dynamic_cast<Function *>(decl);
                return f && f->matches(argTypes);
            }));

            if (!call && !isSystemFunc()) {
                hurl(SEMANTIC, "No function matching both name and arguments found", node->toks.front(), "", true);
            }

            if (!isSystemFunc()) {
                std::map<std::string, TypeCall *> genMap;
                for (size_t i = 0; i < call->params.size(); i++)
                    call->params[i]->type->fillGenerics(argTypes[i], genMap, args[i]->nd);

                actual = dynamic_cast<Function *>(call->rewrite(genMap, {}));
                setExprType(actual->ret);

                if (rewrittenFunctions.find(call->name) == rewrittenFunctions.end())
                    rewrittenFunctions.insert({ call->name, {} });
                rewrittenFunctions[call->name].push_back(actual);
            } else {
                setExprType(getSystemRetType());
            }
        }

        bool isSystemFunc() const {
            return callName == "printf";
        }

        TypeCall *getSystemRetType() const {
            if (callName == "print") return new TypeCall("i32");
            return nullptr;
        }

        static std::string unsanitizeString(const std::string& sanitizedString) {
            std::string unsanitizedString;
            bool escape = false;

            for (char c : sanitizedString) {
                if (escape) {
                    switch (c) {
                        case 'n':
                            unsanitizedString += '\n';
                            break;
                        case 't':
                            unsanitizedString += '\t';
                            break;
                        // Add more cases for other escape sequences as needed
                        case '\\':
                            unsanitizedString += '\\';
                            break;
                        default:
                            // If there's an unsupported escape sequence, just add the escaped character
                            unsanitizedString += '\\';
                            unsanitizedString += c;
                            break;
                    }
                    escape = false;
                } else if (c == '\\') {
                    escape = true;
                } else {
                    unsanitizedString += c;
                }
            }

            return unsanitizedString;
        }

        std::string print() const override {
            std::string end = callType == "(" ? ")" : callType == "[" ? "]" : callType == "{" ? "}" : ")";
            return callName + printArray(args, callType, end, [&](Expr *arg) { return arg->print(); }, ", ");
        }

        llvm::Value *kprintf(LLVMInfo info, const char *format, std::vector<Expr *> _args) {
            // for (auto &func : *info._module) {
            //     std::cout << func.getName().str() << std::endl;
            // }
            // sleep(20);
            llvm::Function *func_printf = info._module->getFunction("printf");
            if (!func_printf) {
                llvm::PointerType *Pty = llvm::PointerType::get(llvm::IntegerType::get(info._module->getContext(), 8), 0);
                // llvm::FunctonType *FuncTy9 = llvm::FunctionType::get(llvm::IntegerType::get(info._module->getContext(), 32), {  }, true);

                // func_printf = llvm::Function::Create(FuncTy9, llvm::GlobalValue::ExternalLinkage, "printf", info._module);
                llvm::FunctionType *FuncTy9 = llvm::FunctionType::get(llvm::IntegerType::getInt32Ty(*info.context), llvm::PointerType::get(llvm::IntegerType::getInt8Ty(*info.context), 0), true);
                func_printf = llvm::Function::Create(FuncTy9, llvm::Function::ExternalLinkage, "printf", info._module);
                func_printf->setCallingConv(llvm::CallingConv::C);

                llvm::AttributeList func_printf_PAL;
                func_printf->setAttributes(func_printf_PAL);
            }

            llvm::Value *str = info.builder->CreateGlobalStringPtr(format);
            std::vector<llvm::Value *> int32_call_params;
            int32_call_params.push_back(str);

            for (auto val : _args)
                int32_call_params.push_back(val->codegen(info));

            return info.builder->CreateCall(func_printf, int32_call_params, "call");
        }

        llvm::Value *codegen(LLVMInfo info) override {
            if (isSystemFunc()) {
                if (callName == "printf") {
                    std::string value = dynamic_cast<Literal *>(args[0])->value;
                    return kprintf(info, unsanitizeString(value.substr(1, value.size() - 2)).c_str(), { args.begin() + 1, args.end() });
                }
            }
            auto *f = info._module->getFunction(actual->name);
            if (!f) {
                f = llvm::dyn_cast<llvm::Function>(actual->codegen(info));
            }
            std::vector<llvm::Value *> _args;
            _args.reserve(args.size());
            for (auto *arg : args) {
                _args.push_back(arg->codegen(info));
                auto *a = info.builder->CreateAlloca(arg->getType(info));
                info.builder->CreateStore(_args.back(), a);
                _args.back() = a;
            }
            return info.builder->CreateCall(f, _args);
        }

        llvm::Type *getType(LLVMInfo info) const override {
            return info._module->getFunction(actual->name)->getReturnType();
        }

        ASTNode *rewrite(std::map<std::string, TypeCall *> gens, std::map<std::string, Decl *> vars) override {
            std::vector<Expr *> copyArgs;
            copyArgs.reserve(args.size());
            std::vector<TypeCall *> copyArgTypes;
            copyArgTypes.reserve(args.size());
            for (auto *arg : args) {
                copyArgs.push_back(dynamic_cast<Expr *>(arg->rewrite(gens, vars)));
                copyArgTypes.push_back(copyArgs.back()->exprType);
            }
            return new CallExpr(callName, callType, copyArgs, copyArgTypes, nd);
        }
    };

}

#endif /* FAA013FE_5806_40A1_92A3_4D83F27C8432 */

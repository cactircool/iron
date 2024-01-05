#ifndef D055037D_BDB1_4FFB_9921_2EA907A0A67F
#define D055037D_BDB1_4FFB_9921_2EA907A0A67F

#include <string>
#include <memory>
#include <map>
#include "../src/ParseTree.hpp"

#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Support/Alignment.h"
#include "llvm/Transforms/Utils/ModuleUtils.h"

namespace iron {

    struct Function;

    Node *currentNode = nullptr;
    llvm::LLVMContext *context;
    llvm::IRBuilder<> *builder;
    llvm::Module *__module;
    llvm::BasicBlock *block;

    std::map<std::string, std::vector<Function *>> rewrittenFunctions;

    struct LLVMInfo {
        llvm::LLVMContext *context;
        llvm::IRBuilder<> *builder;
        llvm::Module *_module;
        llvm::BasicBlock *block;
    };

    struct TypeCall;
    struct Decl;

    struct ASTNode {
        Node *nd;
        ASTNode(Node *node) : nd(node) {}
        virtual ~ASTNode() = default;

        virtual std::string print() const = 0;
        virtual ASTNode *clone() const { return nullptr; }
        virtual ASTNode *rewrite(std::map<std::string, TypeCall *>, std::map<std::string, Decl *>) { return nullptr; }

        virtual llvm::Value *codegen(LLVMInfo) = 0;
        virtual llvm::Type *getType(LLVMInfo) const = 0;

        static ASTNode *parse(Node *);
    };

    template <typename T, typename F>
    std::string printArray(std::vector<T> vec, std::string start, std::string end, F print, std::string separator, bool emptyEmpty = false) {
        if (emptyEmpty && vec.empty()) return "";
        std::string result = start;
        for (auto thing : vec) 
            result += print(thing) + separator;
        if (!vec.empty()) {
            result.pop_back();
            result.pop_back();
        }
        return result + end;
    }

    template <typename T, typename F>
    std::string printScope(std::vector<T> vec, F print) {
        std::string s = "{\n";
        tabs++;
        for (auto thing : vec) {
            for (int i = 0; i < tabs; i++)
                s.push_back('\t');
            s.append(print(thing)).push_back('\n');
        }
        tabs--;
        for (int i = 0; i < tabs; i++)
            s.push_back('\t');
        s.push_back('}');
        return s;
    }

}

#endif /* D055037D_BDB1_4FFB_9921_2EA907A0A67F */

#include <iostream>
#include "src/Compiler.hpp"
#include "src/ParseTree.hpp"
#include "ast/ASTNode.hpp"
#include "ast/Type.hpp"
#include "ast/Variable.hpp"
#include "ast/Literal.hpp"
#include "ast/Function.hpp"
#include "ast/CallExpr.hpp"
#include "ast/Return.hpp"
#include "ast/Identifier.hpp"
#include "ast/BinExpr.hpp"
#include "ast/UnExpr.hpp"
#include "ast/OperOverload.hpp"

#include "llvm/IR/Verifier.h"
#include "llvm/Support/CodeGen.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/TargetParser/Host.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Target/TargetOptions.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Support/CodeGen.h"
#include "llvm/Support/TargetSelect.h"

namespace iron {
    ASTNode *ASTNode::parse(Node *node) {
        if (node->value == "variable") return new Variable(node);
        if (node->value == "type" || node->value == "class" || node->value == "struct" || node->value == "trait") return new Type(node);
        if (node->value == "function") return new Function(node);
        if (node->value == "call") return new CallExpr(node);
        if (node->value == "ret") return new Return(node);
        if (node->value == "datatype") return new Identifier(node);
        if (node->value == "oper_overload") return new OperOverload(node);
        if (node->value == "binary_expr") return new BinExpr(node);
        if (node->value.size() > 10 && node->value.substr(node->value.size() - 10) == "unary_expr") return new UnExpr(node);
        if (node->value.size() > 7 && node->value.substr(node->value.size() - 7) == "literal") return new Literal(node);
        return nullptr;
    }

    NodePrefixStruct create(const std::vector<Token> &toks, const std::string &line) {
        auto *node = parse(toks, line);
        if (!node) return { .node = nullptr, .prefix = {} };
        currentNode = node;
        std::cout << node->toString() << std::endl;
        ASTNode *ast = ASTNode::parse(node);
        return { ast, {} };
    }

    bool hasGeneric(ASTNode *node) {
        if (auto *a = dynamic_cast<Function *>(node))
            return !a->generics.empty();
        if (auto *a = dynamic_cast<Type *>(node))
            return !a->generics.empty();
        return false;
    }

    void createSystemFunctions(LLVMInfo info) {
        llvm::Function *func_printf = info._module->getFunction("printf");
        if (!func_printf) {
            llvm::PointerType *Pty = llvm::PointerType::get(llvm::IntegerType::get(info._module->getContext(), 8), 0);
            llvm::FunctionType *FuncTy9 = llvm::FunctionType::get(llvm::IntegerType::getInt32Ty(*info.context), llvm::PointerType::get(llvm::IntegerType::getInt8Ty(*info.context), 0), true);
            func_printf = llvm::Function::Create(FuncTy9, llvm::Function::ExternalLinkage, "printf", info._module);
            func_printf->setCallingConv(llvm::CallingConv::C);

            func_printf = llvm::Function::Create(FuncTy9, llvm::GlobalValue::ExternalLinkage, "printf", info._module);
            func_printf->setCallingConv(llvm::CallingConv::C);

            llvm::AttributeList func_printf_PAL;
            func_printf->setAttributes(func_printf_PAL);
        }
    }
}

int main(int argc, char **argv) {
    if (argc != 2) {
        std::cerr << "Usage: iron [FILENAME]" << std::endl;
        return 1;
    }

    std::cout << std::boolalpha;

    iron::context = new llvm::LLVMContext();
    iron::builder = new llvm::IRBuilder<>(*iron::context);
    iron::__module = new llvm::Module("Module", *iron::context);
    iron::block = llvm::BasicBlock::Create(*iron::context, "stratosphere");
    iron::LLVMInfo info = {
        iron::context,
        iron::builder,
        iron::__module,
        iron::block
    };

    new iron::Type("c");
    new iron::Type("b");
    new iron::Type("bf");
    new iron::Type("void");

    iron::createSystemFunctions(info);

    std::string filename = argv[1];
    auto output = filename.find_last_of('.') != std::string::npos ? filename.substr(0, filename.find_last_of('.')) + ".o" : filename + ".o";
    auto irepr = filename.find_last_of('.') != std::string::npos ? filename.substr(0, filename.find_last_of('.')) + ".o" : filename + ".o";
    iron::compile(filename);

    for (auto *thing : iron::Program::program) {
         auto *t = dynamic_cast<iron::Type *>(thing);
         if (t && t->generics.empty())
             thing->getType(info);
         else if (t) {
             for (auto [g, type] : t->rewrites) {
                 type->getType(info);
             }
         }

         if (!iron::hasGeneric(thing)) {
             thing->codegen(info);
         }
    }

    std::string ir;
    llvm::raw_string_ostream strm(ir);
    info._module->print(strm, nullptr);

    std::fstream llvmir(irepr, std::ios::out | std::ios::trunc);
    if (llvmir) {
        llvmir << ir << std::endl;
        llvmir.close();
    }

    // info._module->dump();

    llvm::verifyModule(*iron::__module);

    llvm::InitializeAllTargetInfos();
    llvm::InitializeAllTargets();
    llvm::InitializeAllTargetMCs();
    llvm::InitializeAllAsmParsers();
    llvm::InitializeAllAsmPrinters();

    auto targetTriple = llvm::sys::getDefaultTargetTriple();
    std::string error;
    auto target = llvm::TargetRegistry::lookupTarget(targetTriple, error);

    if (!target) {
        std::cerr << error << std::endl;
        return 1;
    }

    auto cpu = "generic";
    auto features = "";

    llvm::TargetOptions opt;
    auto targetMachine = target->createTargetMachine(targetTriple, cpu, features, opt, llvm::Reloc::PIC_);

    info._module->setDataLayout(targetMachine->createDataLayout());
    info._module->setTargetTriple(targetTriple);

    std::error_code EC;
    llvm::raw_fd_ostream dest(output, EC, llvm::sys::fs::OF_None);
    
    if (EC) {
        std::cerr << "Error: could not open file: " << EC.message();
        return 1;
    }

    llvm::legacy::PassManager pass;
    auto fileType = llvm::CodeGenFileType::CGFT_ObjectFile;

    if (targetMachine->addPassesToEmitFile(pass, dest, nullptr, fileType)) {
        std::cerr << "targetMachine can't emit a file of this type" << std::endl;
        return 1;
    }

    pass.run(*info._module);
    dest.flush();
}
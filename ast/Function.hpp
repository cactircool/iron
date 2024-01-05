#ifndef FBBF4A45_01A0_4AD7_BC37_38FB0DF1D2BA
#define FBBF4A45_01A0_4AD7_BC37_38FB0DF1D2BA

#include "Decl.hpp"
#include "TypeCall.hpp"
#include "Variable.hpp"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/IR/Constant.h"
#include "llvm/IR/Instructions.h"

namespace iron {

    struct Function : public Decl {
        std::vector<TypeCall *> generics;
        bool isPure;
        bool isAsync;
        TypeCall *ret;
        std::vector<Variable *> params;
        std::vector<TypeCall *> hurls;
        std::vector<ASTNode *> code;

        std::map<std::string, Function *> rewrites;

        Function(Node *node) : Decl(node) {
            if (auto *gens = find("gens", node)) {
                for (auto *gen : gens->children) {
                    generics.push_back(TypeCall::createGeneric(gen->children[0]->value));
                    TypeCall::genMap[gen->children[0]->value] = generics.back();
                }
            }

            if (auto *mods = find("mods", node)) {
                isPure = find("pure", mods);
                isAsync = find("async", mods);
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

                setName(stack, isFull, true);
            }

            if (auto *dt = find("datatype", node)) {
                ret = new TypeCall(dt);
            }

            if (auto *paramsNode = find("params", node)) {
                for (auto *param : paramsNode->children)
                    params.push_back(new Variable(param));
            }

            if (auto *hurlsNode = find("hurls", node)) {
                for (auto *hurl : hurlsNode->children)
                    hurls.push_back(new TypeCall(hurl));
            }

            if (auto *scope = find("scope", node)) {
                for (auto *item : scope->children) {
                    if (item)
                        code.push_back(ASTNode::parse(item));
                }
            }

            Decl::removeLastScope();
            for (auto thing : code) {
                if (auto d = dynamic_cast<Decl *>(thing)) {
                    Decl::decls[d->name].pop_back();
                }
            }

            for (auto *gen : generics) {
                TypeCall::genMap.erase(gen->genericName);
            }
        }
        Function(Node *node, bool strictPass) : Decl(node) {}

        Function(const std::string &name, const std::vector<TypeCall *> &generics, bool isPure, bool isAsync, TypeCall *ret,
                 const std::vector<Variable *> &params, const std::vector<TypeCall *> &hurls,
                 const std::vector<ASTNode *> &code) : Decl(nullptr), generics(generics), isPure(isPure), isAsync(isAsync), ret(ret),
                                                       params(params), hurls(hurls), code(code) {
            setName(name, true);
        }

        bool matches(std::vector<TypeCall *> ps) {
            if (ps.size() != params.size()) return false;
            for (size_t i = 0; i < params.size(); i++) {
                if (!params[i]->type->matches(ps[i]) && !params[i]->type->isGeneric())
                    return false;
            }
            return true;
        }

        std::string print() const override {
            std::string s = "func " + printArray(generics, "<", "> ", [&](TypeCall *nd) { return nd->print(); }, ", ", true);
            if (isPure) s += "pure ";
            if (isAsync) s += "async ";

            s += name + ":" + ret->print() + printArray(params, "(", ")", [&](Variable *v) { return v->print(); }, ", ") + " ";
            s += printArray(hurls, "hurls ", " ", [&](TypeCall *tc) { return tc->print(); }, ", ", true);
            s += printScope(code, [&](ASTNode *nd) { return nd->print(); });
            return s;
        }

        llvm::Value *codegen(LLVMInfo info) override {
            if (auto *f = info._module->getFunction(name)) {
                return f;
            }

            std::vector<llvm::Type *> types;
            for (auto *var : params)
                types.push_back(var->getType(info));
            llvm::FunctionType *ft = llvm::FunctionType::get(ret->getType(info), llvm::ArrayRef<llvm::Type *>(types), false);
            llvm::Function *func = llvm::Function::Create(ft, llvm::GlobalValue::LinkageTypes::ExternalLinkage, name, *info._module);
            // llvm::Function *func = info._module->getFunction(name);

            llvm::BasicBlock *block = llvm::BasicBlock::Create(*info.context, "entry", func);
            llvm::IRBuilder<> builder(block);

            auto *lastBuilder = info.builder;
            info.builder = &builder;

            for (auto *item : code) {
                item->codegen(info);
            }

            if (!ret->isGeneric() && ret->type->name == "void")
                info.builder->CreateRetVoid();
            if (!ret->isGeneric() && ret->type->name[0] == 'i' && std::to_string(std::stoi(ret->type->name.substr(1))).size() == ret->type->name.size() - 1) {
                if (!llvm::isa<llvm::ReturnInst>(info.builder->GetInsertBlock()->back()))
                    info.builder->CreateRet(llvm::ConstantInt::get(llvm::IntegerType::get(*(info.context), std::stoi(ret->type->name.substr(1))), 0, true));
            }

            info.builder = lastBuilder;

            for (auto &[name, f] : rewrites)
                f->codegen(info);

            return func;
        }

        llvm::Type *getType(LLVMInfo info) const override {
            return ret->getType(info);
        }

        ASTNode *rewrite(std::map<std::string, TypeCall *> gens, std::map<std::string, Decl *> vars) override {
            std::string check;
            for (auto [n, tc] : gens)
                check.append(tc->print()).append(", ");

            if (rewrites.find(check) != rewrites.end())
                return rewrites[check];

            std::vector<Variable *> rws;
            std::map<std::string, Decl *> varMap;
            rws.reserve(params.size());
            for (auto *param : params) {
                rws.push_back(dynamic_cast<Variable *>(param->rewrite(gens, vars)));

                varMap.insert({ param->name, rws.back() });
            }

            std::vector<TypeCall *> hurlRewrites;
            hurlRewrites.reserve(hurls.size());
            for (auto *hurl : hurls)
                hurlRewrites.push_back(dynamic_cast<TypeCall *>(hurl->rewrite(gens, varMap)));

            std::vector<ASTNode *> codeRewrite;
            codeRewrite.reserve(code.size());
            for (auto *item : code)
                codeRewrite.push_back(item->rewrite(gens, varMap));

            auto *copy = new Function(Decl::genName(), {}, isPure, isAsync, dynamic_cast<TypeCall *>(ret->rewrite(gens, varMap)), rws, hurlRewrites, codeRewrite);
            rewrites[check] = copy;
            return copy;
        }
    };

    llvm::Type *Type::getType(LLVMInfo info) const {
        if (name == "b" || name == "c" || name == "bf" ||
            (name.size() > 1 && (name[0] == 'i' || name[0] == 'f') && isdigit(name[1]) && (std::to_string(std::stoi(name.substr(1))).size() == name.size() - 1))) {
            if (name[0] == 'i') {
                return llvm::IntegerType::get(*(info.context), std::stoi(name.substr(1)));
            }
            else if (name[0] == 'b') {
                if (name.size() > 1) 
                    return llvm::Type::getBFloatTy(*(info.context));
                return llvm::IntegerType::get(*(info.context), 1);
            }
            else if (name[0] == 'c')
                return llvm::IntegerType::get(*(info.context), 8);
            else if (name[0] == 'f') {
                int bits = std::stoi(name.substr(1));
                switch (bits) {
                    case 16: return llvm::Type::getHalfTy(*(info.context));
                    case 32: return llvm::Type::getFloatTy(*(info.context));
                    case 64: return llvm::Type::getDoubleTy(*(info.context));
                    case 128: return llvm::Type::getFP128Ty(*(info.context));
                    default: {
                        hurl(SEMANTIC, "no primtive floating point type with " + std::to_string(bits) + " exists", currentNode->toks.front(), "f16, f32, f64, f128 only", true);
                    }
                }
            }
        }

        if (name == "void")
            return llvm::Type::getVoidTy(*info.context);

        llvm::Type *found = llvm::StructType::getTypeByName(*(info.context), name);
        if (found)
            return found;

        llvm::StructType *type = llvm::StructType::create(*info.context, name);
        std::vector<llvm::Type *> fields;
        for (auto &[mods, decl] : members) {
            if (auto *var = dynamic_cast<Variable *>(decl)) {
                if (var->type->type->name == type->getName()) {
                    fields.push_back(type->getPointerTo());
                } else {
                    fields.push_back(decl->getType(info));
                }
            }

//            else if (auto *func = dynamic_cast<Function *>(decl)) {
//                func->params.insert(func->params.begin(), new Variable("this", new TypeCall(const_cast<Type *>(this), "", {}, { "ref" }, nd), nd));
//            }
        }
        type->setBody(fields);

//        for (auto &[mods, decl] : members) {
//            if (auto *func = dynamic_cast<Function *>(decl)) {
//                func->codegen(info);
//            }
//        }

        // llvm::StructType *type = llvm::StructType::create(*(info.context), llvm::ArrayRef<llvm::Type *>(fields), name, isPacked);
        // llvm::FunctionType *ft = llvm::FunctionType::get(type, fields, false);
        // llvm::Function *constructor = llvm::Function::Create(ft, llvm::Function::LinkageTypes::ExternalLinkage, Decl::genName(), *info._module);

        // llvm::BasicBlock *block = llvm::BasicBlock::Create(*info.context, "entry", constructor);
        // llvm::IRBuilder<> builder(block);
        // builder.CreateAlignedStore(val, glob, llvm::Align());

        return type;
    }

}

#endif /* FBBF4A45_01A0_4AD7_BC37_38FB0DF1D2BA */

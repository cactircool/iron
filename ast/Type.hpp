#ifndef D7DB2142_5FC0_4C36_A7CA_CA092DB33517
#define D7DB2142_5FC0_4C36_A7CA_CA092DB33517

#include "Decl.hpp"

namespace iron {

    struct TypeCall;

    struct Type : public Decl {
        std::vector<std::string> mods;
        std::vector<TypeCall *> generics;
        TypeCall *super;
        std::vector<TypeCall *> impls; // deal with inheritance later
        std::vector<std::tuple<std::vector<std::string>, Decl *>> members;
        bool isPacked;

        std::map<std::string, Type *> rewrites;

        static Type *currentType;

        // mangle names later
        Type(Node *node);

        // primitive constructor
        Type(const std::string &name, bool isPrimitive = false) : Decl(nullptr) {
            if (isPrimitive) {
                if (Decl::decls.find(name) == Decl::decls.end())
                    Decl::decls.insert({ name, {} });
                Decl::decls[name].push_back(this);
                this->name = name;
            } else {
                setName(name);
            }
        }

        Decl *getMember(std::string name) {
            for (auto [mods, decl] : members) {
                if (decl->nameMatches(name))
                    return decl;
            }
            return nullptr;
        }

        std::string print() const override;

        llvm::Value *codegen(LLVMInfo) override {
            return nullptr; // ? not entirely sure how this works, but I'm thinking you can return nullptr and construct the type anyway
        }

        llvm::Type *getType(LLVMInfo) const override;

        ASTNode *rewrite(std::map<std::string, TypeCall *>, std::map<std::string, Decl *>) override;
    };

    Type *Type::currentType = nullptr;

}

#endif /* D7DB2142_5FC0_4C36_A7CA_CA092DB33517 */

#ifndef C604EB05_2F6A_4447_A719_2543D6DDB865
#define C604EB05_2F6A_4447_A719_2543D6DDB865

#include "Expr.hpp"
#include "Label.hpp"
#include "Type.hpp"

namespace iron {

    struct TypeCall : public Expr {
        Type *type;
        std::string label;
        std::string genericName;
        std::vector<TypeCall *> gens;
        std::map<std::string, TypeCall *> generics;

        static std::map<std::string, TypeCall *> genMap;

        std::vector<std::string> refs;

        TypeCall() : Expr(nullptr), type(nullptr), label(), genericName(), generics() {}
        TypeCall(Node *node, bool isGeneric = false) : Expr(node) {
            while (node->value == "dref" || node->value == "array") {
                refs.push_back(node->value);
                node = node->children[0];
            }
            type = dynamic_cast<Type *>(
                Decl::getDecl((find("gens", node) || find("label", node)) ? node->children[0]->children[0]->value : node->children[0]->value,
                true, 
                [&](Decl *decl) -> bool {
                    return dynamic_cast<Type *>(decl) != nullptr;
                }));

            std::string name = node->children[0]->value;
            if (!type && (!name.empty() && (name[0] == 'f' || name[0] == 'i') && std::to_string(std::stoi(name.substr(1))).size() == name.size() - 1)) {
                type = new Type(name, true);
            }

            if (!type) {
                if (genMap.find(name) == genMap.end())
                    hurl(SEMANTIC,
                        "type `" + node->children[0]->value + "` is not declared",
                        node->children[0]->toks.front(),
                        "", true);
                else
                    genericName = name;
            }

            if (auto *lab = find("label", node)) {
                label = lab->children[0]->value;
                hurl(SEMANTIC,
                    "there is no label `" + label + "` in type `" + type->name + "`",
                    lab->children[0]->toks.front(), "", true);
            }

            if (auto *gensNode = find("gens", node)) {
                for (auto *gen : gensNode->children)
                    gens.push_back(new TypeCall(gen));

                if (gens.size() != type->generics.size()) {
                    hurl(SEMANTIC, 
                        "`" + type->name = "` expects " + std::to_string(type->generics.size()) + " generics, but " + std::to_string(gens.size()) + " were passed in",
                        gensNode->toks.front(),
                        "", true);
                }

                for (size_t i = 0; i < type->generics.size(); i++) {
                    generics.insert({ type->generics[i]->genericName, gens[i] });
                }
            }

            rewrite_copy(generics, {});
        }
        TypeCall(const std::string &name) : Expr(nullptr), label(), genericName(), generics() {
            type = dynamic_cast<Type *>(Decl::getDecl(
                name,
                true,
                [&](Decl *decl) { return dynamic_cast<Type *>(decl) != nullptr; }
            ));

            if (!type)
                type = new Type(name, true);
        }
        TypeCall(Type *type, std::string label, std::vector<TypeCall *> gens, std::vector<std::string> refs, Node *node) : Expr(node), type(type), label(label), gens(gens), refs(refs) {
            for (size_t i = 0; i < type->generics.size(); i++) {
                generics.insert({ type->generics[i]->genericName, gens[i] });
            }
        }

        static TypeCall *createGeneric(std::string name) {
            TypeCall *thing = new TypeCall();
            thing->genericName = name;
            return thing;
        }

        bool matches(TypeCall *other) {
            if (!genericName.empty() || !other->genericName.empty()) return true;
            if (gens.size() != other->gens.size()) return false;

            if (type->name.size() > 0 && other->type->name.size() > 0 &&
                    ((type->name[0] == 'i' || type->name[0] == 'f') && std::to_string(std::stoi(type->name.substr(1))).size() == type->name.size() - 1) &&
                    ((other->type->name[0] == 'i' || other->type->name[0] == 'f') && std::to_string(std::stoi(other->type->name.substr(1))).size() == other->type->name.size() - 1)) {
                if (type->name[0] == 'i' && other->type->name[0] == 'i') return true;
                if (type->name[0] == 'f') return true;
            }

            for (size_t i = 0; i < gens.size(); i++) {
                if (!gens[i]->matches(other->gens[i]))
                    return false;
            }
            return type == other->type && label == other->label;
        }

        bool smatches(TypeCall *other) {
            if (!genericName.empty() || !other->genericName.empty()) return true;
            if (gens.size() != other->gens.size()) return false;

            if (type->name.size() > 0 && other->type->name.size() > 0 &&
                ((type->name[0] == 'i' || type->name[0] == 'f') && std::to_string(std::stoi(type->name.substr(1))).size() == type->name.size() - 1) &&
                ((other->type->name[0] == 'i' || other->type->name[0] == 'f') && std::to_string(std::stoi(other->type->name.substr(1))).size() == other->type->name.size() - 1)) {
                return type->name[0] == other->type->name[0];
            }

            for (size_t i = 0; i < gens.size(); i++) {
                if (!gens[i]->matches(other->gens[i]))
                    return false;
            }
            return type == other->type && label == other->label;
        }

        std::string print() const override {
            if (!genericName.empty())
                return genericName;
            std::string s = type->name;
            if (!label.empty())
                s += "[" + label + "]";
            s += printArray(gens, "<", ">", [&](TypeCall *tc) { return tc->print(); }, ", ", true);

            for (auto ref : refs) {
                if (ref == "ref")
                    s.insert(s.begin(), '&');
                else {
                    s.insert(s.begin(), '[');
                    s.push_back(']');
                }
            }

            return s;
        }

        llvm::Type *getType(LLVMInfo info) const override {
            llvm::Type *result = type->getType(info);
            for (std::string ref : refs)
                result = llvm::PointerType::get(result, 0);
            return result;
        }

        llvm::Value *codegen(LLVMInfo) override {
            return nullptr;
        }

        bool isGeneric() const { return !genericName.empty(); }

        ASTNode *rewrite_copy(std::map<std::string, TypeCall *> gens, std::map<std::string, Decl *> vars) {
            std::string check;
            for (auto [n, tc] : gens)
                check.append(tc->print()).append(", ");

            std::map<std::string, TypeCall *> genMapRewrite;
            for (auto [name, tc] : generics) {
                genMapRewrite.insert({ name, dynamic_cast<TypeCall *>(tc->rewrite(gens, {})) });
            }

            if (isGeneric())
                return gens[genericName];

            if (!generics.empty())
                type = dynamic_cast<Type *>(type->rewrite(genMapRewrite, vars));

            if (!label.empty()) 
                type = dynamic_cast<Label *>(dynamic_cast<Label *>(type->getMember(label))->rewrite(genMapRewrite, vars));
            label.clear();
            this->gens.clear();
            generics.clear();
            return this;
        }

        ASTNode *rewrite(std::map<std::string, TypeCall *> gens, std::map<std::string, Decl *> vars) override {
            return rewrite_copy(gens, vars);
        }

        void fillGenerics(TypeCall *other, std::map<std::string, TypeCall *> &result, Node *node) {
            if (isGeneric()) {
                if (result.find(genericName) != result.end() && !result[genericName]->matches(other)) {
                    hurl(SEMANTIC, "Cannot resolve template arguments, varying types for the same generic were passed in", node->toks.front(), "", true);
                }
                result[genericName] = other; // make this take the larger of 2 integers or floats
                return;
            }

            for (size_t i = 0; i < gens.size(); i++) 
                gens[i]->fillGenerics(other->gens[i], result, node);
        }

        bool containsGeneric() const {
            if (isGeneric()) return true;
            for (auto *gen : gens) {
                if (gen->containsGeneric())
                    return true;
            }
            return false;
        }

        bool isPrimitive() const {
            if (isGeneric()) return false;
            std::string name = type->name;
            return name == "b" || name == "c" || name == "bf" || 
                (name.size() > 0 && (name[0] == 'i' || name[0] == 'f') && std::to_string(std::stoi(name.substr(1))).size() == name.size() - 1);
        }
    };

    std::map<std::string, TypeCall *> TypeCall::genMap = {};

    Type::Type(Node *node) : Decl(node) {
        currentType = this;
        isPacked = node->value == "struct";

        if (auto *defMods = find("def_mods", node)) {
            if (defMods->children.empty())
                hurl(SYNTAX, "default mods are required for any type declarations", defMods->toks.front(), "type[pub] Type {}", true);
            for (auto *child : defMods->children)
                mods.push_back(child->value);
        } else {
            hurl(SYNTAX, "default mods are required for any type declarations", defMods->toks.front(), "type[pub] Type {}", true);
        }

        if (auto *name = find("name", node)) {
            Node *copy = name;
            name = name->children[0];
            bool isFull = true;

            if (copy->children.size() <= 1) {
                name = copy;
                isFull = false;
            }

            std::string stack = "";
            for (auto *child : name->children) {
                stack += (!stack.empty() ? "::" : "") + (find("datatype", child) ? child->children[0]->children[0]->value : child->children[0]->value);
                if (auto *lab = find("label", child)) {
                    stack += "[" + lab->children[0]->value + "]";
                }
            }

            if (auto *gens = find("gens", name->children.back())) {
                for (auto *gen : gens->children) {
                    // check if the type already exists later <future patch>
                    generics.push_back(TypeCall::createGeneric(gen->children[0]->value));
                    TypeCall::genMap[gen->children[0]->value] = generics.back();
                }
            }
            setName(stack, isFull, true);
        }

        if (auto *ext = find("ext", node)) {
            super = new TypeCall(ext->children[0]);
        }

        if (auto *impls = find("impl", node)) {
            for (auto *impl : impls->children)
                this->impls.push_back(new TypeCall(impl));
        }

        if (auto *members = find("members", node)) {
            for (auto *member : members->children) {
                std::vector<std::string> mods;
                for (auto *mod : find("mods", member)->children)
                    mods.push_back(mod->value);
                this->members.push_back({ mods, dynamic_cast<Decl *>(ASTNode::parse(member->children[1])) });
            }
        }

        Decl::removeLastScope();
        for (auto [a, b] : members) {
            Decl::decls[b->name].pop_back();
        }

        for (auto *gen : generics) {
            TypeCall::genMap.erase(gen->genericName);
        }

        currentType = nullptr;
    }

    std::string Type::print() const {
        std::string s = "type" + printArray(mods, "[", "]", [](std::string str) { return str; }, ", ", false) + " ";
        s.append(name).append(printArray(generics, "<", ">", [&](TypeCall *str) { return str->print(); }, ", ", true)).append(" ");
        if (super) 
            s.append("ext ").append(super->print()).append(" ");
        s.append(printArray(impls, "impl ", " ", [&](TypeCall *tc) { return tc->print(); }, ", ", true));
        s.append(printScope(members, [&](std::tuple<std::vector<std::string>, Decl *> thing) {
            std::string str;
            for (auto thing : std::get<0>(thing))
                str.append(thing).push_back(' ');
            str.append(std::get<1>(thing)->print());
            return str;
        }));
        return s;
    }

    ASTNode *Type::rewrite(std::map<std::string, TypeCall *> gens, std::map<std::string, Decl *> vars) {
        if (name == "void" || name == "b" || name == "bf" || name == "c")
            return nullptr;
        if (!name.empty() && (name[0] == 'i' || name[0] == 'f') && std::to_string(std::stoi(name.substr(1))).size() == name.size() - 1)
            return nullptr;

        std::string check;
        for (auto [n, tc] : gens)
            check.append(tc->print()).append(", ");

        if (rewrites.find(check) != rewrites.end())
            return rewrites.at(check);
        
        Type *copy = new Type(Decl::genName(), false);
        for (auto poss : queuePossibilities(name))
            vars[poss] = copy;

        copy->super = super ? dynamic_cast<TypeCall *>(super->rewrite(gens, vars)) : nullptr;
        copy->impls.reserve(impls.size());
        for (auto *impl : impls) 
            copy->impls.push_back(dynamic_cast<TypeCall *>(impl->rewrite(gens, vars)));

        copy->members.reserve(members.size());
        for (auto &[mods, item] : members) {
            copy->members.push_back({mods, dynamic_cast<Decl *>(item->rewrite(gens, vars))});
        }

        copy->isPacked = isPacked;
        rewrites.insert({ check, copy });

        codegen({ context, builder, __module, nullptr });
        return copy;
    }

    llvm::Type *Expr::getType(LLVMInfo info) const {
        return exprType->getType(info);
    }

}

#endif /* C604EB05_2F6A_4447_A719_2543D6DDB865 */

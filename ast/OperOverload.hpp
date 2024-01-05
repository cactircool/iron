#ifndef FF679963_BE75_469C_9D76_EF5D54565E71
#define FF679963_BE75_469C_9D76_EF5D54565E71

#include "Function.hpp"

namespace iron {

    struct OperOverload : public Function {
        std::string oper;

        static std::map<std::string, std::vector<OperOverload*>> overloads;

        OperOverload(Node *node) : Function(node, true) {
            setName(Decl::genName(), true, true);

            if (auto *gensNode = find("gens", node)) {
                for (auto *gen : gensNode->children) {
                    generics.push_back(TypeCall::createGeneric(gen->children[0]->value));
                    TypeCall::genMap[gen->children[0]->value] = generics.back();
                }
            }

            if (auto *mods = find("mods", node)) {
                isPure = find("pure", mods);
                isAsync = find("async", mods);
            }

            if (auto *dt = find("datatype", node)) {
                ret = new TypeCall(dt);
            }

            if (auto *paramsNode = find("params", node)) {
                for (auto *param : paramsNode->children) {
                    if (param->value == "this") {
                        params.push_back(new Variable(new TypeCall(Type::currentType, "", Type::currentType->generics, { "ref" }, param), param));
                    } else if (param->value == "variable") {
                        params.push_back(new Variable(param));
                    } else {
                        oper = paramsNode->children.size() == 3 ? param->value : (paramsNode->children[0] == param ? "l" + param->value : "r" + param->value);
                    }
                }
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

            if (overloads.find(oper) == overloads.end())
                overloads.insert({ oper, {} });
            overloads[oper].push_back(this);

            Decl::removeLastScope();

            for (auto *gen : generics) {
                TypeCall::genMap.erase(gen->genericName);
            }
        }

        std::string print() const override {
            std::string s = "oper" + printArray(generics, "<", ">", [&](TypeCall *tc) { return tc->print(); }, ", ", true);
            s += ":" + ret->print() + " ";
            if (isPure) s += "pure ";
            if (isAsync) s += "async ";

            s += oper + printArray(params, "(", ")", [&](Variable *var) { return var->print(); }, ", ") + " ";
            s += printScope(code, [&](ASTNode *node) { return node->print(); });
            return s;
        }

        static OperOverload *findOverload(std::string oper, std::vector<TypeCall *> params) {
            if (overloads.find(oper) == overloads.end())
                return nullptr;
            for (auto *overload : overloads[oper]) {
                if (overload->matches(params))
                    return overload;
            }
            return nullptr;
        }
    };

    std::map<std::string, std::vector<OperOverload *>> OperOverload::overloads = {};

}

#endif /* FF679963_BE75_469C_9D76_EF5D54565E71 */

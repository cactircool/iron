#ifndef A1DC0BC5_39AD_49EA_9F49_C68F726FF972
#define A1DC0BC5_39AD_49EA_9F49_C68F726FF972

#include "ASTNode.hpp"
#include <string>
#include <vector>
#include <map>
#include <random>

namespace iron {

    struct Decl : public ASTNode {
        static std::map<std::string, std::vector<Decl*>> decls; // nullptr = forward declaration
        static std::string scope;

        std::string name;

        Decl(Node *node) : ASTNode(node) {}

        bool nameMatches(std::string n) const {
            size_t index = name.find(n);
            if (index == std::string::npos)
                return false;
            return name.substr(index) == n;
        }

        void setName(std::string name, bool alreadyFull = false, bool append = false) {
            std::string full = alreadyFull ? name : (scope.empty() ? name : scope + "::" + name);
            auto list = queuePossibilities(full);

            for (auto &p : list) {
                if (decls.find(p) == decls.end())
                    decls.insert({p, {}});
                decls[p].push_back(this);
            }

            if (append)
                scope = full;

            this->name = full;
        }

        static void removeLastScope() {
            size_t index = scope.find_last_of(':');
            if (index == std::string::npos)
                scope.clear();
            else 
                scope = scope.substr(0, index - 1);
        }

        static std::vector<std::string> queuePossibilities(std::string scope) {
            std::vector<std::string> result{scope};
            
            size_t index = scope.find("::");
            while (index != std::string::npos) {
                scope.erase(0, index + 2);
                result.push_back(scope);
                index = scope.find("::");
            }

            if (scope != result.back())
                result.push_back(scope);

            return result;
        }

        static void removeOfScope(std::string scope) {
            std::vector<std::string> items;
            for (auto &[name, stack] : decls) {
                size_t index = name.find(scope);
                if (index != std::string::npos) {
                    
                    std::string match = name.substr(index + 2);
                    for (auto &[_name, _stack] : decls) {
                        if (_name == match)
                            items.push_back(_name);
                    }

                }
            }

            for (auto &item : items) {
                decls[item].clear();
                decls.erase(item);
            }
        }

        static Decl *getDecl(const std::string& name, bool includeScope = true, std::function<bool(Decl*)> checker = [](Decl *decl) { return true; }) {
            std::string full = includeScope ? (scope.empty() ? name : scope + "::" + name) : name;
            for (auto poss : queuePossibilities(full)) {
                if (decls.find(poss) != decls.end()) {
                    for (auto thing: decls[poss]) {
                        if (checker(thing))
                            return thing;
                    }
                }
            }

            return nullptr;
        }

        static std::string printId(Node *node) {
            std::string full;
            if (node->children.size() == 1)
                full += node->children[0]->value;
            else {
                for (auto *child : node->children) {
                    full += "::" + child->children[0]->value;
                }
                full = full.substr(2);
            }
            return full;
        }

        static std::string genName() {
            static auto& chrs = "0123456789"
                "abcdefghijklmnopqrstuvwxyz"
                "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

            thread_local static std::mt19937 rg{std::random_device{}()};
            thread_local static std::uniform_int_distribution<std::string::size_type> pick(0, sizeof(chrs) - 2);

            std::string s;
            while (s.empty() || decls.find(s) != decls.end()) {
                size_t length = 52;
                s.reserve(length);
                while (length--)
                    s += chrs[pick(rg)];
            }

            return s;
        }
    };

    std::string Decl::scope = "";
    std::map<std::string, std::vector<Decl*>> Decl::decls = {};

}

#endif /* A1DC0BC5_39AD_49EA_9F49_C68F726FF972 */

#ifndef DEB1320A_64D6_47CA_B609_B53B45505B98
#define DEB1320A_64D6_47CA_B609_B53B45505B98

#include <cassert>
#include <ostream>
#include <regex>
#include <string>
#include <memory>
#include "Compiler.hpp"
#include "Error.hpp"


std::ostream& operator<<(std::ostream& strm, std::vector<iron::Token> vec) {
    if (vec.empty()) return strm << "[]";

    strm << "[\n";
    for (auto &tok : vec) {
        strm << '\t' << tok.toString() << " : " << tok.count << " : " << tok.type << '\n';
    }
    return strm << ']';
}

namespace iron {
	int tabs = 0;

    struct Node {
        std::string value;
        std::vector<Node *> children;
        std::string line;
        std::vector<Token> toks;

        Node(std::string value, const std::vector<Node *> &children, const std::string &line, const std::vector<Token> &toks) : value(value), children(children), line(line), toks(toks) {}
        ~Node() {
            while (!children.empty()) {
                auto *back = children.back();
                children.pop_back();
                delete back;
            }
            children.clear();
        }

        void add(Node *node) {
            children.push_back(node);
        }

		std::string toString() const {
			std::string str;
			for (int i = 0; i < tabs; i++)
				str.append("    ");
			str.append(value).push_back('\n');
			tabs++;
			for (auto *child : children) {
                if (child)
				    str.append(child->toString());
			}
			tabs--;
			return str;
		}
    };

    #define leaf(x, y, ...) new Node(x, std::vector<Node *>{__VA_ARGS__}, line, y)
    #define carr const std::vector<Token> &
    #define arr std::vector<Token>
    #define cstr const std::string &
    #define str std::string

    Node *parse(const std::vector<Token> &toks, const std::string &line);

    size_t lowest(carr vec) {
        int count = 0;
        int low = INT_MIN;
        size_t index = std::string::npos;
        for (size_t i = 0; i < vec.size(); i++) {
            count += vec[i].scoper(true);
            if (count == 0 && vec[i].precedence() > low) {
                low = vec[i].precedence();
                index = i;
            }
        }
        return index;
    }

    size_t find(carr vec, const std::vector<TokenType> &options, std::vector<int> counts, bool dt = false) {
        int count = 0;
        for (size_t i = 0; i < vec.size(); i++) {
            count += vec[i].scoper(dt);
            size_t rel = count - vec.front().count;
            if (rel == std::string::npos) rel = 0;
            for (size_t j = 0; j < options.size(); j++) 
                if ((count == counts[j] || counts[j] == -1) && vec[i] == options[j])
                    return i;
        }
        return std::string::npos;
    }

    size_t contains(carr vec, std::function<bool(Token)> check = [](Token a) { return a.binary() || a.unary(); }) {
        for (size_t i = 0; i < vec.size(); ++i) 
            if (check(vec[i]))
                return i;
        return std::string::npos;
    }

    Node *expr(carr toks, cstr line);
    Node *type(carr toks, cstr line);

    Node *args(carr toks, cstr line, std::function<Node *(carr, cstr)> parser, std::string label = "args", std::vector<TokenType> separator = { _comma }, std::vector<int> counts = {0}, bool includeLast = false) {
        auto *node = leaf(label, toks);
        int count = 0;
        size_t start = 0;
        for (size_t i = 0; i < toks.size(); i++) {
            count += toks[i].scoper(true);
            auto a = std::find(counts.begin(), counts.end(), count);
            auto b = std::find(separator.begin(), separator.end(), toks[i].type);
            if (a != counts.end() && b != separator.end() && a[b - separator.begin()] == count) {
                std::vector<Token> vec{toks.begin() + start, toks.begin() + i};
                if (includeLast)
                    vec.push_back(toks[i]);
                node->add(parser(vec, line));
                start = i + 1;
            }
        }

        if (!toks.empty()) 
            node->add(parser({toks.begin() + start, toks.end()}, line));

        return node;
    }

    Node *call(carr toks, cstr line) {
        size_t pindex = find(toks, { _op, _obc, _obk }, { 1 }, true);
        if (pindex == std::string::npos)
            return nullptr;

        auto node = leaf("call", toks);

        if (contains(toks) == std::string::npos)
            node->add(leaf("name", toks, type(arr(toks.begin(), toks.begin() + pindex), line)));
        else 
            node->add(leaf("name", toks, expr(arr(toks.begin(), toks.begin() + pindex), line)));

        std::vector<Token> vec{toks.begin() + pindex + 1, toks.end() - (toks.back() == _scolon ? 2 : 1)};

        node->add(args(vec, line, expr));
        node->children.back()->children.insert(node->children.back()->children.begin(), leaf(toks[pindex].value, toks));
        return node;
    }

    Node *type(carr toks, cstr line) {
        if (toks.front() == _dref || toks.front() == _ref) {
            return leaf("dref", toks, type({ toks.begin() + 1, toks.end() }, line));
        }

        if ((toks.front() == _dobk || toks.front() == _obk) && (toks.back() == _dcbk || toks.back() == _cbk)) {
            return leaf("array", toks, type({ toks.begin() + 1, toks.end() - 1 }, line));
        }

        Node *dt = leaf("datatype", toks);
        if (find(toks, { _scope }, { 0 }, true) != std::string::npos) {
            auto *scopes = args(toks, line, type, "scopes", {_scope});
            for (auto *child : scopes->children)
                dt->add(child);
            return dt;
        }

        size_t id_index = find(toks, { _dobk, _doa }, { 1, 1 }, true);
        if (id_index == std::string::npos) {
            dt->add(leaf(toks.front().value, toks));
            return dt;
        }

        dt->add(type({toks.begin(), toks.begin() + id_index}, line));
        if (toks[id_index] == _dobk) {
            dt->add(leaf("label", toks, new Node(toks[id_index + 1].value, {}, line, {toks.begin() + id_index + 1, toks.end()})));
        }

        size_t gen_index = find(toks, { _doa }, { 0 });
        if (gen_index != std::string::npos) {
            auto *gens = args({toks.begin() + gen_index + 1, toks.end() - 1}, line, type);
            gens->value = "gens";
            dt->add(gens);
        }

        return dt;
    }

    Node *expr(carr toks, cstr line) {
        size_t index = lowest(toks);
        if (index == std::string::npos) {
            // parse type/call/literal/singular id
            if (auto *n = call(toks, line))
                return n;

            // datatype/literal/singular id
            if (toks.size() == 1) {
                if (toks[0].literal()) {
                    switch (toks[0].type) {
                        case _int: return leaf("int_literal", toks, leaf(toks[0].value, toks));
                        case _float: return leaf("float_literal", toks, leaf(toks[0].value, toks));
                        case _char: return leaf("char_literal", toks, leaf(toks[0].value, toks));
                        case _str: return leaf("str_literal", toks, leaf(toks[0].value, toks));
                        case _bool: return leaf("bool_literal", toks, leaf(toks[0].value, toks));
                        default: break;
                    }
                } else if (toks[0] == _id) {
                    return type(toks, line);
                }
            }

            return type(toks, line);
        }

        if (toks[index].unary()) {
            if (index == 0 || toks[index - 1].binary() || toks[index - 1].unary() || toks[index - 1].scoper() == 1)
                return leaf("left_unary_expr", arr(toks.begin() + index, toks.end()), 
                        leaf(toks[index].value, arr(toks.begin() + index, toks.end())),
                        expr(arr(toks.begin() + index + 1, toks.end()), line));
            return leaf("right_unary_expr", arr(toks.begin() + index, toks.end()),
                    expr(arr(toks.begin(), toks.begin() + index), line),
                    leaf(toks[index].value, arr(toks.begin() + index, toks.end())));
        }

        return leaf("binary_expr", toks, leaf(toks[index].value, toks,
                expr(arr(toks.begin(), toks.begin() + index), line), 
                expr(arr(toks.begin() + index + 1, toks.end()), line)));
    }

	Node *var(carr toks, cstr line) {
		auto *node = leaf("variable", toks);

        auto *mods = leaf("mods", toks);
        size_t i = 1;
        while (toks[i] == _vol || toks[i] == _const) {
            mods->add(leaf(toks[i].value, arr(toks.begin() + i, toks.end())));
            i++;
        }
        node->add(mods);

		size_t colonIndex = find(toks, { _colon, _eq }, { 0, 0 }, true);
		if (colonIndex != std::string::npos) 
			node->add(leaf("name", arr(toks.begin() + i, toks.end()), type({toks.begin() + 1, toks.begin() + colonIndex}, line)));
		else {
		 	hurl(
				SYNTAX,
				"expected a colon to denote the type of the variable or an expression following an = operator to automatically determine the type",
				toks.front(),
				"let x:i32 = 42;\n    let x = 42;",
                true
			);
        }

		size_t typeEndIndex = find(toks, { _eq }, { 0 }, true);
		if (colonIndex != std::string::npos && toks[colonIndex] == _colon) {
            std::vector<Token> vec = arr(toks.begin() + colonIndex + 1,
                             typeEndIndex != std::string::npos ? toks.begin() + typeEndIndex : toks.end());
            node->add(leaf("type", vec, type(vec, line)));
        }

		if (typeEndIndex != std::string::npos)
			node->add(leaf("value", arr(toks.begin() + typeEndIndex + 1, toks.end()), expr({toks.begin() + typeEndIndex + 1, toks.end() - 1}, line)));

		return node;
	}

    Node *explicit_type(carr toks, cstr line) {
        if (toks.front() == _dref || toks.front() == _ref) {
            return leaf("dref", toks, explicit_type({ toks.begin() + 1, toks.end() }, line));
        }

        if ((toks.front() == _dobk || toks.front() == _obk) && (toks.back() == _dcbk || toks.back() == _cbk)) {
            return leaf("array", toks, explicit_type({ toks.begin() + 1, toks.end() - 1 }, line));
        }

        Node *dt = leaf("datatype", toks);

        if (toks.size() == 2 && toks.front() == _id && toks.back() == _spread) {
            dt->add(leaf(toks.front().value, toks));
            dt->add(leaf("...", toks));
            return dt;
        }

        if (find(toks, { _scope }, { 0 }, true) != std::string::npos) {
            auto *scopes = args(toks, line, explicit_type, "scopes", {_scope});
            for (auto *child : scopes->children)
                dt->add(child);
            return dt;
        }

        size_t id_index = find(toks, { _dobk, _doa }, { 1, 1 }, true);
        if (id_index == std::string::npos) {
            dt->add(leaf(toks.front().value, toks));
            return dt;
        }

        dt->add(type({toks.begin(), toks.begin() + id_index}, line));
        if (toks[id_index] == _dobk) {
            dt->add(leaf("label", toks, new Node(toks[id_index + 1].value, {}, line, {toks.begin() + id_index + 1, toks.end()})));
        }

        size_t gen_index = find(toks, { _doa }, { 0 });
        if (gen_index != std::string::npos) {
            auto *gens = args({toks.begin() + gen_index + 1, toks.end() - 1}, line, explicit_type);
            gens->value = "gens";
            dt->add(gens);
        }

        return dt;
    }

    Node *structure(carr toks, cstr line) {
        Node *node = leaf(toks.front().value, toks);

        size_t defModEndIndex = 0;
        if (toks.front() == _type) {
            defModEndIndex = find(toks, { _cbk, _dcbk }, { 0, 0 }, true);
            if (defModEndIndex != std::string::npos) {
                auto *defMods = args(
                    {toks.begin() + 2, toks.begin() + defModEndIndex}, 
                    line, 
                    [&](carr a, cstr l) { return leaf(a[0].value, a); }, 
                    "def_mods");
                node->add(defMods);
            }
        } else if (toks.front() == _class) {
            node->add(leaf("def_mods", toks, leaf("priv", toks)));
        } else if (toks.front() == _struct) {
            node->add(leaf("def_mods", toks, leaf("pub", toks)));
        } else if (toks.front() == _trait) {
            node->add(leaf("def_mods", toks, leaf("pub", toks), leaf("fin", toks)));
        }

        // make type make the rest of the things till the obrace be doa shit

        size_t nameEndIndex = find(toks, { _ext, _impl, _obc, _scolon }, { 0, 0, 1, 0 }, true);
        if (nameEndIndex != std::string::npos) {
            auto *name = leaf(
                "name", 
                arr(toks.begin() + defModEndIndex + 1, toks.begin() + nameEndIndex), 
                explicit_type({toks.begin() + defModEndIndex + 1, toks.begin() + nameEndIndex}, line)
            );
            node->add(name);
        }

        if (toks[nameEndIndex] == _ext) {
            size_t start = nameEndIndex;
            nameEndIndex = find(toks, { _impl, _obc, _scolon }, { 0, 1, 0 }, true);
            if (nameEndIndex != std::string::npos) {
                std::vector<Token> vec{toks.begin() + start + 1, toks.begin() + nameEndIndex};
                auto *ext = explicit_type(vec, line);
                node->add(leaf("ext", vec, ext));
            }
        }

        if (toks[nameEndIndex] == _impl) {
            size_t start = nameEndIndex;
            nameEndIndex = find(toks, { _obc, _scolon }, { 1, 0 }, true);
            if (nameEndIndex != std::string::npos) {
                auto *impl = args({toks.begin() + start + 1, toks.begin() + nameEndIndex}, line, type, "impl");
                node->add(impl);
            }
        }

        if (toks[nameEndIndex] == _obc) {
            auto *scope = args({toks.begin() + nameEndIndex + 1, toks.end() - 1}, line, [&](carr vec, cstr line) {
                if (vec.empty()) return leaf("", vec);
                auto *node = leaf("member", vec);
                auto *mods = leaf("mods", vec);
                size_t i;
                for (i = 0; vec[i].mod(); i++) 
                    mods->add(leaf(vec[i].value, arr(vec.begin() + i, vec.end())));

                node->add(mods);
                node->add(parse({vec.begin() + i, vec.end()}, line));
                return node;
            }, "members", {_scolon, _cbc}, {0, 0}, true);

            if (!scope->children.empty() && scope->children.back()->value.empty()) {
                auto *last = scope->children.back();
                scope->children.pop_back();
                delete last;
            }

            node->add(scope);
            return node;
        }

        // scolon, so it's a forward declaration
        node->value = "fwd_type";
        return node;
    }

    Node *function(carr toks, cstr line) {
        auto *node = leaf("function", toks);
        size_t i = 1;
        if (toks[i] == _doa) {
            size_t index = find(toks, { _dca }, { 0 }, true);
            node->add(args({ toks.begin() + i + 1, toks.begin() + index }, line, type, "gens"));
            i = index + 1;
        }

        auto *mods = leaf("mods", toks);
        while (toks[i] == _pure || toks[i] == _impure || toks[i] == _async) {
            mods->add(leaf(toks[i].value, arr(toks.begin() + i, toks.end())));
            i++;
        }
        node->add(mods);

        size_t index = find(toks, { _colon }, { 0 }, true);
        if (index != std::string::npos) {
            node->add(leaf(
                "name",
                arr(toks.begin() + i, toks.end()),
                type({toks.begin() + i, toks.begin() + index}, line)
            ));
        }

        i = index + 1;
        index = find(toks, { _op }, { 1 }, true);
        if (index != std::string::npos) {
            node->add(type({ toks.begin() + i, toks.begin() + index }, line));
        }

        i = index + 1;
        index = find(toks, { _cp }, { 0 }, true);
        if (index != std::string::npos) {
            node->add(
                args({toks.begin() + i, toks.begin() + index}, line, var, "params")
            );
        }

        if (toks[index + 1] == _hurls) {
            i = index + 2;
            index = find(toks, { _obc }, { 1 }, true);
            node->add(
                args({toks.begin() + i, toks.begin() + index}, line, type, "hurls")
            );
            index--;
        }

        i = index + 2;
        index = find(toks, { _cbc }, { 0 }, true);
        if (index != std::string::npos) {
            node->add(
                args({toks.begin() + i, toks.begin() + index}, line, parse, "scope", { _scolon }, { 0 }, true)
            );
        }

        return node;
    }

    Node *oper_overload(carr toks, cstr line) {
        /*
            oper<A, B>:int pure async this + let y:int {}
        */

        auto *node = leaf("oper_overload", toks);
        size_t index = 2;

        if (toks[1] == _doa) {
            size_t i = find(toks, { _dca }, { 0 }, true);
            node->add(
                args({ toks.begin() + 2, toks.begin() + i }, line, type, "gens")
            );
            index = i + 2;
        }

        size_t index_tmp = index + 1 + find({ toks.begin() + index + 1, toks.end() }, { _pure, _impure, _async, _let, _id }, { 0, 0, 0, 0, 0 }, true);
        node->add(type({ toks.begin() + index, toks.begin() + index_tmp }, line));

        index = index_tmp;

        auto *mods = leaf("mods", arr( toks.begin() + index, toks.end() ));
        while (toks[index] == _pure || toks[index] == _impure || toks[index] == _async)
            mods->add(leaf(toks[index].value, arr( toks.begin() + (index++), toks.end() )));
        node->add(mods);

        auto *params = leaf("params", arr( toks.begin() + index, toks.end() ));

        while (true) {
            if (toks[index] == _obc || toks[index] == _dobc || toks[index] == _hurls)
                break;
            else if (toks[index] == _let) {
                size_t it = index + 1;
                for (size_t i = index + 1; i < toks.size(); i++) {
                    if ((toks[i].binary() && toks[i] != _colon) || toks[i].unary() || toks[i] == _obc || toks[i] == _dobc || toks[i] == _hurls) {
                        it = i;
                        break;
                    }
                }

                params->add(var({ toks.begin() + index, toks.begin() + it }, line));
                index = it - 1;
            }
            else if (toks[index].binary() || toks[index].unary())
                params->add(leaf(toks[index].value, arr(toks.begin() + index, toks.end())));
            else if (toks[index].value == "this")
                params->add(leaf("this", arr(toks.begin() + index, toks.end())));
            index++;
        }

        node->add(params);

        if (toks[index] == _hurls) {
            size_t end = find(toks, { _obc, _dobc }, { 1, 1 }, true);
            node->add(args({ toks.begin() + index + 1, toks.begin() + end }, line, type, "hurls"));
        }

        node->add(
            args({ toks.begin() + index + 1, toks.end() - 1 }, line, parse, "scope", { _scolon }, { 0 }, true)
        );

        return node;
    }

    #undef carr
    #undef arr
    #undef cstr
    #undef str

    bool oper_header = false;

    Node *parse(const std::vector<Token> &toks, const std::string &line) {
        if (!oper_header && toks.size() >= 2 && toks.back() == _id && toks[toks.size() - 2] == _id) {
			hurl(
				SYNTAX, 
				"expected an operator between two identifiers, did you mean: \033[36m" + toks[toks.size() - 2].value + "::" + toks.back().value + "\033[0m?", 
				toks.back(),
				"",
				true);
		}

        if(toks.size() <= 0) return nullptr;

        if (toks.size() == 1 && toks.front() == _oper) oper_header = true;
        if (oper_header && (toks.front() == _obc || toks.front() == _dobc || toks.front() == _scolon)) oper_header = false;

        if (toks.front() == _let && toks.back() == _scolon && toks.back().count == toks.front().count) {
			return var(toks, line);
        }
        if ((toks.front() == _type || toks.front() == _class || toks.front() == _struct || toks.front() == _trait || toks.front() == _label) && ((toks.back() == _cbc || toks.back() == _scolon) && toks.back().count == toks.front().count)) {
            return structure(toks, line);
        }
        if (toks.front() == _func && toks.back() == _cbc && toks.back().count == toks.front().count) {
            return function(toks, line);
        }
        if (toks.front() == _ret && toks.back() == _scolon && toks.back().count == toks.front().count) {
            return leaf("ret", toks, expr({toks.begin() + 1, toks.end() - 1}, line));
        }
        if (toks.front() == _oper && (toks.back() == _cbc || toks.back() == _dcbc)) {
            return oper_overload(toks, line);
        }
        if (toks.back() == _scolon && toks.back().count == toks.front().count) {
            return expr(toks, line);
        }
        return nullptr;
    }

	NodePrefixStruct create(const std::vector<Token> &toks, const std::string &line);

    Node *find(std::string str, Node *node) {
        for (auto *child : node->children) 
            if (child && child->value == str)
                return child;
        return nullptr;
    }

    #undef leaf

}

#endif /* DEB1320A_64D6_47CA_B609_B53B45505B98 */

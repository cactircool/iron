#ifndef C135D9C5_AE01_454A_866F_F6FDC1BDABEB
#define C135D9C5_AE01_454A_866F_F6FDC1BDABEB

#include <iostream>
#include <string>
#include <cstring>
#include <numeric>
#include <algorithm>
#include <tuple>
#include <unistd.h>

namespace iron {

    std::string trim(std::string aString) {
        return aString.empty() ? "" : aString.substr(aString.find_first_not_of(' '), (aString.find_last_not_of(' ') - aString.find_first_not_of(' ')) + 1);
    }

    enum TokenType {
        _unknown,

        _binop_start,

        _bin_basic_start,
        _mul = 0,
        _div,
        _mod,
        _add,
        _sub,
        _ls,
        _rs,
        _band,
        _bxor,
        _bor,
        _ternary,
        _in,
        _range,
        _rangeeq,
        _colon,
        _bin_basic_end,

        _bin_bool_start = _bin_basic_end - 1,
        _tlt,
        _tgt,
        _tlteq,
        _tgteq,
        _teq,
        _tneq,
        _tand,
        _tor,
        _bin_bool_end,

        _bin_assign_start = _bin_bool_end - 1,
        _eq,
        _addeq,
        _subeq,
        _modeq,
        _muleq,
        _diveq,
        _lseq,
        _rseq,
        _bandeq,
        _boreq,
        _bxoreq,
        _bin_assign_end,

        _binop_end,

        _unop_start,

        _un_basic_start = _bin_assign_end - 1,
        _ref,
        _deref,
        _rinc,
        _rdec,
        _linc,
        _ldec,
        _plus,
        _minus,
        _bnot,
        _await,
        _build,
        _alloc,
        _free,
        _hurl,
        _spread,
        _yield,
        _size, // $
        _un_basic_end,

        _un_bool_start = _un_basic_end - 1,
        _tnot,
        _un_bool_end,

        _unop_end,

        _key_start = _un_bool_end - 1,

        _let,
        _class,
        _struct,
        _type,
        _trait,
        _func,
        _pub,
        _priv,
        _prot,
        _fin,
        _abstr,
        _stat,
        _label,
        _const,
        _default,
        _enum,
        _module,
        _exp,
        _ext,
        _impl,
        _templ,
        _union,
        _oper,
        _space,
        _pure,
        _impure,
        _async,
        _hurls,
        _vol,

        _key_end,

        _cmd_start = _key_end - 1,

        _break,
        _continue,
        _if,
        _while,
        _switch,
        _for,
        _else,
        _try,
        _catch,
        _dile,
        _ret,
        _imp,
        _link,
        _macro, // single macro with replacement or macro function
        _use,
        _unuse,

        _cmd_end,

        _lit_start = _cmd_end - 1,

        _id,
        _int,
        _float,
        _str,
        _char,
        _bool,

        _lit_end,

        _symbol_start = _lit_end - 1,

        _dot,
        _scope,
        _comma,
        _op,
        _cp,
        _obk,
        _cbk,
        _obc,
        _cbc,
        _scolon,
        _arrow,

        _symbol_end,

        _dt_symbol_start = _symbol_end - 1,

        _doa,
        _dca,
        _dobk,
        _dcbk,
        _dobc,
        _dcbc,
        _dref,

        _dt_symbol_end,
    };

#define cf(flag) { return t < _ ## flag ## _end && t > _ ## flag ## _start; }

    inline bool isBinary(TokenType t) cf(binop)
    inline bool isBooleanBinary(TokenType t) cf(bin_bool)
    inline bool isBasicBinary(TokenType t) cf(bin_basic)
    inline bool isAssignBinary(TokenType t) cf(bin_assign)

    inline bool isUnary(TokenType t) cf(unop)
    inline bool isBasicUnary(TokenType t) cf(un_basic)
    inline bool isBooleanUnary(TokenType t) cf(un_bool)

    inline bool isKeyword(TokenType t) cf(key)

    inline bool isCommand(TokenType t) cf(cmd)

    inline bool isLiteral(TokenType t) cf(lit)

    inline bool isSymbol(TokenType t) cf(symbol)

    inline bool isDataTypeSymbol(TokenType t) cf(dt_symbol)

    inline int scoper(TokenType t, bool dt = false) {
        switch (t) {
            case _op:
            case _obk:
            case _obc:
                return 1;
            case _cp:
            case _cbk:
            case _cbc:
                return -1;
            default: {
                if (dt) {
                    switch (t) {
                        case _dobk:
                        case _dobc:
                        case _doa:
                            return 1;
                        case _dcbk:
                        case _dcbc:
                        case _dca:
                            return -1;
                        default:
                            return 0;
                    }
                }
                return 0;
            }
        }
    }

#undef cf

    // boolean flags that tell of the context of the token passed into the lex method
    struct TokenContext {
        bool in_colon;
        bool type_header;
        bool func_header;
        int func_header_count;
        TokenType last;

        void reset() {
            in_colon = false;
            type_header = false;
            func_header = false;
            func_header_count = 0;
        }
    };

    std::vector<std::tuple<std::string, int>> tokens;

    std::string toString(TokenType t) {
        for (auto &[str, id] : tokens) 
            if (id == t) return str;
        return "";
    }

    std::tuple<TokenType, std::string> lex(std::string &text, TokenContext &context) {
        if (tokens.empty()) {
            std::string _toks = "*,/,%,+,-,<<,>>,&,^,|,?,in,..,..=,:,<,>,<=,>=,==,!=,&&,||,=,+=,-=,%=,*=,/=,<<=,>>=,&=,|=,^=,&,*,++,--,++,--,+,-,~,await,build,alloc,free,hurl,...,yield,$,!,let,class,struct,type,trait,func,pub,priv,prot,fin,abstr,stat,label,const,default,enum,module,exp,ext,impl,templ,union,oper,space,pure,impure,async,hurls,vol,break,continue,if,while,switch,for,else,try,catch,dile,ret,imp,link,macro,use,unuse,`,`,`,`,`,`,.,::,(,),[,],{,},;,->";
            _toks.erase(std::remove_if(_toks.begin(), _toks.end(), isspace));
            char *__toks = _toks.data();
            char *ptr = std::strtok(__toks, ",");

            std::vector<std::string> toks;
            while (ptr != nullptr) {
                toks.push_back(ptr);
                ptr = std::strtok(nullptr, ",");
            }

            toks.insert(toks.begin() + _comma, ",");

            std::vector<int> indexes(toks.size());
            std::iota(indexes.begin(), indexes.end(), 0);
            std::stable_sort(indexes.begin(), indexes.end(), [&toks](int i1, int i2) {return toks[i1] > toks[i2];});

            tokens.reserve(indexes.size());
            for (auto index : indexes) 
                tokens.push_back({ toks[index], index });
        }

        TokenType result;
        std::string resultString;

        for (auto &[tok, index] : tokens) {
            if (text.size() >= tok.size() && text.substr(0, tok.size()) == tok) {
                result = static_cast<TokenType>(index);
                text.erase(0, tok.size());
                resultString = tok;
                break;
            }
        }

        // it is a literal
        if (trim(resultString) == "`" || trim(resultString).empty()) {
            resultString.clear();
            // b, o, x, t
            if (text[0] == '0' && text[1] == 'b') {
                resultString.append(text.substr(0, 2));
                text.erase(0, 2);
                while (text[0] == '0' || text[0] == '1') {
                    resultString.push_back(text[0]);
                    text.erase(0, 1);
                }
                result = _int;
            }
            else if (text[0] == '0' && text[1] == 'o') {
                resultString.append(text.substr(0, 2));
                text.erase(0, 2);
                while (text[0] >= '0' && text[0] <= '7') {
                    resultString.push_back(text[0]);
                    text.erase(0, 1);
                }
                result = _int;
            }
            else if (text[0] == '0' && text[1] == 'x') {
                resultString.append(text.substr(0, 2));
                text.erase(0, 2);
                while (isdigit(text[0]) || (text[0] >= 'a' && text[0] <= 'f') || (text[0] >= 'A' && text[0] <= 'F')) {
                    resultString.push_back(text[0]);
                    text.erase(0, 1);
                }
                result = _int;
            }
            else if (text[0] == '0' && text[1] == 't') {
                resultString.append(text.substr(0, 2));
                text.erase(0, 2);
                while (isdigit(text[0]) || (text[0] >= 'a' && text[0] <= 'z') || (text[0] >= 'A' && text[0] <= 'Z')) {
                    resultString.push_back(text[0]);
                    text.erase(0, 1);
                }
                result = _int;
            }
            else if (isdigit(text[0]) || text[0] == '.') {
                bool dec = text[0] == '.';
                resultString.push_back(text[0]);
                text.erase(0, 1);

                if (dec && !isdigit(text[1]))
                    result = _dot;

                while (isdigit(text[0]) || (text[0] == '.' && !dec)) {
                    if (text[0] == '.')
                        dec = true;
                    resultString.push_back(text[0]);
                    text.erase(0, 1);
                }

                if (dec)
                    result = _float;
                else
                    result = _int;
            }
            else if (text[0] == '"' || text[0] == '\'') {
                resultString.push_back(text[0]);
                char start = text[0];

                text.erase(0, 1);
                bool slash = false;

                while (text[0] != start || slash) {
                    slash = text[0] == '\\';
                    resultString.push_back(text[0]);
                    text.erase(0, 1);
                }

                resultString.push_back(start);
                text.erase(0, 1);

                if (start == '"' || resultString.size() > 3)
                    result = _str;
                else
                    result = _char;
            }
            else if (isalpha(text[0]) || text[0] == '_') {
                while (std::isalnum(text[0]) || text[0] == '_') {
                    resultString.push_back(text[0]);
                    text.erase(0, 1);
                }

                if (resultString == "true" || resultString == "false")
                    result = _bool;
                else
                    result = _id;
            }
        }
        else if (resultString.size() == 1 && resultString[0] == '.' && isdigit(text[0])) {
            while (isdigit(text[0])) {
                resultString.push_back(text[0]);
                text.erase(0, 1);
            }

            result = _float;
        }
        else if (isalpha(text[0]) && (isKeyword(result) || result == _in)) {
            while (isalpha(text[0])) {
                resultString.push_back(text[0]);
                text.erase(0, 1);
            }
            if (resultString == "true" || resultString == "false")
                result = _bool;
            else
                result = _id;
        }

        if (result == _colon) context.in_colon = true;
        else if (result == _type || result == _class || result == _struct || result == _trait) {
            context.in_colon = true;
            context.type_header = true;
        }
        else if (result == _func || result == _oper) {
            context.func_header = true;
            context.in_colon = true;
        }
        else if ((result == _obc || result == _scolon) && context.type_header) context.reset();
        else if (result == _add && (isBinary(context.last) || scoper(context.last) == 1)) result = _plus;
        else if (result == _sub && (isBinary(context.last) || scoper(context.last) == 1)) result = _minus;
        else if (result == _mul && (isBinary(context.last) || scoper(context.last) == 1)) result = _deref;
        else if (result == _band && (isBinary(context.last) || scoper(context.last) == 1)) result = _ref;
        else if (result == _rinc && (isBinary(context.last) || scoper(context.last) == 1)) result = _linc;
        else if (result == _rdec && (isBinary(context.last) || scoper(context.last) == 1)) result = _ldec;
        else if (result == _linc && !(isBinary(context.last) || scoper(context.last) == 1)) result = _rinc;
        else if (result == _ldec && !(isBinary(context.last) || scoper(context.last) == 1)) result = _rdec;
        else if (context.func_header && result == _obc && context.func_header_count == 0) context.reset();
        else if (context.in_colon && result == _obk) result = _dobk;
        else if (context.in_colon && result == _cbk) result = _dcbk;
        else if (context.in_colon && result == _obc) result = _dobc;
        else if (context.in_colon && result == _cbc) result = _dcbc;
        else if (context.in_colon && result == _tlt) result = _doa;
        else if (context.in_colon && result == _tgt) result = _dca;
        else if (context.in_colon && result == _rs) {
            result = _dca;
            resultString = ">";
            text.insert(0, ">");
        }
        else if (context.in_colon && result == _rseq) {
            result = _dca;
            resultString = ">";
            text.insert(0, ">=");
        }
        else if (context.in_colon && result == _tgteq) {
            result = _dca;
            text.insert(0, "=");
        }
        else if ((context.in_colon && !context.type_header && !context.func_header) && (isBinary(result) || isUnary(result) || isKeyword(result) || isCommand(result) || (isLiteral(result) && result != _id) || isSymbol(result))) context.reset();

        if (context.func_header)
            context.func_header_count += scoper(result, true);

        context.last = result;
        return {result, resultString};
    }

}

#endif /* C135D9C5_AE01_454A_866F_F6FDC1BDABEB */

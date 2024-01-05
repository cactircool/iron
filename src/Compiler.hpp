#ifndef EA685BB7_AB95_4840_8A50_27B75FB076EC
#define EA685BB7_AB95_4840_8A50_27B75FB076EC

#include <iostream>
#include <string>
#include <fstream>
#include <memory>
#include <sstream>
#include <unistd.h>
#include <ranges>

#include "TokenType.hpp"

namespace iron {

    struct ASTNode;

    class Program {
    public:
        static std::vector<ASTNode *> program;
    };

    std::vector<ASTNode *> Program::program = {};

    struct Token {
        static size_t currentLineNum;
        static size_t currentLinePos;
        static std::vector<Token> currentTokens;
        static std::string currentLine;

        TokenType type;
        std::string value;
        size_t line;
        size_t pos;
        size_t count = 0;

        Token(TokenType type) : type(type), value(iron::toString(type)) {}
        Token(TokenType type, std::string value) : type(type), value(value) {}

        bool operator==(TokenType t) const {
            return type == t;
        }
        bool operator==(std::string str) const {
            return value == str;
        }
        bool operator==(Token t) const {
            return t.type == type && t.value == value;
        }

        int scoper(bool dt = false) const {
            return iron::scoper(type, dt);
        }
        int precedence() const {
            #define c(x) case _ ## x:
            switch (type) {
                c(rinc)
                c(rdec)
                c(spread)
                c(dot)
                    return 1;
                c(linc)
                c(ldec)
                c(plus)
                c(minus)
                c(tnot)
                c(bnot)
                c(deref)
                c(ref)
                c(size)
                c(await)
                c(build)
                c(alloc)
                c(free)
                c(colon)
                    return 2;
                c(mul)
                c(div)
                c(mod)
                    return 3;
                c(add)
                c(sub)
                    return 4;
                c(ls)
                c(rs)
                    return 5;
                c(tlt)
                c(tgt)
                c(tlteq)
                c(tgteq)
                    return 6;
                c(teq)
                c(tneq)
                    return 7;
                c(band) return 8;
                c(bxor) return 9;
                c(bor) return 10;
                c(tand) return 11;
                c(tor) return 12;
                c(range)
                c(rangeeq)
                    return 13;
                c(ternary)
                c(hurl)
                c(yield)
                c(eq)
                c(addeq)
                c(subeq)
                c(muleq)
                c(diveq)
                c(modeq)
                c(bandeq)
                c(boreq)
                c(bxoreq)
                c(lseq)
                c(rseq)
                    return 14;
                default:
                    return INT_MIN;
            }
            #undef c
        }

        bool unary() const { return isUnary(type); }
        bool binary() const { return isBinary(type); }
        bool literal() const { return isLiteral(type); }
        bool mod() const {
            switch (type) {
                case _pub:
                case _priv:
                case _prot:
                case _fin:
                case _stat:
                case _abstr:
                    return true;
                default:
                    return false;
            }
        }

        std::string toString() const {
            return value;
        }
    };

    std::vector<Token> Token::currentTokens = {};
    std::string Token::currentLine = "";
    size_t Token::currentLineNum = 1;
    size_t Token::currentLinePos = 1;

    struct NodePrefixStruct {
        ASTNode *node;
        std::vector<Token> prefix;
    };

    NodePrefixStruct create(const std::vector<Token> &toks, const std::string &line);

    void consume(std::string &text) {
        while (isspace(text[0]) || (text.size() > 2 && (text.substr(0, 2) == "/*" || text.substr(0, 2) == "//"))) {
            if (isspace(text[0])) 
                while (isspace(text[0])) {
                    Token::currentLine += text[0];
                    text.erase(0, 1);
                }
            else if (text.substr(0, 2) == "/*") {
                text.erase(0, 2);
                while (text.size() > 2 && text.substr(0, 2) != "*/") text.erase(0, 1);
                if (!text.empty()) text.erase(0, 2);
            }
            else {
                while (text[0] != '\n') text.erase(0, 1);
                text.erase(0, 1);
            }
        }

        TokenContext ctx;
        auto [tok, txt] = lex(text, ctx);

        Token token{tok, txt};
        token.count = Token::currentTokens.empty() ? token.scoper(true) : Token::currentTokens.back().count + token.scoper(true);

        Token::currentTokens.push_back(token);

        size_t lastNewLine = Token::currentLine.find_last_of('\n');
        Token::currentLinePos = lastNewLine == std::string::npos ? Token::currentLine.size() : Token::currentLine.substr(lastNewLine).size();
        Token::currentLine += txt;
        Token::currentLineNum += std::ranges::count(txt, '\n');

        Token::currentTokens.back().line = Token::currentLineNum;
        Token::currentTokens.back().pos = Token::currentLinePos;

        NodePrefixStruct strct = create(Token::currentTokens, Token::currentLine);
        if (strct.node) {
            Token::currentTokens = strct.prefix;
            Program::program.push_back(strct.node);
        }
    }

    std::string sourceName;

    void compile(const std::string &filename) {
        std::ifstream file(filename);
        std::stringstream ss;
        ss << file.rdbuf();
        std::string text = ss.str();

        sourceName = filename;

        while (!text.empty()) {
            consume(text);
        }
    }

}

#endif /* EA685BB7_AB95_4840_8A50_27B75FB076EC */

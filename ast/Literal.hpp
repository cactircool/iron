#ifndef BA2C7946_7599_478F_A80F_58E37E1719B8
#define BA2C7946_7599_478F_A80F_58E37E1719B8

#include "Expr.hpp"
#include "TypeCall.hpp"
#include "boost/multiprecision/fwd.hpp"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"

#include <algorithm>
#include <numeric>
#include <cctype>
#include <boost/multiprecision/cpp_int.hpp>
#include <boost/multiprecision/cpp_dec_float.hpp>
#include <boost/multiprecision/cpp_bin_float.hpp>

namespace mp = boost::multiprecision;

namespace iron {

    struct Literal : public Expr {
        std::string value;
        int radix;

        Literal(Node *node) : Expr(node), value(node->children[0]->value) {
            std::string name = node->value;
            value = node->children[0]->value;

            radix = 10;

            if (name == "int_literal") {
                if (value.size() > 1 && value.substr(0, 2) == "0b") radix = 2;
                else if (value.size() > 1 && value.substr(0, 2) == "0o") radix = 8;
                else if (value.size() > 1 && value.substr(0, 2) == "0x") radix = 16;
                else if (value.size() > 1 && value.substr(0, 2) == "0t") radix = 36;
                setExprType(new TypeCall("i" + std::to_string(getIntBits(value))));
            }
            else if (name == "float_literal") setExprType(new TypeCall("f" + std::to_string(getFloatBits(value))));
            else if (name == "bool_literal") setExprType(new TypeCall("b"));
            else if (name == "char_literal") setExprType(new TypeCall("c"));
            else setExprType(new TypeCall(dynamic_cast<Type *>(Decl::getDecl("c", false, [&](Decl *decl) { return dynamic_cast<Type *>(decl); })), "", {}, { "ref" }, nullptr));
        }

        static std::string getOctalBinary(char octalDigit) {
            switch (octalDigit) {
                case '0': return "000";
                case '1': return "001";
                case '2': return "010";
                case '3': return "011";
                case '4': return "100";
                case '5': return "101";
                case '6': return "110";
                case '7': return "111";
                default: return "000";
            }
        }

        static std::string getHexBinary(char hexDigit) {
            switch (hexDigit) {
                case '0': return "0000";
                case '1': return "0001";
                case '2': return "0010";
                case '3': return "0011";
                case '4': return "0100";
                case '5': return "0101";
                case '6': return "0110";
                case '7': return "0111";
                case '8': return "1000";
                case '9': return "1001";
                case 'a':
                case 'A': return "1010";
                case 'b':
                case 'B': return "1011";
                case 'c':
                case 'C': return "1100";
                case 'd':
                case 'D': return "1101";
                case 'e':
                case 'E': return "1110";
                case 'f':
                case 'F': return "1111";
                default: return "0000";
            }
        }

        static std::string toBinary(mp::cpp_int n) {
            std::string r;
            while (n != 0) {
                r = (n % 2 == 0 ? "0":"1") + r;
                n /= 2;
            }
            return r;
        }

        static std::string convertB36Binary(std::string input) {
            mp::cpp_int large = 0;
            std::transform(input.begin(), input.end(), input.begin(), [](unsigned char c) { return std::toupper(c); });

            for (size_t i = input.size() - 1, j = 0; j < input.size(); j++, i--) {
                if (isalpha(input[i])) 
                    large += ((input[i] - 'A' + 10) * (mp::pow(mp::cpp_int(j), j)));
                else
                    large += ((input[i] - '0') * (mp::pow(mp::cpp_int(j), j)));
            }

            return toBinary(large);
        }

        static size_t getIntBits(std::string value) {
            if (value.size() > 2 && value.substr(0, 2) == "0b") {
                value = value.substr(2);
                return value.size();
            }

            if (value.size() > 2 && value.substr(0, 2) == "0o") {
                value = value.substr(2);
                std::string result;
                for (char c : value)
                    result += getOctalBinary(c);
                return result.size();
            }

            if (value.size() > 2 && value.substr(0, 2) == "0x") {
                value = value.substr(2);
                std::string result;
                for (char c : value)
                    result += getHexBinary(c);
                return result.size();
            }

            if (value.size() > 2 && value.substr(0, 2) == "0t") {
                value = value.substr(2);
                std::string result = convertB36Binary(value);
                return result.size();
            }

            std::string result = toBinary(mp::cpp_int(value));
            return result.size();
        }

        static size_t getFloatBits(std::string value) {
            return 32;
        }

        std::string print() const override {
            return value;
        }

        llvm::Type *getType(LLVMInfo info) const override {
            return exprType->getType(info);
        }

        llvm::Value *codegen(LLVMInfo info) override {
            std::string name = exprType->type->name;
            if (name[0] == 'i') {
                // + 1 for the negative bit
                return llvm::ConstantInt::get(llvm::IntegerType::get(*(info.context), (unsigned int) std::stoi(name.substr(1)) + 1), value, radix);
            }
            else if (name[0] == 'f')
                return llvm::ConstantFP::get(llvm::Type::getFloatTy(*(info.context)), value);
            else if (name[0] == 'b')
                return llvm::ConstantInt::get(llvm::IntegerType::get(*(info.context), 1), value == "true" ? 1 : 0);
            else if (name[0] == 'c') {
                // add the stuff like \n, \t, \033;
                return llvm::ConstantInt::get(llvm::IntegerType::get(*(info.context), 8), std::to_string((int) value[0]), 10);
            }
            
            std::string s = value.substr(1, value.size() - 2);
            return info.builder->CreateGlobalString(s, Decl::genName(), 0, info._module);
        }

        ASTNode *rewrite(std::map<std::string, TypeCall *>, std::map<std::string, Decl *>) override {
            return this;
        }
    };

}

#endif /* BA2C7946_7599_478F_A80F_58E37E1719B8 */

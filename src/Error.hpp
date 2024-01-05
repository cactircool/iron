#ifndef E7CC20C8_58D4_4B44_935F_FA3A5DA60406
#define E7CC20C8_58D4_4B44_935F_FA3A5DA60406

#include <string>
#include <vector>
#include "Compiler.hpp"

namespace iron {
    namespace Errors {
        std::vector<std::string> messages;
    }

    enum ErrorType {
        LOGIC,
        PREPROCESSOR,
        SYNTAX,
        SEMANTIC,
        RUNTIME
    };

    std::string errorTypes[] = {
        "\033[32mlogic\033[0m",
        "\033[36mpreprocessor\033[0m",
        "\033[31msyntax\033[0m",
        "\033[31msemantic\033[0m",
        "\033[33mruntime\033[0m",
    };

    void hurl(ErrorType errorType, std::string message, Token tok, std::string example, bool fatal = false) {
        std::string preamble = sourceName + ":" + std::to_string(tok.line + 1) + ":" + std::to_string(tok.pos + 1) + ": ";

        std::ifstream file(sourceName);
        std::string line;
        for (size_t i = 0; i <= tok.line + 1; i++)
            std::getline(file, line);
        std::cerr << line << std::endl;

        for (size_t i = 0; i < tok.pos > 0 ? tok.pos - 1 : 0; i++)
            std::cerr << ' ';
        std::cerr << "\033[32m^\033[0m" << std::endl;
        std::cerr << preamble << errorTypes[errorType] << ": " << message << std::endl;

        if (!example.empty()) {
            std::cerr << "ex: \033[32m" << example << "\033[0m" << std::endl;
        }

        if (fatal)
            exit(1);
    }
}

#endif /* E7CC20C8_58D4_4B44_935F_FA3A5DA60406 */

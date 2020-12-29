#define _CRT_SECURE_NO_WARNINGS

#include <cstdio>

#include "Error.h"
#include "Lexer.h"
#include "Parser.h"
#include "SymbolTable.h"
#include "Intermediate.h"
#include "ObjectCode.h"

int main() {
    std::FILE* f_in = std::fopen("testfile.txt", "r");
    std::FILE* f_out = std::fopen("mips.txt", "w");

    Error error;
    Lexer lexer(f_in, error);
    SymbolTable symbol_table;
    Intermediate intermediate(symbol_table);
    Parser parser(lexer, symbol_table, error, intermediate);
    ObjectCode object_code(symbol_table, intermediate);

    parser.Program();

    intermediate.PrintIntermediateCode(stdout);

    if (!error.HasError()) {
        object_code.GenCode();
        object_code.PrintToFile(f_out);
    }
    
    std::fclose(f_in);
    std::fclose(f_out);
    return 0;
}

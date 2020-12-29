#pragma once

#include <cstdio>
#include <string>
#include <vector>

#include "SymbolTable.h"

enum class IntermediateOp {
    ADD,
    SUB,
    MUL,
    DIV,

    SCAN,
    PRINT,

    ARR_STORE,
    ARR_LOAD,

    BEQ,
    BNE,
    BGT,
    BGE,
    BLT,
    BLE,

    FUNC_BEGIN,
    FUNC_END,
    PUSH,
    CALL,
    RET,

    GOTO,
    LABEL,
    EXIT
};

struct IntermediateCode {
    std::string dst;
    IntermediateOp op;
    std::string src1;
    std::string src2;
};

class Intermediate {
private:

    int tmp_cnt_;
    int label_cnt_;

    SymbolTable& symbol_table_;

public:

    std::vector<IntermediateCode> intermediate_code;

    Intermediate(SymbolTable& symbol_table);

    std::string GenTmp(const std::string& cur_func, SymbolType type);
    std::string GenLabel();

    void AddIntermediateCode(const std::string& dst,
        IntermediateOp op, const std::string& src1, const std::string& src2);
    void AddIntermediateCode(IntermediateCode& intermediate);
    void PrintIntermediateCode(FILE *f) const;
};

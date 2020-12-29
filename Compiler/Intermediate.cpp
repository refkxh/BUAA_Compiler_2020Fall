#include "Intermediate.h"

Intermediate::Intermediate(SymbolTable& symbol_table) :tmp_cnt_(0), label_cnt_(0), symbol_table_(symbol_table) {}

std::string Intermediate::GenTmp(const std::string& cur_func, SymbolType type) {
    std::string name = "T" + std::to_string(tmp_cnt_++);
    symbol_table_.AddSymbol(cur_func, name, type, SymbolClass::VAR, 0);
    return name;
}

std::string Intermediate::GenLabel() {
    return "L" + std::to_string(label_cnt_++);
}

void Intermediate::AddIntermediateCode(const std::string& dst,
    IntermediateOp op, const std::string& src1, const std::string& src2) {
    intermediate_code.emplace_back(IntermediateCode{ .dst = dst, .op = op, .src1 = src1, .src2 = src2 });
}

void Intermediate::AddIntermediateCode(IntermediateCode& intermediate) {
    intermediate_code.emplace_back(intermediate);
}

void Intermediate::PrintIntermediateCode(FILE *f) const {
    for (auto& code : intermediate_code) {
        switch (code.op) {
        case IntermediateOp::ADD:
            fprintf(f, "%s = %s + %s\n", code.dst.c_str(), code.src1.c_str(), code.src2.c_str());
            break;
        case IntermediateOp::SUB:
            fprintf(f, "%s = %s - %s\n", code.dst.c_str(), code.src1.c_str(), code.src2.c_str());
            break;
        case IntermediateOp::MUL:
            fprintf(f, "%s = %s * %s\n", code.dst.c_str(), code.src1.c_str(), code.src2.c_str());
            break;
        case IntermediateOp::DIV:
            fprintf(f, "%s = %s / %s\n", code.dst.c_str(), code.src1.c_str(), code.src2.c_str());
            break;
        case IntermediateOp::SCAN:
            fprintf(f, "scanf %s\n", code.dst.c_str());
            break;
        case IntermediateOp::PRINT:
            if (!code.src2.empty()) fprintf(f, "printf %s, %s\n", code.src1.c_str(), code.src2.c_str());
            else fprintf(f, "printf %s\n", code.src1.c_str());
            break;
        case IntermediateOp::ARR_STORE:
            fprintf(f, "%s []= %s %s\n", code.dst.c_str(), code.src1.c_str(), code.src2.c_str());
            break;
        case IntermediateOp::ARR_LOAD:
            fprintf(f, "%s =[] %s %s\n", code.dst.c_str(), code.src1.c_str(), code.src2.c_str());
            break;
        case IntermediateOp::BEQ:
            fprintf(f, "beq %s, %s, %s\n", code.dst.c_str(), code.src1.c_str(), code.src2.c_str());
            break;
        case IntermediateOp::BNE:
            fprintf(f, "bne %s, %s, %s\n", code.dst.c_str(), code.src1.c_str(), code.src2.c_str());
            break;
        case IntermediateOp::BGT:
            fprintf(f, "bgt %s, %s, %s\n", code.dst.c_str(), code.src1.c_str(), code.src2.c_str());
            break;
        case IntermediateOp::BGE:
            fprintf(f, "bge %s, %s, %s\n", code.dst.c_str(), code.src1.c_str(), code.src2.c_str());
            break;
        case IntermediateOp::BLT:
            fprintf(f, "blt %s, %s, %s\n", code.dst.c_str(), code.src1.c_str(), code.src2.c_str());
            break;
        case IntermediateOp::BLE:
            fprintf(f, "ble %s, %s, %s\n", code.dst.c_str(), code.src1.c_str(), code.src2.c_str());
            break;
        case IntermediateOp::FUNC_BEGIN:
            fprintf(f, "func %s\n", code.dst.c_str());
            break;
        case IntermediateOp::FUNC_END:
            fprintf(f, "endfunc %s\n", code.dst.c_str());
            break;
        case IntermediateOp::PUSH:
            fprintf(f, "push %s\n", code.dst.c_str());
            break;
        case IntermediateOp::CALL:
            fprintf(f, "call %s\n", code.src1.c_str());
            if (!code.dst.empty()) fprintf(f, "%s = RET\n", code.dst.c_str());
            break;
        case IntermediateOp::RET:
            fprintf(f, "ret %s\n", code.dst.c_str());
            break;
        case IntermediateOp::GOTO:
            fprintf(f, "goto %s\n", code.dst.c_str());
            break;
        case IntermediateOp::LABEL:
            fprintf(f, "%s:\n", code.dst.c_str());
            break;
        case IntermediateOp::EXIT:
            fprintf(f, "exit\n");
            break;
        default:
            break;
        }
    }
}

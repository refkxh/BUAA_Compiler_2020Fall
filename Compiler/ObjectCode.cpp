#include <stdexcept>

#include "ObjectCode.h"

void ObjectCode::ClearT_() {
    for (int i = 0; i < 10; i++) {
        t_offset_[i] = -1;
        t_use_[i] = false;
    }
    t_clock_ = 0;
}

int ObjectCode::GetT_(int offset, bool need_val, int sp_offset) {
    for (int i = 0; i < 10; i++) {
        if (offset == t_offset_[i]) {
            t_use_[i] = true;
            return i;
        }
        else if (t_offset_[i] == -1) break;
    }
    while (t_use_[t_clock_]) {
        t_use_[t_clock_] = false;
        t_clock_ = (t_clock_ + 1) % 10;
    }
    if (t_offset_[t_clock_] > 0) {
        mips_code_.emplace_back("sw $t" + std::to_string(t_clock_) 
            + ", " + std::to_string(t_offset_[t_clock_] + sp_offset) + "($sp)");
    }
    t_offset_[t_clock_] = offset;
    t_use_[t_clock_] = true;
    if (need_val) {
        mips_code_.emplace_back("lw $t" + std::to_string(t_clock_) 
            + ", " + std::to_string(offset + sp_offset) + "($sp)");
    }
    int ret = t_clock_;
    t_clock_ = (t_clock_ + 1) % 10;
    return ret;
}

void ObjectCode::StoreT_(int sp_offset) {
    for (int i = 0; i < 10; i++) {
        if (t_offset_[i] >= 0) {
            mips_code_.emplace_back("sw $t" + std::to_string(i)
                + ", " + std::to_string(t_offset_[i] + sp_offset) + "($sp)");
        }
    }
}

void ObjectCode::GenData_() {
    mips_code_.emplace_back(".data");

    for (auto& item : symbol_table_.get_global_table()) {
        if (item.second.cls != SymbolClass::VAR) continue;
        if (item.second.dim0 == 0) {
            mips_code_.emplace_back(item.first + ": .word " + std::to_string(item.second.val));
        }
        else if (item.second.dim1 == 0) {
            mips_code_.emplace_back(item.first + ':');
            if (item.second.val < 0) {
                mips_code_.emplace_back(".word 0:" + std::to_string(item.second.dim0));
            }
            else {
                auto initializer = symbol_table_.GetKthArrayInitializer(item.second.val);
                for (int i = 0; i < item.second.dim0; i++) {
                    mips_code_.emplace_back(".word " + std::to_string(initializer[i]));
                }
            }
        }
        else {
            mips_code_.emplace_back(item.first + ':');
            if (item.second.val < 0) {
                mips_code_.emplace_back(".word 0:" + std::to_string(item.second.dim1 * item.second.dim0));
            }
            else {
                auto initializer = symbol_table_.GetKthArrayInitializer(item.second.val);
                for (int i = 0; i < item.second.dim1 * item.second.dim0; i++) {
                    mips_code_.emplace_back(".word " + std::to_string(initializer[i]));
                }
            }
        }
    }

    for (auto& item : symbol_table_.get_string_table()) {
        std::string str;
        for (auto c : item.first) {
            str += c;
            if (c == '\\') str += c;
        }
        mips_code_.emplace_back("S" + std::to_string(item.second) + ": .asciiz \"" + str + '\"');
    }
}

void ObjectCode::GenText_() {
    mips_code_.emplace_back(".text");
    mips_code_.emplace_back("j main");
    for (auto& item : intermediate_.intermediate_code) {
        switch (item.op) {
        case IntermediateOp::ADD:
            GenAdd_(item);
            break;
        case IntermediateOp::SUB:
            GenSub_(item);
            break;
        case IntermediateOp::MUL:
            GenMul_(item);
            break;
        case IntermediateOp::DIV:
            GenDiv_(item);
            break;
        case IntermediateOp::SCAN:
            GenScan_(item);
            break;
        case IntermediateOp::PRINT:
            GenPrint_(item);
            break;
        case IntermediateOp::ARR_STORE:
            GenArrStore_(item);
            break;
        case IntermediateOp::ARR_LOAD:
            GenArrLoad_(item);
            break;
        case IntermediateOp::BEQ:
        case IntermediateOp::BNE:
        case IntermediateOp::BGT:
        case IntermediateOp::BGE:
        case IntermediateOp::BLT:
        case IntermediateOp::BLE:
            StoreT_();
            GenBranch_(item);
            ClearT_();
            break;
        case IntermediateOp::FUNC_BEGIN:
            ClearT_();
            cur_func_ = item.dst;
            reg_cnt_ = 0;
            GenFuncBegin_();
            break;
        case IntermediateOp::FUNC_END:
            GenFuncEnd_();
            cur_func_.clear();
            break;
        case IntermediateOp::PUSH:
            args_.emplace_back(item.dst);
            break;
        case IntermediateOp::CALL:
            GenCall_(item);
            break;
        case IntermediateOp::RET:
            GenRet_(item);
            break;
        case IntermediateOp::GOTO:
            StoreT_();
            mips_code_.emplace_back("j " + item.dst);
            ClearT_();
            break;
        case IntermediateOp::LABEL:
            StoreT_();
            mips_code_.emplace_back(item.dst + ":");
            ClearT_();
            break;
        case IntermediateOp::EXIT:
            GenExit_();
            break;
        default:
            break;
        }
    }
}

void ObjectCode::GenAdd_(IntermediateCode& code) {
    auto dst_entry = symbol_table_.LookupSymbol(cur_func_, code.dst);
    auto src1_entry = symbol_table_.LookupSymbol(cur_func_, code.src1);
    auto src2_entry = symbol_table_.LookupSymbol(cur_func_, code.src2);

    std::string dst_reg = "$v1";
    if (!dst_entry.second && dst_entry.first->addr >= 0) {
        dst_reg = "$t" + std::to_string(GetT_(dst_entry.first->addr));
    }
    else if (dst_entry.first->addr < 0) dst_reg = "$s" + std::to_string(-dst_entry.first->addr - 1);

    int op1, op2;
    bool is_imm1 = true;
    bool is_imm2 = true;

    try {
        op1 = std::stoi(code.src1);
    }
    catch (std::invalid_argument& e) {
        if (src1_entry.first->cls == SymbolClass::CONST) op1 = src1_entry.first->val;
        else is_imm1 = false;
    }

    try {
        op2 = std::stoi(code.src2);
    }
    catch (std::invalid_argument& e) {
        if (src2_entry.first->cls == SymbolClass::CONST) op2 = src2_entry.first->val;
        else is_imm2 = false;
    }

    if (is_imm1 && is_imm2) {
        mips_code_.emplace_back("li " + dst_reg + ", " + std::to_string(op1 + op2));
    }
    else if (is_imm1) {
        if (!src2_entry.second && src2_entry.first->addr >= 0) {
            mips_code_.emplace_back("addiu " + dst_reg + ", $t"
                + std::to_string(GetT_(src2_entry.first->addr, true)) + ", " + std::to_string(op1));
        }
        else if (src2_entry.second) {
            mips_code_.emplace_back("lw " + dst_reg + ", " + code.src2);
            mips_code_.emplace_back("addiu " + dst_reg + ", " + dst_reg + ", " + std::to_string(op1));
        }
        else {
            mips_code_.emplace_back("addiu " + dst_reg + ", $s"
                + std::to_string(-src2_entry.first->addr - 1) + ", " + std::to_string(op1));
        }
    }
    else if (is_imm2) {
        if (!src1_entry.second && src1_entry.first->addr >= 0) {
            mips_code_.emplace_back("addiu " + dst_reg + ", $t"
                + std::to_string(GetT_(src1_entry.first->addr, true)) + ", " + std::to_string(op2));
        }
        else if (src1_entry.second) {
            mips_code_.emplace_back("lw " + dst_reg + ", " + code.src1);
            mips_code_.emplace_back("addiu " + dst_reg + ", " + dst_reg + ", " + std::to_string(op2));
        }
        else {
            mips_code_.emplace_back("addiu " + dst_reg + ", $s"
                + std::to_string(-src1_entry.first->addr - 1) + ", " + std::to_string(op2));
        }
    }
    else {
        std::string src1_reg = dst_reg;
        std::string src2_reg = "$fp";

        if (!src1_entry.second && src1_entry.first->addr >= 0) {
            src1_reg = "$t" + std::to_string(GetT_(src1_entry.first->addr, true));
        }
        else if (src1_entry.second) mips_code_.emplace_back("lw " + src1_reg + ", " + code.src1);
        else src1_reg = "$s" + std::to_string(-src1_entry.first->addr - 1);

        if (!src2_entry.second && src2_entry.first->addr >= 0) {
            src2_reg = "$t" + std::to_string(GetT_(src2_entry.first->addr, true));
        }
        else if (src2_entry.second) mips_code_.emplace_back("lw " + src2_reg + ", " + code.src2);
        else src2_reg = "$s" + std::to_string(-src2_entry.first->addr - 1);

        mips_code_.emplace_back("addu " + dst_reg + ", " + src1_reg + ", " + src2_reg);
    }

    if (dst_reg == "$v1") mips_code_.emplace_back("sw $v1, " + code.dst);
}

void ObjectCode::GenSub_(IntermediateCode& code) {
    auto dst_entry = symbol_table_.LookupSymbol(cur_func_, code.dst);
    auto src1_entry = symbol_table_.LookupSymbol(cur_func_, code.src1);
    auto src2_entry = symbol_table_.LookupSymbol(cur_func_, code.src2);

    std::string dst_reg = "$v1";
    if (!dst_entry.second && dst_entry.first->addr >= 0) {
        dst_reg = "$t" + std::to_string(GetT_(dst_entry.first->addr));
    }
    else if (dst_entry.first->addr < 0) dst_reg = "$s" + std::to_string(-dst_entry.first->addr - 1);

    int op1, op2;
    bool is_imm1 = true;
    bool is_imm2 = true;

    try {
        op1 = std::stoi(code.src1);
    }
    catch (std::invalid_argument& e) {
        if (src1_entry.first->cls == SymbolClass::CONST) op1 = src1_entry.first->val;
        else is_imm1 = false;
    }

    try {
        op2 = std::stoi(code.src2);
    }
    catch (std::invalid_argument& e) {
        if (src2_entry.first->cls == SymbolClass::CONST) op2 = src2_entry.first->val;
        else is_imm2 = false;
    }

    if (is_imm1 && is_imm2) {
        mips_code_.emplace_back("li " + dst_reg + ", " + std::to_string(op1 - op2));
    }
    else if (is_imm1) {
        if (op1 == 0) {
            if (!src2_entry.second && src2_entry.first->addr >= 0) {
                mips_code_.emplace_back("subu " + dst_reg + ", $0, $t"
                    + std::to_string(GetT_(src2_entry.first->addr, true)));
            }
            else if (src2_entry.second) {
                mips_code_.emplace_back("lw $fp, " + code.src2);
                mips_code_.emplace_back("subu " + dst_reg + ", $0, $fp");
            }
            else {
                mips_code_.emplace_back("subu " + dst_reg + ", $0, $s"
                    + std::to_string(-src2_entry.first->addr - 1));
            }
        }
        else {
            mips_code_.emplace_back("li " + dst_reg + ", " + std::to_string(op1));
            if (!src2_entry.second && src2_entry.first->addr >= 0) {
                mips_code_.emplace_back("subu " + dst_reg + ", " + dst_reg + ", $t"
                    + std::to_string(GetT_(src2_entry.first->addr, true)));
            }
            else if (src2_entry.second) {
                mips_code_.emplace_back("lw $fp, " + code.src2);
                mips_code_.emplace_back("subu " + dst_reg + ", " + dst_reg + ", $fp");
            }
            else {
                mips_code_.emplace_back("subu " + dst_reg + ", " + dst_reg + ", $s"
                    + std::to_string(-src2_entry.first->addr - 1));
            }
        }
        
    }
    else if (is_imm2) {
        if (!src1_entry.second && src1_entry.first->addr >= 0) {
            mips_code_.emplace_back("addiu " + dst_reg + ", $t"
                + std::to_string(GetT_(src1_entry.first->addr, true)) + ", " + std::to_string(-op2));
        }
        else if (src1_entry.second) {
            mips_code_.emplace_back("lw " + dst_reg + ", " + code.src1);
            mips_code_.emplace_back("addiu " + dst_reg + ", " + dst_reg + ", " + std::to_string(-op2));
        }
        else {
            mips_code_.emplace_back("addiu " + dst_reg + ", $s"
                + std::to_string(-src1_entry.first->addr - 1) + ", " + std::to_string(-op2));
        }
    }
    else {
        std::string src1_reg = dst_reg;
        std::string src2_reg = "$fp";

        if (!src1_entry.second && src1_entry.first->addr >= 0) {
            src1_reg = "$t" + std::to_string(GetT_(src1_entry.first->addr, true));
        }
        else if (src1_entry.second) mips_code_.emplace_back("lw " + src1_reg + ", " + code.src1);
        else src1_reg = "$s" + std::to_string(-src1_entry.first->addr - 1);

        if (!src2_entry.second && src2_entry.first->addr >= 0) {
            src2_reg = "$t" + std::to_string(GetT_(src2_entry.first->addr, true));
        }
        else if (src2_entry.second) mips_code_.emplace_back("lw " + src2_reg + ", " + code.src2);
        else src2_reg = "$s" + std::to_string(-src2_entry.first->addr - 1);

        mips_code_.emplace_back("subu " + dst_reg + ", " + src1_reg + ", " + src2_reg);
    }

    if (dst_reg == "$v1") mips_code_.emplace_back("sw $v1, " + code.dst);
}

void ObjectCode::GenMul_(IntermediateCode& code) {
    auto dst_entry = symbol_table_.LookupSymbol(cur_func_, code.dst);
    auto src1_entry = symbol_table_.LookupSymbol(cur_func_, code.src1);
    auto src2_entry = symbol_table_.LookupSymbol(cur_func_, code.src2);

    std::string dst_reg = "$v1";
    if (!dst_entry.second && dst_entry.first->addr >= 0) {
        dst_reg = "$t" + std::to_string(GetT_(dst_entry.first->addr));
    }
    else if (dst_entry.first->addr < 0) dst_reg = "$s" + std::to_string(-dst_entry.first->addr - 1);

    int op1, op2;
    bool is_imm1 = true;
    bool is_imm2 = true;

    try {
        op1 = std::stoi(code.src1);
    }
    catch (std::invalid_argument& e) {
        if (src1_entry.first->cls == SymbolClass::CONST) op1 = src1_entry.first->val;
        else is_imm1 = false;
    }

    try {
        op2 = std::stoi(code.src2);
    }
    catch (std::invalid_argument& e) {
        if (src2_entry.first->cls == SymbolClass::CONST) op2 = src2_entry.first->val;
        else is_imm2 = false;
    }

    if ((is_imm1 && is_imm2) || (is_imm1 && op1 == 0) || (is_imm2 && op2 == 0)) {
        mips_code_.emplace_back("li " + dst_reg + ", " + std::to_string(op1 * op2));
    }
    else if (is_imm1) {
        if (op1 > 0 && (op1 & (op1 - 1)) == 0) {
            int shift = 0;
            while (op1 > 1) {
                shift++;
                op1 >>= 1;
            }

            if (!src2_entry.second && src2_entry.first->addr >= 0) {
                mips_code_.emplace_back("sll " + dst_reg + ", $t"
                    + std::to_string(GetT_(src2_entry.first->addr, true)) + ", " + std::to_string(shift));
            }
            else if (src2_entry.second) {
                mips_code_.emplace_back("lw " + dst_reg + ", " + code.src2);
                mips_code_.emplace_back("sll " + dst_reg + ", " + dst_reg + ", " + std::to_string(shift));
            }
            else {
                mips_code_.emplace_back("sll " + dst_reg + ", $s"
                    + std::to_string(-src2_entry.first->addr - 1) + ", " + std::to_string(shift));
            }
        }
        else {
            if (!src2_entry.second && src2_entry.first->addr >= 0) {
                mips_code_.emplace_back("mul " + dst_reg + ", $t"
                    + std::to_string(GetT_(src2_entry.first->addr, true)) + ", " + std::to_string(op1));
            }
            else if (src2_entry.second) {
                mips_code_.emplace_back("lw " + dst_reg + ", " + code.src2);
                mips_code_.emplace_back("mul " + dst_reg + ", " + dst_reg + ", " + std::to_string(op1));
            }
            else {
                mips_code_.emplace_back("mul " + dst_reg + ", $s"
                    + std::to_string(-src2_entry.first->addr - 1) + ", " + std::to_string(op1));
            }
        }
    }
    else if (is_imm2) {
        if (op2 > 0 && (op2 & (op2 - 1)) == 0) {
            int shift = 0;
            while (op2 > 1) {
                shift++;
                op2 >>= 1;
            }

            if (!src1_entry.second && src1_entry.first->addr >= 0) {
                mips_code_.emplace_back("sll " + dst_reg + ", $t"
                    + std::to_string(GetT_(src1_entry.first->addr, true)) + ", " + std::to_string(shift));
            }
            else if (src1_entry.second) {
                mips_code_.emplace_back("lw " + dst_reg + ", " + code.src1);
                mips_code_.emplace_back("sll " + dst_reg + ", " + dst_reg + ", " + std::to_string(shift));
            }
            else {
                mips_code_.emplace_back("sll " + dst_reg + ", $s"
                    + std::to_string(-src1_entry.first->addr - 1) + ", " + std::to_string(shift));
            }
        }
        else {
            if (!src1_entry.second && src1_entry.first->addr >= 0) {
                mips_code_.emplace_back("mul " + dst_reg + ", $t"
                    + std::to_string(GetT_(src1_entry.first->addr, true)) + ", " + std::to_string(op2));
            }
            else if (src1_entry.second) {
                mips_code_.emplace_back("lw " + dst_reg + ", " + code.src1);
                mips_code_.emplace_back("mul " + dst_reg + ", " + dst_reg + ", " + std::to_string(op2));
            }
            else {
                mips_code_.emplace_back("mul " + dst_reg + ", $s"
                    + std::to_string(-src1_entry.first->addr - 1) + ", " + std::to_string(op2));
            }
        }
    }
    else {
        std::string src1_reg = dst_reg;
        std::string src2_reg = "$fp";

        if (!src1_entry.second && src1_entry.first->addr >= 0) {
            src1_reg = "$t" + std::to_string(GetT_(src1_entry.first->addr, true));
        }
        else if (src1_entry.second) mips_code_.emplace_back("lw " + src1_reg + ", " + code.src1);
        else src1_reg = "$s" + std::to_string(-src1_entry.first->addr - 1);

        if (!src2_entry.second && src2_entry.first->addr >= 0) {
            src2_reg = "$t" + std::to_string(GetT_(src2_entry.first->addr, true));
        }
        else if (src2_entry.second) mips_code_.emplace_back("lw " + src2_reg + ", " + code.src2);
        else src2_reg = "$s" + std::to_string(-src2_entry.first->addr - 1);

        mips_code_.emplace_back("mul " + dst_reg + ", " + src1_reg + ", " + src2_reg);
    }

    if (dst_reg == "$v1") mips_code_.emplace_back("sw $v1, " + code.dst);
}

void ObjectCode::GenDiv_(IntermediateCode& code) {
    auto dst_entry = symbol_table_.LookupSymbol(cur_func_, code.dst);
    auto src1_entry = symbol_table_.LookupSymbol(cur_func_, code.src1);
    auto src2_entry = symbol_table_.LookupSymbol(cur_func_, code.src2);

    std::string dst_reg = "$v1";
    if (!dst_entry.second && dst_entry.first->addr >= 0) {
        dst_reg = "$t" + std::to_string(GetT_(dst_entry.first->addr));
    }
    else if (dst_entry.first->addr < 0) dst_reg = "$s" + std::to_string(-dst_entry.first->addr - 1);

    int op1, op2;
    bool is_imm1 = true;
    bool is_imm2 = true;

    try {
        op1 = std::stoi(code.src1);
    }
    catch (std::invalid_argument& e) {
        if (src1_entry.first->cls == SymbolClass::CONST) op1 = src1_entry.first->val;
        else is_imm1 = false;
    }

    try {
        op2 = std::stoi(code.src2);
    }
    catch (std::invalid_argument& e) {
        if (src2_entry.first->cls == SymbolClass::CONST) op2 = src2_entry.first->val;
        else is_imm2 = false;
    }

    if (is_imm1 && is_imm2) {
        mips_code_.emplace_back("li " + dst_reg + ", " + std::to_string(op1 / op2));
    }
    else if (is_imm1) {
        mips_code_.emplace_back("li " + dst_reg + ", " + std::to_string(op1));
        if (op1 != 0) {
            if (!src2_entry.second && src2_entry.first->addr >= 0) {
                mips_code_.emplace_back("div " + dst_reg + ", $t"
                    + std::to_string(GetT_(src2_entry.first->addr, true)));
            }
            else if (src2_entry.second) {
                mips_code_.emplace_back("lw $fp, " + code.src2);
                mips_code_.emplace_back("div " + dst_reg + ", $fp");
            }
            else {
                mips_code_.emplace_back("div " + dst_reg + ", $s"
                    + std::to_string(-src2_entry.first->addr - 1));
            }
            mips_code_.emplace_back("mflo " + dst_reg);
        }
    }
    else if (is_imm2) {
        mips_code_.emplace_back("li " + dst_reg + ", " + std::to_string(op2));
        if (!src1_entry.second && src1_entry.first->addr >= 0) {
            mips_code_.emplace_back("div $t"
                + std::to_string(GetT_(src1_entry.first->addr, true)) + ", " + dst_reg);
        }
        else if (src1_entry.second) {
            mips_code_.emplace_back("lw $fp, " + code.src1);
            mips_code_.emplace_back("div $fp, "+ dst_reg);
        }
        else {
            mips_code_.emplace_back("div $s"
                + std::to_string(-src1_entry.first->addr - 1) + ", " + dst_reg);
        }
        mips_code_.emplace_back("mflo " + dst_reg);
    }
    else {
        std::string src1_reg = dst_reg;
        std::string src2_reg = "$fp";

        if (!src1_entry.second && src1_entry.first->addr >= 0) {
            src1_reg = "$t" + std::to_string(GetT_(src1_entry.first->addr, true));
        }
        else if (src1_entry.second) mips_code_.emplace_back("lw " + src1_reg + ", " + code.src1);
        else src1_reg = "$s" + std::to_string(-src1_entry.first->addr - 1);

        if (!src2_entry.second && src2_entry.first->addr >= 0) {
            src2_reg = "$t" + std::to_string(GetT_(src2_entry.first->addr, true));
        }
        else if (src2_entry.second) mips_code_.emplace_back("lw " + src2_reg + ", " + code.src2);
        else src2_reg = "$s" + std::to_string(-src2_entry.first->addr - 1);

        mips_code_.emplace_back("div " + src1_reg + ", " + src2_reg);
        mips_code_.emplace_back("mflo " + dst_reg);
    }

    if (dst_reg == "$v1") mips_code_.emplace_back("sw $v1, " + code.dst);
}

void ObjectCode::GenScan_(IntermediateCode& code) {
    auto entry = symbol_table_.LookupSymbol(cur_func_, code.dst);
    if (entry.first->type == SymbolType::INT) mips_code_.emplace_back("li $v0, 5");
    else mips_code_.emplace_back("li $v0, 12");
    mips_code_.emplace_back("syscall");
    if (!entry.second && entry.first->addr >= 0) {
        mips_code_.emplace_back("move $t"
            + std::to_string(GetT_(entry.first->addr)) + ", $v0");
    }
    else if (entry.second) {
        mips_code_.emplace_back("sw $v0, " + code.dst);
    }
    else mips_code_.emplace_back("move $s" + std::to_string(-entry.first->addr - 1) + ", $v0");
}

void ObjectCode::GenPrint_(IntermediateCode& code) {
    if (code.src1[0] == 'S') {
        mips_code_.emplace_back("la $a0, " + code.src1);
        mips_code_.emplace_back("li $v0, 4");
        mips_code_.emplace_back("syscall");
        if (!code.src2.empty()) {
            try {
                int to_print = std::stoi(code.src2);
                mips_code_.emplace_back("li $a0, " + std::to_string(to_print));
                mips_code_.emplace_back("li $v0, 1");
            }
            catch (std::invalid_argument& e) {
                auto entry = symbol_table_.LookupSymbol(cur_func_, code.src2);
                if (entry.first->cls == SymbolClass::CONST) {
                    mips_code_.emplace_back("li $a0, " + std::to_string(entry.first->val));
                }
                else if (!entry.second && entry.first->addr >= 0) {
                    mips_code_.emplace_back("move $a0, $t" 
                        + std::to_string(GetT_(entry.first->addr, true)));
                }
                else if (entry.second) {
                    mips_code_.emplace_back("lw $a0, " + code.src2);
                }
                else {
                    mips_code_.emplace_back("move $a0, $s" + std::to_string(-entry.first->addr - 1));
                }
                if (entry.first->type == SymbolType::INT) mips_code_.emplace_back("li $v0, 1");
                else mips_code_.emplace_back("li $v0, 11");
            }
            mips_code_.emplace_back("syscall");
        }
    }
    else {
        try {
            int to_print = std::stoi(code.src1);
            mips_code_.emplace_back("li $a0, " + std::to_string(to_print));
            mips_code_.emplace_back("li $v0, 1");
        }
        catch (std::invalid_argument& e) {
            auto entry = symbol_table_.LookupSymbol(cur_func_, code.src1);
            if (entry.first->cls == SymbolClass::CONST) {
                mips_code_.emplace_back("li $a0, " + std::to_string(entry.first->val));
            }
            else if (!entry.second && entry.first->addr >= 0) {
                mips_code_.emplace_back("move $a0, $t"
                    + std::to_string(GetT_(entry.first->addr, true)));
            }
            else if (entry.second) {
                mips_code_.emplace_back("lw $a0, " + code.src1);
            }
            else {
                mips_code_.emplace_back("move $a0, $s"
                    + std::to_string(-entry.first->addr - 1));
            }
            if (entry.first->type == SymbolType::INT) mips_code_.emplace_back("li $v0, 1");
            else mips_code_.emplace_back("li $v0, 11");
        }
        mips_code_.emplace_back("syscall");
    }
    mips_code_.emplace_back("li $a0, 10");
    mips_code_.emplace_back("li $v0, 11");
    mips_code_.emplace_back("syscall");
}

void ObjectCode::GenArrStore_(IntermediateCode& code) {
    auto arr_entry = symbol_table_.LookupSymbol(cur_func_, code.dst);
    auto index_entry = symbol_table_.LookupSymbol(cur_func_, code.src1);
    auto val_entry = symbol_table_.LookupSymbol(cur_func_, code.src2);

    std::string index_reg = "$v1";
    std::string val_reg = "$fp";

    try {
        int true_ind = std::stoi(code.src1) << 2;
        mips_code_.emplace_back("li " + index_reg + ", " + std::to_string(true_ind));
    }
    catch (std::invalid_argument& e) {
        if (index_entry.first->cls == SymbolClass::CONST) {
            mips_code_.emplace_back("li " + index_reg + ", " 
                + std::to_string(index_entry.first->val << 2));
        }
        else if (!index_entry.second && index_entry.first->addr >= 0) {
            mips_code_.emplace_back("sll " + index_reg 
                + ", $t" + std::to_string(GetT_(index_entry.first->addr, true)) + ", 2");
        }
        else if (index_entry.second) {
            mips_code_.emplace_back("lw " + index_reg + ", " + code.src1);
            mips_code_.emplace_back("sll " + index_reg + ", " + index_reg + ", 2");
        }
        else {
            mips_code_.emplace_back("sll " + index_reg
                + ", $s" + std::to_string(-index_entry.first->addr - 1) + ", 2");
        }
    }

    try {
        std::stoi(code.src2);
        mips_code_.emplace_back("li " + val_reg + ", " + code.src2);
    }
    catch (std::invalid_argument& e) {
        if (val_entry.first->cls == SymbolClass::CONST) {
            mips_code_.emplace_back("li " + val_reg + ", "
                + std::to_string(val_entry.first->val));
        }
        else if (!val_entry.second && val_entry.first->addr >= 0) {
            val_reg = "$t" + std::to_string(GetT_(val_entry.first->addr, true));
        }
        else if (val_entry.second) {
            mips_code_.emplace_back("lw " + val_reg + ", " + code.src2);
        }
        else val_reg = "$s" + std::to_string(-val_entry.first->addr - 1);
    }

    if (arr_entry.second) mips_code_.emplace_back("sw " + val_reg + ", " + code.dst 
        + "(" + index_reg + ")");
    else {
        mips_code_.emplace_back("addu " + index_reg + ", " + index_reg + ", $sp");
        mips_code_.emplace_back("sw " + val_reg + ", " + std::to_string(arr_entry.first->addr)
            + "(" + index_reg + ")");
    }
}

void ObjectCode::GenArrLoad_(IntermediateCode& code) {
    auto arr_entry = symbol_table_.LookupSymbol(cur_func_, code.dst);
    auto index_entry = symbol_table_.LookupSymbol(cur_func_, code.src1);
    auto val_entry = symbol_table_.LookupSymbol(cur_func_, code.src2);

    std::string index_reg = "$v1";
    std::string val_reg = "$fp";

    try {
        int true_ind = std::stoi(code.src1) << 2;
        mips_code_.emplace_back("li " + index_reg + ", " + std::to_string(true_ind));
    }
    catch (std::invalid_argument& e) {
        if (index_entry.first->cls == SymbolClass::CONST) {
            mips_code_.emplace_back("li " + index_reg + ", "
                + std::to_string(index_entry.first->val << 2));
        }
        else if (!index_entry.second && index_entry.first->addr >= 0) {
            mips_code_.emplace_back("sll " + index_reg
                + ", $t" + std::to_string(GetT_(index_entry.first->addr, true)) + ", 2");
        }
        else if (index_entry.second) {
            mips_code_.emplace_back("lw " + index_reg + ", " + code.src1);
            mips_code_.emplace_back("sll " + index_reg + ", " + index_reg + ", 2");
        }
        else {
            mips_code_.emplace_back("sll " + index_reg
                + ", $s" + std::to_string(-index_entry.first->addr - 1) + ", 2");
        }
    }

    if (!val_entry.second) {
        if (val_entry.first->addr >= 0) {
            val_reg = "$t" + std::to_string(GetT_(val_entry.first->addr));
        }
        else val_reg = "$s" + std::to_string(-val_entry.first->addr - 1);
    }

    if (arr_entry.second) mips_code_.emplace_back("lw " + val_reg + ", " + code.dst
        + "(" + index_reg + ")");
    else {
        mips_code_.emplace_back("addu " + index_reg + ", " + index_reg + ", $sp");
        mips_code_.emplace_back("lw " + val_reg + ", " + std::to_string(arr_entry.first->addr)
            + "(" + index_reg + ")");
    }

    if (val_reg == "$fp") mips_code_.emplace_back("sw " + val_reg + ", " + code.src2);
}

void ObjectCode::GenBranch_(IntermediateCode& code) {
    std::string branch_head;
    switch (code.op) {
    case IntermediateOp::BEQ:
        branch_head = "beq ";
        break;
    case IntermediateOp::BNE:
        branch_head = "bne ";
        break;
    case IntermediateOp::BGT:
        branch_head = "bgt ";
        break;
    case IntermediateOp::BGE:
        branch_head = "bge ";
        break;
    case IntermediateOp::BLT:
        branch_head = "blt ";
        break;
    case IntermediateOp::BLE:
        branch_head = "ble ";
        break;
    default:
        break;
    }

    auto src1_entry = symbol_table_.LookupSymbol(cur_func_, code.src1);
    auto src2_entry = symbol_table_.LookupSymbol(cur_func_, code.src2);

    int op1, op2;
    bool is_imm1 = true;
    bool is_imm2 = true;

    try {
        op1 = std::stoi(code.src1);
    }
    catch (std::invalid_argument& e) {
        if (src1_entry.first->cls == SymbolClass::CONST) op1 = src1_entry.first->val;
        else is_imm1 = false;
    }

    try {
        op2 = std::stoi(code.src2);
    }
    catch (std::invalid_argument& e) {
        if (src2_entry.first->cls == SymbolClass::CONST) op2 = src2_entry.first->val;
        else is_imm2 = false;
    }

    if (is_imm1 && is_imm2) {
        if (code.op == IntermediateOp::BEQ && op1 == op2) {
            mips_code_.emplace_back("j " + code.dst);
        }
        else if (code.op == IntermediateOp::BNE && op1 != op2) {
            mips_code_.emplace_back("j " + code.dst);
        }
        else if (code.op == IntermediateOp::BGE && op1 >= op2) {
            mips_code_.emplace_back("j " + code.dst);
        }
        else if (code.op == IntermediateOp::BGT && op1 > op2) {
            mips_code_.emplace_back("j " + code.dst);
        }
        else if (code.op == IntermediateOp::BLE && op1 <= op2) {
            mips_code_.emplace_back("j " + code.dst);
        }
        else if (code.op == IntermediateOp::BLT && op1 < op2) {
            mips_code_.emplace_back("j " + code.dst);
        }
    }
    else if (is_imm1) {
        switch (code.op) {
        case IntermediateOp::BGT:
            branch_head = "blt ";
            break;
        case IntermediateOp::BGE:
            branch_head = "ble ";
            break;
        case IntermediateOp::BLT:
            branch_head = "bgt ";
            break;
        case IntermediateOp::BLE:
            branch_head = "bge ";
            break;
        default:
            break;
        }

        std::string src2_reg = "$fp";

        if (!src2_entry.second && src2_entry.first->addr >= 0) {
            src2_reg = "$t" + std::to_string(GetT_(src2_entry.first->addr, true));
        }
        else if (src2_entry.second) mips_code_.emplace_back("lw " + src2_reg + ", " + code.src2);
        else src2_reg = "$s" + std::to_string(-src2_entry.first->addr - 1);

        mips_code_.emplace_back(branch_head + src2_reg + ", " + std::to_string(op1) + ", " + code.dst);
    }
    else if (is_imm2) {
        std::string src1_reg = "$v1";

        if (!src1_entry.second && src1_entry.first->addr >= 0) {
            src1_reg = "$t" + std::to_string(GetT_(src1_entry.first->addr, true));
        }
        else if (src1_entry.second) mips_code_.emplace_back("lw " + src1_reg + ", " + code.src1);
        else src1_reg = "$s" + std::to_string(-src1_entry.first->addr - 1);

        mips_code_.emplace_back(branch_head + src1_reg + ", " + std::to_string(op2) + ", " + code.dst);
    }
    else {
        std::string src1_reg = "$v1";
        std::string src2_reg = "$fp";

        if (!src1_entry.second && src1_entry.first->addr >= 0) {
            src1_reg = "$t" + std::to_string(GetT_(src1_entry.first->addr, true));
        }
        else if (src1_entry.second) mips_code_.emplace_back("lw " + src1_reg + ", " + code.src1);
        else src1_reg = "$s" + std::to_string(-src1_entry.first->addr - 1);

        if (!src2_entry.second && src2_entry.first->addr >= 0) {
            src2_reg = "$t" + std::to_string(GetT_(src2_entry.first->addr, true));
        }
        else if (src2_entry.second) mips_code_.emplace_back("lw " + src2_reg + ", " + code.src2);
        else src2_reg = "$s" + std::to_string(-src2_entry.first->addr - 1);

        mips_code_.emplace_back(branch_head + src1_reg + ", " + src2_reg + ", " + code.dst);
    }
}

void ObjectCode::GenFuncBegin_() {
    mips_code_.emplace_back(cur_func_ + ":");
    int stack_size = symbol_table_.LookupSymbol("", cur_func_).first->dim0;
    if (cur_func_ != "main") stack_size += 36;
    int pos = mips_code_.size();
    mips_code_.emplace_back("");

    int offset = 0;
    for (auto& item : symbol_table_.get_function_table(cur_func_)) {
        if (item.second.cls == SymbolClass::FUNCTION
            || item.second.cls == SymbolClass::CONST) continue;
        if (reg_cnt_ < 8 && item.first[0] != 'T' && item.second.dim0 == 0) {
            item.second.addr = -(++reg_cnt_);
            stack_size -= 4;
        }
        else {
            if (item.first[0] == 'T') stack_size += 4;
            item.second.addr = offset;
            if (item.second.dim0 == 0) offset += 4;
            else if (item.second.dim1 == 0) offset += (item.second.dim0) << 2;
            else offset += (item.second.dim1 * item.second.dim0) << 2;
        }
    }

    symbol_table_.LookupSymbol("", cur_func_).first->dim0 = stack_size;
    if (stack_size > 0) mips_code_[pos] = "addiu $sp, $sp, -" + std::to_string(stack_size);

    if (cur_func_ != "main") {
        for (int i = 0; i < reg_cnt_; i++) {
            mips_code_.emplace_back("sw $s" + std::to_string(i) + ", "
                + std::to_string(stack_size - 36 + (i << 2)) + "($sp)");
        }
        if (symbol_table_.LookupSymbol("", cur_func_).first->dim1 > 0) {
            mips_code_.emplace_back("sw $ra, " + std::to_string(stack_size - 4) + "($sp)");
        }
    }

    for (auto& item : symbol_table_.get_function_table(cur_func_)) {
        if (!item.second.need_init && item.second.cls != SymbolClass::PARAM) continue;
        int offset = item.second.addr;
        if (offset < 0) {
            if (item.second.cls == SymbolClass::PARAM) {
                if (item.second.val < 4) {
                    mips_code_.emplace_back("move $s" + std::to_string(-offset - 1) 
                        + ", $a" + std::to_string(item.second.val));
                }
                else {
                    mips_code_.emplace_back("lw $s" + std::to_string(-offset - 1) + ", "
                        + std::to_string(stack_size + (symbol_table_.LookupSymbol("", cur_func_).first->val - item.second.val - 1) * 4)
                        + "($sp)");
                }
            }
            else {
                mips_code_.emplace_back("li $s" + std::to_string(-offset - 1) + ", "
                    + std::to_string(item.second.val));
            }
        }
        else {
            if (item.second.dim0 == 0) {
                if (item.second.cls == SymbolClass::PARAM) {
                    if (item.second.val < 4) {
                        mips_code_.emplace_back("sw $a" + std::to_string(item.second.val)
                            + ", " + std::to_string(offset) + "($sp)");
                    }
                    else {
                        mips_code_.emplace_back("lw $v1, "
                            + std::to_string(stack_size + (symbol_table_.LookupSymbol("", cur_func_).first->val - item.second.val - 1) * 4)
                            + "($sp)");
                        mips_code_.emplace_back("sw $v1, " + std::to_string(offset) + "($sp)");
                    }
                }
                else {
                    mips_code_.emplace_back("li $v1, " + std::to_string(item.second.val));
                    mips_code_.emplace_back("sw $v1, " + std::to_string(offset) + "($sp)");
                }
            }
            else if (item.second.dim1 == 0) {
                auto& initializer = symbol_table_.GetKthArrayInitializer(item.second.val);
                for (int i = 0; i < item.second.dim0; i++) {
                    mips_code_.emplace_back("li $v1, " + std::to_string(initializer[i]));
                    mips_code_.emplace_back("sw $v1, " + std::to_string(offset + (i << 2)) + "($sp)");
                }
            }
            else {
                auto& initializer = symbol_table_.GetKthArrayInitializer(item.second.val);
                for (int i = 0; i < item.second.dim1 * item.second.dim0; i++) {
                    mips_code_.emplace_back("li $v1, " + std::to_string(initializer[i]));
                    mips_code_.emplace_back("sw $v1, " + std::to_string(offset + (i << 2)) + "($sp)");
                }
            }
        }
    }
}

void ObjectCode::GenFuncEnd_() {
    auto entry = symbol_table_.LookupSymbol("", cur_func_).first;
    int stack_size = entry->dim0;

    if (cur_func_ != "main") {
        for (int i = 0; i < reg_cnt_; i++) {
            mips_code_.emplace_back("lw $s" + std::to_string(i) + ", "
                + std::to_string(stack_size - 36 + (i << 2)) + "($sp)");
        }
        if (symbol_table_.LookupSymbol("", cur_func_).first->dim1 > 0) {
            mips_code_.emplace_back("lw $ra, " + std::to_string(stack_size - 4) + "($sp)");
        }

        mips_code_.emplace_back("addiu $sp, $sp, " + std::to_string(stack_size + (entry->val << 2)));
        mips_code_.emplace_back("jr $ra");
    }
    else GenExit_();
}

void ObjectCode::GenCall_(IntermediateCode& code) {
    if (!args_.empty()) {
        mips_code_.emplace_back("addiu $sp, $sp, -" + std::to_string(args_.size() << 2));
    }
    int cnt = 0;
    for (auto& arg : args_) {
        if (cnt < 4) {
            try {
                std::stoi(arg);
                mips_code_.emplace_back("li $a" + std::to_string(cnt) + ", " + arg);
            }
            catch (std::invalid_argument& e) {
                auto entry = symbol_table_.LookupSymbol(cur_func_, arg);
                if (entry.first->cls == SymbolClass::CONST) {
                    mips_code_.emplace_back("li $a" + std::to_string(cnt) + ", "
                        + std::to_string(entry.first->val));
                }
                else {
                    mips_code_.emplace_back("move $a" + std::to_string(cnt) + ", $t"
                        + std::to_string(GetT_(entry.first->addr, true, args_.size() << 2)));
                }
            }
        }
        else {
            try {
                std::stoi(arg);
                mips_code_.emplace_back("li $v1, " + arg);
            }
            catch (std::invalid_argument& e) {
                auto entry = symbol_table_.LookupSymbol(cur_func_, arg);
                if (entry.first->cls == SymbolClass::CONST) {
                    mips_code_.emplace_back("li $v1, " + std::to_string(entry.first->val));
                }
                else {
                    mips_code_.emplace_back("move $v1, $t" 
                        + std::to_string(GetT_(entry.first->addr, true, args_.size() << 2)));
                }
            }
            mips_code_.emplace_back("sw $v1, " + std::to_string((args_.size() - cnt - 1) << 2) + "($sp)");
        }
        cnt++;
    }
    StoreT_(args_.size() << 2);
    args_.clear();

    mips_code_.emplace_back("jal " + code.src1);
    ClearT_();

    if (!code.dst.empty()) {
        auto dst_entry = symbol_table_.LookupSymbol(cur_func_, code.dst);
        mips_code_.emplace_back("move $t" + std::to_string(GetT_(dst_entry.first->addr)) + ", $v0");
    }
}

void ObjectCode::GenRet_(IntermediateCode& code) {
    if (!code.dst.empty()) {
        try {
            int ret = std::stoi(code.dst);
            mips_code_.emplace_back("li $v0, " + std::to_string(ret));
        }
        catch (std::invalid_argument& e) {
            auto entry = symbol_table_.LookupSymbol(cur_func_, code.dst);
            if (entry.first->cls == SymbolClass::CONST) {
                mips_code_.emplace_back("li $v0, " + std::to_string(entry.first->val));
            }
            else if (!entry.second && entry.first->addr >= 0) {
                mips_code_.emplace_back("move $v0, $t"
                    + std::to_string(GetT_(entry.first->addr, true)));
            }
            else if (entry.second) {
                mips_code_.emplace_back("lw $v0, " + code.dst);
            }
            else {
                mips_code_.emplace_back("move $v0, $s" + std::to_string(-entry.first->addr - 1));
            }
        }
    }
    GenFuncEnd_();
}

void ObjectCode::GenExit_() {
    mips_code_.emplace_back("li $v0, 10");
    mips_code_.emplace_back("syscall");
}

ObjectCode::ObjectCode(SymbolTable& symbol_table, Intermediate& intermediate) 
    :symbol_table_(symbol_table), intermediate_(intermediate) {
    ClearT_();
}

void ObjectCode::GenCode() {
    GenData_();
    GenText_();
}

void ObjectCode::PrintToFile(FILE* f) {
    for (auto& code : mips_code_) {
        fprintf(f, "%s\n", code.c_str());
    }
}

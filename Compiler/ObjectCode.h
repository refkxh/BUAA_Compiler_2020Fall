#pragma once

#include <cstdio>
#include <string>
#include <vector>

#include "Intermediate.h"
#include "SymbolTable.h"

class ObjectCode {
private:

    SymbolTable& symbol_table_;
    Intermediate& intermediate_;

    int reg_cnt_;

    int t_offset_[10];
    bool t_use_[10];
    int t_clock_;

    std::string cur_func_;

    std::vector<std::string> args_;

    std::vector<std::string> mips_code_;

    void ClearT_();
    int GetT_(int offset, bool need_val = false, int sp_offset = 0);
    void StoreT_(int sp_offset = 0);

    void GenData_();
    void GenText_();

    void GenAdd_(IntermediateCode& code);
    void GenSub_(IntermediateCode& code);
    void GenMul_(IntermediateCode& code);
    void GenDiv_(IntermediateCode& code);
    void GenScan_(IntermediateCode& code);
    void GenPrint_(IntermediateCode& code);
    void GenArrStore_(IntermediateCode& code);
    void GenArrLoad_(IntermediateCode& code);
    void GenBranch_(IntermediateCode& code);
    void GenFuncBegin_();
    void GenFuncEnd_();
    void GenCall_(IntermediateCode& code);
    void GenRet_(IntermediateCode& code);
    void GenExit_();

public:

    ObjectCode(SymbolTable& symbol_table, Intermediate& intermediate);

    void GenCode();

    void PrintToFile(FILE* f);
};

#pragma once

#include <cstdio>
#include <string>
#include <vector>
#include <unordered_map>
#include <utility>

#include "Intermediate.h"
#include "Lexer.h"
#include "ObjectCode.h"
#include "SymbolTable.h"

class Parser {
private:

    Lexer& lexer_;
    SymbolTable& symbol_table_;
    Error& error_;
    Intermediate& intermediate_;

    
    bool print_info_;
    std::vector<std::string>* output_;

    LexicalSymbol type_;
    std::string name_;
    int dim0_, dim1_;

    std::string cur_func_;
    int var_size_;
    bool has_ret_;

    void ConstDeclaration();
    void ConstDefinition();

    int Integer();

    void VarDeclaration();
    void VarDefinition();
    void VarDefinitionUninitialized();
    void VarDefinitionInitialized();

    void ParseArrayInfo();

    void FunDefinitionWithRet();
    void FunDefinitionWithoutRet();

    void ParseFuncBody();

    void CompoundStatement();

    int ParamList();

    void MainFun();

    std::pair<SymbolType, std::string> Expression();
    std::pair<SymbolType, std::string> Term();
    std::pair<SymbolType, std::string> Factor();

    void Statement();
    void AssignStatement(TableEntry* table_entry);
    void ConditionalStatement();
    void Condition(const std::string& skip_label);
    void LoopStatement();
    void CaseStatement();
    void CaseTable(const std::pair<SymbolType, std::string>& exp, const std::string& skip_label);
    std::string CaseSubstatement(const std::pair<SymbolType, std::string>& exp, const std::string& skip_label);
    void DefaultCase();

    std::pair<SymbolType, std::string> CallFunc();

    std::vector<std::string> ValParamList(const std::string func_name, const int param_num);

    void StatementList();

    void ReadStatement();
    void WriteStatement();
    void ReturnStatement();

    void ErrorHandler(const char cls);

public:

    Parser(Lexer& lexer, SymbolTable& symbol_table, Error& error, Intermediate& intermediate, bool print_info = false, std::vector<std::string>* output = nullptr);

    void Program();

};

#include <stdexcept>

#include "Parser.h"

void Parser::ConstDeclaration() {
    do {
        lexer_.ProcessNextSymbol();
        ConstDefinition();
        if (lexer_.get_symbol() == LexicalSymbol::SEMICN) lexer_.ProcessNextSymbol();
        else ErrorHandler('k');
    } while (lexer_.get_symbol() == LexicalSymbol::CONSTTK);
    if (print_info_) output_->emplace_back("<常量说明>");
}

void Parser::ConstDefinition() {
    type_ = lexer_.get_symbol();
    lexer_.ProcessNextSymbol();

    while (true) {
        name_ = lexer_.get_lower_token();
        lexer_.ProcessNextSymbol();
        lexer_.ProcessNextSymbol();

        if (lexer_.get_symbol() == LexicalSymbol::CHARCON) {
            if (!symbol_table_.AddSymbol(
                cur_func_, name_, SymbolType::CHAR, SymbolClass::CONST, lexer_.get_token()[0])) {
                ErrorHandler('b');
            }
            lexer_.ProcessNextSymbol();
        }
        else {
            if (!symbol_table_.AddSymbol(
                cur_func_, name_, SymbolType::INT, SymbolClass::CONST, Integer())) {
                ErrorHandler('b');
            }
        }

        if (lexer_.get_symbol() != LexicalSymbol::COMMA) break;
        lexer_.ProcessNextSymbol();
    }
    if (print_info_) output_->emplace_back("<常量定义>");
}

int Parser::Integer() {
    int ret = 1;
    if (lexer_.get_symbol() == LexicalSymbol::PLUS) lexer_.ProcessNextSymbol();
    else if (lexer_.get_symbol() == LexicalSymbol::MINU) {
        ret = -1;
        lexer_.ProcessNextSymbol();
    }

    ret *= lexer_.GetTokenToInt();

    lexer_.ProcessNextSymbol();
    if (print_info_) {
        output_->emplace_back("<无符号整数>");
        output_->emplace_back("<整数>");
    }
    return ret;
}

void Parser::VarDeclaration() {
    int insert_pos = 0;
    while (true) {
        VarDefinition();

        if (lexer_.get_symbol() != LexicalSymbol::SEMICN) ErrorHandler('k');
        else lexer_.ProcessNextSymbol();

        if (!cur_func_.empty()) {
            if (lexer_.get_symbol() != LexicalSymbol::INTTK &&
                lexer_.get_symbol() != LexicalSymbol::CHARTK) {
                if (print_info_) output_->emplace_back("<变量说明>");
                return;
            }
        }
        else if (print_info_) insert_pos = output_->size();

        type_ = lexer_.get_symbol();
        lexer_.ProcessNextSymbol();

        name_ = lexer_.get_lower_token();
        lexer_.ProcessNextSymbol();
        
        if (cur_func_.empty()) {
            if (!(lexer_.get_symbol() == LexicalSymbol::ASSIGN ||
                    lexer_.get_symbol() == LexicalSymbol::LBRACK ||
                    lexer_.get_symbol() == LexicalSymbol::SEMICN ||
                    lexer_.get_symbol() == LexicalSymbol::COMMA)) {
                break;
            }
        }  
    }
    if (print_info_) {
        output_->insert(output_->begin() + insert_pos, "<变量说明>");
    }
}

void Parser::VarDefinition() {
    ParseArrayInfo();
    if (lexer_.get_symbol() == LexicalSymbol::ASSIGN) VarDefinitionInitialized();
    else VarDefinitionUninitialized();
    if (print_info_) output_->emplace_back("<变量定义>");
}

void Parser::VarDefinitionUninitialized() {
    if (!symbol_table_.AddSymbol(
        cur_func_, name_, type_ == LexicalSymbol::INTTK ? SymbolType::INT : SymbolType::CHAR,
        SymbolClass::VAR, dim0_ == 0 ? 0 : -1, dim0_, dim1_)) {
        ErrorHandler('b');
    }
    else {
        if (dim0_ == 0) var_size_ += 4;
        else if (dim1_ == 0) var_size_ += dim0_ << 2;
        else var_size_ += (dim0_ * dim1_) << 2;
    }
    while (lexer_.get_symbol() == LexicalSymbol::COMMA) {
        lexer_.ProcessNextSymbol();

        name_ = lexer_.get_lower_token();
        lexer_.ProcessNextSymbol();

        ParseArrayInfo();

        if (!symbol_table_.AddSymbol(
            cur_func_, name_, type_ == LexicalSymbol::INTTK ? SymbolType::INT : SymbolType::CHAR,
            SymbolClass::VAR, dim0_ == 0 ? 0 : -1, dim0_, dim1_)) {
            ErrorHandler('b');
        }
        else {
            if (dim0_ == 0) var_size_ += 4;
            else if (dim1_ == 0) var_size_ += dim0_ << 2;
            else var_size_ += (dim0_ * dim1_) << 2;
        }
    }
    if (print_info_) output_->emplace_back("<变量定义无初始化>");
}

void Parser::VarDefinitionInitialized() {
    if (!symbol_table_.AddSymbol(
        cur_func_, name_,
        type_ == LexicalSymbol::CHARTK ? SymbolType::CHAR : SymbolType::INT,
        SymbolClass::VAR, symbol_table_.GetArrayInitializersSize(), dim0_, dim1_, true)) {
        ErrorHandler('b');
    }
    auto entry = symbol_table_.LookupSymbol(cur_func_, name_).first;

    lexer_.ProcessNextSymbol();

    if (dim0_ == 0) {
        if (lexer_.get_symbol() == LexicalSymbol::CHARCON) {
            if (type_ != LexicalSymbol::CHARTK) ErrorHandler('o');
            entry->val = lexer_.get_token()[0];
            lexer_.ProcessNextSymbol();
        }
        else {
            if (type_ != LexicalSymbol::INTTK) ErrorHandler('o');
            entry->val = Integer();
        }
        if (print_info_) output_->emplace_back("<常量>");
        var_size_ += 4;
    }
    else if (dim1_ == 0) {
        lexer_.ProcessNextSymbol();

        std::vector<int> initializer;
        int i = 0;
        while (lexer_.get_prev_symbol() != LexicalSymbol::RBRACE) {
            if (lexer_.get_symbol() == LexicalSymbol::CHARCON) {
                if (type_ != LexicalSymbol::CHARTK) ErrorHandler('o');
                initializer.emplace_back(lexer_.get_token()[0]);
                lexer_.ProcessNextSymbol();
            }
            else {
                if (type_ != LexicalSymbol::INTTK) ErrorHandler('o');
                initializer.emplace_back(Integer());
            }
            if (print_info_) output_->emplace_back("<常量>");
            lexer_.ProcessNextSymbol();
            i++;
        }
        if (i != dim0_) ErrorHandler('n');
        symbol_table_.AddArrayInitializer(initializer);
        var_size_ += dim0_ << 2;
    }
    else {
        lexer_.ProcessNextSymbol();

        std::vector<int> initializer;
        int i = 0;
        while (lexer_.get_prev_symbol() != LexicalSymbol::RBRACE) {
            lexer_.ProcessNextSymbol();

            int j = 0;
            while (lexer_.get_prev_symbol() != LexicalSymbol::RBRACE) {
                if (lexer_.get_symbol() == LexicalSymbol::CHARCON) {
                    if (type_ != LexicalSymbol::CHARTK) ErrorHandler('o');
                    initializer.emplace_back(lexer_.get_token()[0]);
                    lexer_.ProcessNextSymbol();
                }
                else {
                    if (type_ != LexicalSymbol::INTTK) ErrorHandler('o');
                    initializer.emplace_back(Integer());
                }
                if (print_info_) output_->emplace_back("<常量>");
                lexer_.ProcessNextSymbol();
                j++;
            }
            if (j != dim0_) ErrorHandler('n');

            lexer_.ProcessNextSymbol();
            i++;
        }
        if (i != dim1_) ErrorHandler('n');
        symbol_table_.AddArrayInitializer(initializer);
        var_size_ += (dim0_ * dim1_) << 2;
    }
    if (print_info_) output_->emplace_back("<变量定义及初始化>");
}

void Parser::ParseArrayInfo() {
    dim0_ = 0;
    dim1_ = 0;
    if (lexer_.get_symbol() == LexicalSymbol::LBRACK) {
        lexer_.ProcessNextSymbol();

        if (lexer_.get_symbol() != LexicalSymbol::INTCON) ErrorHandler('i');
        else dim0_ = lexer_.GetTokenToInt();
        lexer_.ProcessNextSymbol();

        if (print_info_ && dim0_ > 0) output_->emplace_back("<无符号整数>");

        if (lexer_.get_symbol() != LexicalSymbol::RBRACK) ErrorHandler('m');
        else lexer_.ProcessNextSymbol();

        if (lexer_.get_symbol() == LexicalSymbol::LBRACK) {
            lexer_.ProcessNextSymbol();

            if (lexer_.get_symbol() != LexicalSymbol::INTCON) ErrorHandler('i');
            else {
                dim1_ = dim0_;
                dim0_ = lexer_.GetTokenToInt();
            }
            lexer_.ProcessNextSymbol();

            if (print_info_ && dim0_ > 0) output_->emplace_back("<无符号整数>");

            if (lexer_.get_symbol() != LexicalSymbol::RBRACK) ErrorHandler('m');
            else lexer_.ProcessNextSymbol();
        }
    }
}

void Parser::FunDefinitionWithRet() {
    ParseFuncBody();
    if (print_info_) output_->emplace_back("<有返回值函数定义>");
}

void Parser::FunDefinitionWithoutRet() {
    ParseFuncBody();
    if (print_info_) output_->emplace_back("<无返回值函数定义>");
}

void Parser::ParseFuncBody() {
    has_ret_ = false;
    lexer_.ProcessNextSymbol();

    int param_num = ParamList();
    var_size_ += param_num << 2;
    auto entry = symbol_table_.LookupSymbol("", cur_func_);
    entry.first->val = param_num;

    if (lexer_.get_symbol() != LexicalSymbol::RPARENT) ErrorHandler('l');
    else lexer_.ProcessNextSymbol();

    lexer_.ProcessNextSymbol();

    CompoundStatement();

    if (entry.first->type != SymbolType::VOID && !has_ret_) ErrorHandler('h');

    lexer_.ProcessNextSymbol();
}

void Parser::CompoundStatement() {
    if (lexer_.get_symbol() == LexicalSymbol::CONSTTK) ConstDeclaration();

    if (lexer_.get_symbol() == LexicalSymbol::INTTK ||
        lexer_.get_symbol() == LexicalSymbol::CHARTK) {
        type_ = lexer_.get_symbol();
        lexer_.ProcessNextSymbol();

        name_ = lexer_.get_lower_token();
        lexer_.ProcessNextSymbol();

        VarDeclaration();
    }

    StatementList();

    if (print_info_) output_->emplace_back("<复合语句>");
}

int Parser::ParamList() {
    int ret = 0;
    dim0_ = 0;
    dim1_ = 0;
    while (lexer_.get_symbol() != LexicalSymbol::RPARENT &&
        lexer_.get_symbol() != LexicalSymbol::LBRACE) {
        type_ = lexer_.get_symbol();
        lexer_.ProcessNextSymbol();

        name_ = lexer_.get_lower_token();
        if (!symbol_table_.AddSymbol(
            cur_func_, name_,
            type_ == LexicalSymbol::INTTK ? SymbolType::INT : SymbolType::CHAR,
            SymbolClass::PARAM, ret)) {
            ErrorHandler('b');
        }
        else ret++;
        lexer_.ProcessNextSymbol();

        if (lexer_.get_symbol() == LexicalSymbol::COMMA) lexer_.ProcessNextSymbol();
    }
    if (print_info_) output_->emplace_back("<参数表>");
    return ret;
}

void Parser::MainFun() {
    lexer_.ProcessNextSymbol();

    if (lexer_.get_symbol() != LexicalSymbol::RPARENT) ErrorHandler('l');
    else lexer_.ProcessNextSymbol();

    lexer_.ProcessNextSymbol();

    CompoundStatement();

    lexer_.ProcessNextSymbol();
    if (print_info_) output_->emplace_back("<主函数>");
}

std::pair<SymbolType, std::string> Parser::Expression() {
    SymbolType ret_type = SymbolType::CHAR;
    bool is_negative = false;
    if (lexer_.get_symbol() == LexicalSymbol::PLUS) {
        ret_type = SymbolType::INT;
        lexer_.ProcessNextSymbol();
    }
    else if (lexer_.get_symbol() == LexicalSymbol::MINU) {
        ret_type = SymbolType::INT;
        lexer_.ProcessNextSymbol();
        is_negative = true;
    }

    auto term_ret = Term();
    if (term_ret.first == SymbolType::INT) ret_type = SymbolType::INT;
    auto ret = term_ret.second;
    if (is_negative) {
        auto tmp = intermediate_.GenTmp(cur_func_, SymbolType::INT);
        intermediate_.AddIntermediateCode(tmp, IntermediateOp::SUB, "0", ret);
        ret = tmp;
    }
    else if (ret_type != term_ret.first) {
        auto tmp = intermediate_.GenTmp(cur_func_, SymbolType::INT);
        intermediate_.AddIntermediateCode(tmp, IntermediateOp::ADD, ret, "0");
        ret = tmp;
    }

    while (lexer_.get_symbol() == LexicalSymbol::PLUS ||
        lexer_.get_symbol() == LexicalSymbol::MINU) {
        ret_type = SymbolType::INT;
        auto tmp = intermediate_.GenTmp(cur_func_, SymbolType::INT);
        if (lexer_.get_symbol() == LexicalSymbol::PLUS) {
            lexer_.ProcessNextSymbol();
            term_ret = Term();
            intermediate_.AddIntermediateCode(tmp, IntermediateOp::ADD, ret, term_ret.second);
        }
        else {
            lexer_.ProcessNextSymbol();
            term_ret = Term();
            intermediate_.AddIntermediateCode(tmp, IntermediateOp::SUB, ret, term_ret.second);
        }
        ret = tmp;
    }
    if (print_info_) output_->emplace_back("<表达式>");
    return std::make_pair(ret_type, ret);
}

std::pair<SymbolType, std::string> Parser::Term() {
    SymbolType ret_type;

    auto factor_ret = Factor();
    ret_type = factor_ret.first;
    auto ret = factor_ret.second;

    while (lexer_.get_symbol() == LexicalSymbol::MULT ||
        lexer_.get_symbol() == LexicalSymbol::DIV) {
        ret_type = SymbolType::INT;
        auto tmp = intermediate_.GenTmp(cur_func_, SymbolType::INT);
        if (lexer_.get_symbol() == LexicalSymbol::MULT) {
            lexer_.ProcessNextSymbol();
            factor_ret = Factor();
            intermediate_.AddIntermediateCode(tmp, IntermediateOp::MUL, ret, factor_ret.second);
        }
        else {
            lexer_.ProcessNextSymbol();
            factor_ret = Factor();
            intermediate_.AddIntermediateCode(tmp, IntermediateOp::DIV, ret, factor_ret.second);
        }
        ret = tmp;
    }
    if (print_info_) output_->emplace_back("<项>");
    return std::make_pair(ret_type, ret);
}

std::pair<SymbolType, std::string> Parser::Factor() {
    SymbolType ret_type = SymbolType::CHAR;
    std::string ret;
    std::string array_name;
    std::string array_index;
    std::string tmp;

    std::pair<TableEntry*, bool> table_entry;
    std::pair<SymbolType, std::string> exp_ret;

    switch (lexer_.get_symbol()) {
    case LexicalSymbol::IDENFR:
        name_ = lexer_.get_lower_token();
        lexer_.ProcessNextSymbol();

        switch (lexer_.get_symbol()) {
        case LexicalSymbol::LPARENT:
            exp_ret = CallFunc();
            ret_type = exp_ret.first;
            ret = exp_ret.second;
            break;
        case LexicalSymbol::LBRACK:
            array_name = name_;
            table_entry = symbol_table_.LookupSymbol(cur_func_, name_);
            if (table_entry.first == nullptr) ErrorHandler('c');
            else ret_type = table_entry.first->type;
            lexer_.ProcessNextSymbol();

            ret = intermediate_.GenTmp(cur_func_, ret_type);

            exp_ret = Expression();
            if (exp_ret.first != SymbolType::INT) ErrorHandler('i');
            array_index = exp_ret.second;

            if (lexer_.get_symbol() != LexicalSymbol::RBRACK) ErrorHandler('m');
            else lexer_.ProcessNextSymbol();

            if (lexer_.get_symbol() == LexicalSymbol::LBRACK) {
                lexer_.ProcessNextSymbol();

                exp_ret = Expression();
                if (exp_ret.first != SymbolType::INT) ErrorHandler('i');
                tmp = intermediate_.GenTmp(cur_func_, SymbolType::INT);
                intermediate_.AddIntermediateCode(tmp, IntermediateOp::MUL,
                    array_index, std::to_string(table_entry.first->dim0));
                intermediate_.AddIntermediateCode(tmp, IntermediateOp::ADD, tmp, exp_ret.second);
                array_index = tmp;

                if (lexer_.get_symbol() != LexicalSymbol::RBRACK) ErrorHandler('m');
                else lexer_.ProcessNextSymbol();
            }

            intermediate_.AddIntermediateCode(array_name, IntermediateOp::ARR_LOAD, array_index, ret);

            break;
        default:
            table_entry = symbol_table_.LookupSymbol(cur_func_, name_);
            if (table_entry.first == nullptr) ErrorHandler('c');
            else {
                ret_type = table_entry.first->type;
                ret = name_;
            }
            break;
        }
        break;
    case LexicalSymbol::LPARENT:
        ret_type = SymbolType::INT;
        lexer_.ProcessNextSymbol();

        exp_ret = Expression();
        ret = exp_ret.second;

        if (ret_type != exp_ret.first) {
            auto tmp = intermediate_.GenTmp(cur_func_, SymbolType::INT);
            intermediate_.AddIntermediateCode(tmp, IntermediateOp::ADD, ret, "0");
            ret = tmp;
        }

        if (lexer_.get_symbol() != LexicalSymbol::RPARENT) ErrorHandler('l');
        else lexer_.ProcessNextSymbol();

        break;
    case LexicalSymbol::PLUS:
    case LexicalSymbol::MINU:
    case LexicalSymbol::INTCON:
        ret_type = SymbolType::INT;
        ret = std::to_string(Integer());
        break;
    case LexicalSymbol::CHARCON:
        ret = intermediate_.GenTmp(cur_func_, SymbolType::CHAR);
        intermediate_.AddIntermediateCode(ret, IntermediateOp::ADD,
            "0", std::to_string(int(lexer_.get_token()[0])));
        lexer_.ProcessNextSymbol();
        break;
    default:
        break;
    }
    if (print_info_) output_->emplace_back("<因子>");
    return std::make_pair(ret_type, ret);
}

void Parser::Statement() {
    std::pair<TableEntry*, bool> table_entry;
    switch (lexer_.get_symbol()) {
    case LexicalSymbol::WHILETK:
    case LexicalSymbol::FORTK:
        LoopStatement();
        break;
    case LexicalSymbol::IFTK:
        ConditionalStatement();
        break;
    case LexicalSymbol::IDENFR:
        name_ = lexer_.get_lower_token();
        lexer_.ProcessNextSymbol();

        switch (lexer_.get_symbol()) {
        case LexicalSymbol::LPARENT:
            CallFunc();

            if (lexer_.get_symbol() != LexicalSymbol::SEMICN) ErrorHandler('k');
            else lexer_.ProcessNextSymbol();
            break;
        case LexicalSymbol::ASSIGN:
        case LexicalSymbol::LBRACK:
            table_entry = symbol_table_.LookupSymbol(cur_func_, name_);
            if (table_entry.first == nullptr) ErrorHandler('c');
            else if (table_entry.first->cls == SymbolClass::CONST) ErrorHandler('j');

            AssignStatement(table_entry.first);

            if (lexer_.get_symbol() != LexicalSymbol::SEMICN) ErrorHandler('k');
            else lexer_.ProcessNextSymbol();
            break;
        default:
            break;
        }
        break;
    case LexicalSymbol::SCANFTK:
        ReadStatement();

        if (lexer_.get_symbol() != LexicalSymbol::SEMICN) ErrorHandler('k');
        else lexer_.ProcessNextSymbol();
        break;
    case LexicalSymbol::PRINTFTK:
        WriteStatement();

        if (lexer_.get_symbol() != LexicalSymbol::SEMICN) ErrorHandler('k');
        else lexer_.ProcessNextSymbol();
        break;
    case LexicalSymbol::SWITCHTK:
        CaseStatement();
        break;
    case LexicalSymbol::SEMICN:
        lexer_.ProcessNextSymbol();
        break;
    case LexicalSymbol::RETURNTK:
        ReturnStatement();

        if (lexer_.get_symbol() != LexicalSymbol::SEMICN) ErrorHandler('k');
        else lexer_.ProcessNextSymbol();
        break;
    case LexicalSymbol::LBRACE:
        lexer_.ProcessNextSymbol();

        StatementList();

        lexer_.ProcessNextSymbol();
        break;
    default:
        break;
    }
    if (print_info_) output_->emplace_back("<语句>");
}

void Parser::AssignStatement(TableEntry* table_entry) {
    std::string dst = name_;
    std::pair<SymbolType, std::string> exp_ret;
    bool is_array = false;
    std::string array_index;
    if (lexer_.get_symbol() == LexicalSymbol::LBRACK) {
        is_array = true;
        lexer_.ProcessNextSymbol();

        exp_ret = Expression();
        if (exp_ret.first != SymbolType::INT) ErrorHandler('i');
        array_index = exp_ret.second;

        if (lexer_.get_symbol() != LexicalSymbol::RBRACK) ErrorHandler('m');
        else lexer_.ProcessNextSymbol();

        if (lexer_.get_symbol() == LexicalSymbol::LBRACK) {
            lexer_.ProcessNextSymbol();

            exp_ret = Expression();
            if (exp_ret.first != SymbolType::INT) ErrorHandler('i');
            auto tmp = intermediate_.GenTmp(cur_func_, SymbolType::INT);
            intermediate_.AddIntermediateCode(tmp, IntermediateOp::MUL,
                array_index, std::to_string(table_entry->dim0));
            intermediate_.AddIntermediateCode(tmp, IntermediateOp::ADD, tmp, exp_ret.second);
            array_index = tmp;

            if (lexer_.get_symbol() != LexicalSymbol::RBRACK) ErrorHandler('m');
            else lexer_.ProcessNextSymbol();
        }
    }
    lexer_.ProcessNextSymbol();

    if (is_array) intermediate_.AddIntermediateCode(dst, IntermediateOp::ARR_STORE, array_index, Expression().second);
    else intermediate_.AddIntermediateCode(dst, IntermediateOp::ADD, Expression().second, "0");

    if (print_info_) output_->emplace_back("<赋值语句>");
}

void Parser::ConditionalStatement() {
    lexer_.ProcessNextSymbol();
    lexer_.ProcessNextSymbol();

    auto skip_label = intermediate_.GenLabel();
    Condition(skip_label);

    if (lexer_.get_symbol() != LexicalSymbol::RPARENT) ErrorHandler('l');
    else lexer_.ProcessNextSymbol();

    Statement();

    if (lexer_.get_symbol() == LexicalSymbol::ELSETK) {
        auto end_label = intermediate_.GenLabel();
        intermediate_.AddIntermediateCode(end_label, IntermediateOp::GOTO, "", "");
        intermediate_.AddIntermediateCode(skip_label, IntermediateOp::LABEL, "", "");
        lexer_.ProcessNextSymbol();

        Statement();
        intermediate_.AddIntermediateCode(end_label, IntermediateOp::LABEL, "", "");
    }
    else intermediate_.AddIntermediateCode(skip_label, IntermediateOp::LABEL, "", "");
    if (print_info_) output_->emplace_back("<条件语句>");
}

void Parser::Condition(const std::string& skip_label) {
    auto exp_ret1 = Expression();
    if (exp_ret1.first != SymbolType::INT) ErrorHandler('f');

    auto symbol = lexer_.get_symbol();
    lexer_.ProcessNextSymbol();

    auto exp_ret2 = Expression();
    if(exp_ret2.first != SymbolType::INT) ErrorHandler('f');

    switch (symbol) {
    case LexicalSymbol::LSS:
        intermediate_.AddIntermediateCode(skip_label, IntermediateOp::BGE, exp_ret1.second, exp_ret2.second);
        break;
    case LexicalSymbol::LEQ:
        intermediate_.AddIntermediateCode(skip_label, IntermediateOp::BGT, exp_ret1.second, exp_ret2.second);
        break;
    case LexicalSymbol::GRE:
        intermediate_.AddIntermediateCode(skip_label, IntermediateOp::BLE, exp_ret1.second, exp_ret2.second);
        break;
    case LexicalSymbol::GEQ:
        intermediate_.AddIntermediateCode(skip_label, IntermediateOp::BLT, exp_ret1.second, exp_ret2.second);
        break;
    case LexicalSymbol::EQL:
        intermediate_.AddIntermediateCode(skip_label, IntermediateOp::BNE, exp_ret1.second, exp_ret2.second);
        break;
    case LexicalSymbol::NEQ:
        intermediate_.AddIntermediateCode(skip_label, IntermediateOp::BEQ, exp_ret1.second, exp_ret2.second);
        break;
    default:
        break;
    }

    if (print_info_) output_->emplace_back("<条件>");
}

void Parser::LoopStatement() {
    IntermediateCode update_var;
    update_var.dst = "";

    auto begin_label = intermediate_.GenLabel();
    auto skip_label = intermediate_.GenLabel();

    if (lexer_.get_symbol() == LexicalSymbol::WHILETK) {
        lexer_.ProcessNextSymbol();
        lexer_.ProcessNextSymbol();

        intermediate_.AddIntermediateCode(begin_label, IntermediateOp::LABEL, "", "");
        Condition(skip_label);
    }
    else {
        lexer_.ProcessNextSymbol();
        lexer_.ProcessNextSymbol();

        name_ = lexer_.get_lower_token();
        auto table_entry = symbol_table_.LookupSymbol(cur_func_, name_);
        if (table_entry.first == nullptr) ErrorHandler('c');
        else if (table_entry.first->cls == SymbolClass::CONST) ErrorHandler('j');
        auto var_name = name_;
        lexer_.ProcessNextSymbol();

        lexer_.ProcessNextSymbol();

        intermediate_.AddIntermediateCode(var_name, IntermediateOp::ADD, Expression().second, "0");

        if (lexer_.get_symbol() != LexicalSymbol::SEMICN) ErrorHandler('k');
        else lexer_.ProcessNextSymbol();

        intermediate_.AddIntermediateCode(begin_label, IntermediateOp::LABEL, "", "");
        Condition(skip_label);
        if (lexer_.get_symbol() != LexicalSymbol::SEMICN) ErrorHandler('k');
        else lexer_.ProcessNextSymbol();

        name_ = lexer_.get_lower_token();
        table_entry = symbol_table_.LookupSymbol(cur_func_, name_);
        if (table_entry.first == nullptr) ErrorHandler('c');
        else if (table_entry.first->cls == SymbolClass::CONST) ErrorHandler('j');
        update_var.dst = name_;
        lexer_.ProcessNextSymbol();

        lexer_.ProcessNextSymbol();

        name_ = lexer_.get_lower_token();
        table_entry = symbol_table_.LookupSymbol(cur_func_, name_);
        if (table_entry.first == nullptr) ErrorHandler('c');
        update_var.src1 = name_;
        lexer_.ProcessNextSymbol();

        if (lexer_.get_symbol() == LexicalSymbol::PLUS) update_var.op = IntermediateOp::ADD;
        else update_var.op = IntermediateOp::SUB;
        lexer_.ProcessNextSymbol();

        int step = lexer_.GetTokenToInt();
        update_var.src2 = std::to_string(step);
        lexer_.ProcessNextSymbol();

        if (print_info_) output_->emplace_back("<无符号整数>");
        if (print_info_) output_->emplace_back("<步长>");
    }
    if (lexer_.get_symbol() != LexicalSymbol::RPARENT) ErrorHandler('l');
    else lexer_.ProcessNextSymbol();
    
    Statement();
    if (!update_var.dst.empty()) intermediate_.AddIntermediateCode(update_var);

    intermediate_.AddIntermediateCode(begin_label, IntermediateOp::GOTO, "", "");
    intermediate_.AddIntermediateCode(skip_label, IntermediateOp::LABEL, "", "");

    if (print_info_) output_->emplace_back("<循环语句>");
}

void Parser::CaseStatement() {
    lexer_.ProcessNextSymbol();
    lexer_.ProcessNextSymbol();

    auto exp_ret = Expression();

    if (lexer_.get_symbol() != LexicalSymbol::RPARENT) ErrorHandler('l');
    else lexer_.ProcessNextSymbol();

    lexer_.ProcessNextSymbol();

    auto skip_label = intermediate_.GenLabel();
    CaseTable(exp_ret, skip_label);

    DefaultCase();
    intermediate_.AddIntermediateCode(skip_label, IntermediateOp::LABEL, "", "");

    lexer_.ProcessNextSymbol();

    if (print_info_) output_->emplace_back("<情况语句>");
}

void Parser::CaseTable(const std::pair<SymbolType, std::string>& exp, const std::string& skip_label) {
    std::string label;
    while (lexer_.get_symbol() == LexicalSymbol::CASETK) {
        label = CaseSubstatement(exp, skip_label);
        intermediate_.AddIntermediateCode(label, IntermediateOp::LABEL, "", "");
    }
    if (print_info_) output_->emplace_back("<情况表>");
}

std::string Parser::CaseSubstatement(const std::pair<SymbolType, std::string>& exp, const std::string& skip_label) {
    auto ret = intermediate_.GenLabel();
    int con = 0;
    
    lexer_.ProcessNextSymbol();

    if (lexer_.get_symbol() != LexicalSymbol::CHARCON) {
        if (exp.first != SymbolType::INT) ErrorHandler('o');
        con = Integer();
    }
    else {
        if (exp.first != SymbolType::CHAR) ErrorHandler('o');
        else con = lexer_.get_token()[0];
        lexer_.ProcessNextSymbol();
    }
    if (print_info_) output_->emplace_back("<常量>");

    lexer_.ProcessNextSymbol();

    intermediate_.AddIntermediateCode(ret, IntermediateOp::BNE, exp.second, std::to_string(con));
    Statement();
    intermediate_.AddIntermediateCode(skip_label, IntermediateOp::GOTO, "", "");

    if (print_info_) output_->emplace_back("<情况子语句>");
    return ret;
}

void Parser::DefaultCase() {
    if (lexer_.get_symbol() != LexicalSymbol::DEFAULTTK) ErrorHandler('p');
    else {
        lexer_.ProcessNextSymbol();
        lexer_.ProcessNextSymbol();

        Statement();

        if (print_info_) output_->emplace_back("<缺省>");
    }
}

std::pair<SymbolType, std::string> Parser::CallFunc() {
    symbol_table_.LookupSymbol("", cur_func_).first->dim1 = 1;

    auto func_name = name_;
    SymbolType ret_type = SymbolType::CHAR;
    std::string ret = "";

    auto it = symbol_table_.LookupSymbol("", func_name);
    if (it.first == nullptr) ErrorHandler('c');
    else ret_type = it.first->type;

    lexer_.ProcessNextSymbol();

    auto param_list = ValParamList(func_name, it.first != nullptr ? it.first->val : 0);

    if (lexer_.get_symbol() != LexicalSymbol::RPARENT) ErrorHandler('l');
    else lexer_.ProcessNextSymbol();
    
    if (ret_type != SymbolType::VOID) ret = intermediate_.GenTmp(cur_func_, ret_type);
    for (auto& param : param_list) {
        intermediate_.AddIntermediateCode(param, IntermediateOp::PUSH, "", "");
    }
    intermediate_.AddIntermediateCode(ret, IntermediateOp::CALL, func_name, "");

    if (print_info_) {
        if (ret_type != SymbolType::VOID) output_->emplace_back("<有返回值函数调用语句>");
        else output_->emplace_back("<无返回值函数调用语句>");
    }

    return std::make_pair(ret_type, ret);
}

std::vector<std::string> Parser::ValParamList(const std::string func_name, const int param_num) {
    std::vector<std::string> ret;
    while (true) {
        SymbolType ret_type = SymbolType::CHAR;
        unsigned long long prev_cnt = lexer_.get_cnt();
        auto exp_ret = Expression();
        if (lexer_.get_cnt() == prev_cnt) break;

        if (ret.size() < param_num) {
            ret_type = symbol_table_.GetKthParam(func_name, ret.size())->type;
            if (exp_ret.first != ret_type) ErrorHandler('e');
        }
        try {
            std::stoi(exp_ret.second);
            ret.emplace_back(exp_ret.second);
        }
        catch (std::invalid_argument& e) {
            auto entry = symbol_table_.LookupSymbol(cur_func_, exp_ret.second);
            if (entry.first->cls != SymbolClass::CONST && exp_ret.second[0] != 'T') {
                auto tmp = intermediate_.GenTmp(cur_func_, ret_type);
                intermediate_.AddIntermediateCode(tmp, IntermediateOp::ADD, exp_ret.second, "0");
                ret.emplace_back(tmp);
            }
            else ret.emplace_back(exp_ret.second);
        }

        if (lexer_.get_symbol() == LexicalSymbol::COMMA) {
            lexer_.ProcessNextSymbol();
        }
        else break;
    }
    if (ret.size() != param_num) ErrorHandler('d');
    if (print_info_) output_->emplace_back("<值参数表>");
    return ret;
}

void Parser::StatementList() {
    while (lexer_.get_symbol() != LexicalSymbol::RBRACE) {
        Statement();
    }
    if (print_info_) output_->emplace_back("<语句列>");
}

void Parser::ReadStatement() {
    lexer_.ProcessNextSymbol();
    lexer_.ProcessNextSymbol();

    name_ = lexer_.get_lower_token();
    auto table_entry = symbol_table_.LookupSymbol(cur_func_, name_);
    if (table_entry.first == nullptr) ErrorHandler('c');
    else if (table_entry.first->cls == SymbolClass::CONST) ErrorHandler('j');
    intermediate_.AddIntermediateCode(name_, IntermediateOp::SCAN, "", "");
    lexer_.ProcessNextSymbol();

    if (lexer_.get_symbol() != LexicalSymbol::RPARENT) ErrorHandler('l');
    else lexer_.ProcessNextSymbol();

    if (print_info_) output_->emplace_back("<读语句>");
}

void Parser::WriteStatement() {
    lexer_.ProcessNextSymbol();
    lexer_.ProcessNextSymbol();

    if (lexer_.get_symbol() == LexicalSymbol::STRCON) {
        int num = symbol_table_.AddString(lexer_.get_token());
        lexer_.ProcessNextSymbol();
        if (print_info_) output_->emplace_back("<字符串>");

        std::string exp;
        if (lexer_.get_symbol() == LexicalSymbol::COMMA) {
            lexer_.ProcessNextSymbol();

            exp = Expression().second;
        }
        intermediate_.AddIntermediateCode("", IntermediateOp::PRINT, "S" + std::to_string(num), exp);
    }
    else intermediate_.AddIntermediateCode("", IntermediateOp::PRINT, Expression().second, "");

    if (lexer_.get_symbol() != LexicalSymbol::RPARENT) ErrorHandler('l');
    else lexer_.ProcessNextSymbol();

    if (print_info_) output_->emplace_back("<写语句>");
}

void Parser::ReturnStatement() {
    lexer_.ProcessNextSymbol();
    auto cur_func_entry = symbol_table_.LookupSymbol("", cur_func_);
    std::pair<SymbolType, std::string> exp_ret;

    if (lexer_.get_symbol() == LexicalSymbol::LPARENT) {
        if (cur_func_entry.first->type == SymbolType::VOID) ErrorHandler('g');
        lexer_.ProcessNextSymbol();

        unsigned long long prev_cnt = lexer_.get_cnt();
        exp_ret = Expression();
        if (lexer_.get_cnt() != prev_cnt) {
            if (cur_func_entry.first->type != SymbolType::VOID &&
                exp_ret.first != cur_func_entry.first->type) {
                ErrorHandler('h');
            }
        }
        else if (cur_func_entry.first->type != SymbolType::VOID) ErrorHandler('h');
        intermediate_.AddIntermediateCode(exp_ret.second, IntermediateOp::RET, "", "");

        if (lexer_.get_symbol() != LexicalSymbol::RPARENT) ErrorHandler('l');
        else lexer_.ProcessNextSymbol();
    }
    else {
        if (cur_func_entry.first->type != SymbolType::VOID) ErrorHandler('h');
        if (cur_func_ == "main") {
            intermediate_.AddIntermediateCode("", IntermediateOp::EXIT, "", "");
        }
        else {
            intermediate_.AddIntermediateCode("", IntermediateOp::RET, "", "");
        }
    }
    has_ret_ = true;
    if (print_info_) output_->emplace_back("<返回语句>");
}

void Parser::ErrorHandler(const char cls) {
    if (cls == 'k') error_.LogError(lexer_.get_prev_line_no(), cls);
    else error_.LogError(lexer_.get_line_no(), cls);
    return;
}

Parser::Parser(Lexer& lexer, SymbolTable& symbol_table, Error& error, Intermediate& intermediate, bool print_info, std::vector<std::string>* output)
    :lexer_(lexer), symbol_table_(symbol_table), error_(error), intermediate_(intermediate), print_info_(print_info), output_(output), dim0_(0), dim1_(0), var_size_(0) {}

void Parser::Program() {
    lexer_.ProcessNextSymbol();

    if (lexer_.get_symbol() == LexicalSymbol::CONSTTK) ConstDeclaration();

    type_ = lexer_.get_symbol();
    lexer_.ProcessNextSymbol();

    name_ = lexer_.get_lower_token();
    lexer_.ProcessNextSymbol();

    if (lexer_.get_symbol() == LexicalSymbol::ASSIGN ||
            lexer_.get_symbol() == LexicalSymbol::LBRACK ||
            lexer_.get_symbol() == LexicalSymbol::SEMICN ||
            lexer_.get_symbol() == LexicalSymbol::COMMA) {
        VarDeclaration();
    }

    bool main_found = false;

    while (true) {
        switch (type_) {
        case LexicalSymbol::INTTK:
            if (print_info_) output_->emplace_back("<声明头部>");
            if (!symbol_table_.AddSymbol(
                cur_func_, name_, SymbolType::INT, SymbolClass::FUNCTION, 0)) {
                ErrorHandler('b');
            }
            cur_func_ = name_;
            var_size_ = 0;
            intermediate_.AddIntermediateCode(cur_func_, IntermediateOp::FUNC_BEGIN, "", "");
            FunDefinitionWithRet();
            intermediate_.AddIntermediateCode(cur_func_, IntermediateOp::FUNC_END, "", "");
            symbol_table_.LookupSymbol("", cur_func_).first->dim0 = var_size_;
            cur_func_.clear();
            break;
        case LexicalSymbol::CHARTK:
            if (print_info_) output_->emplace_back("<声明头部>");
            if (!symbol_table_.AddSymbol(
                cur_func_, name_, SymbolType::CHAR, SymbolClass::FUNCTION, 0)) {
                ErrorHandler('b');
            }
            cur_func_ = name_;
            var_size_ = 0;
            intermediate_.AddIntermediateCode(cur_func_, IntermediateOp::FUNC_BEGIN, "", "");
            FunDefinitionWithRet();
            intermediate_.AddIntermediateCode(cur_func_, IntermediateOp::FUNC_END, "", "");
            symbol_table_.LookupSymbol("", cur_func_).first->dim0 = var_size_;
            cur_func_.clear();
            break;
        case LexicalSymbol::VOIDTK:
            if (!symbol_table_.AddSymbol(
                cur_func_, name_, SymbolType::VOID, SymbolClass::FUNCTION, 0)) {
                ErrorHandler('b');
            }
            if (lexer_.get_prev_symbol() == LexicalSymbol::MAINTK) {
                main_found = true;
                cur_func_ = name_;
                var_size_ = 0;
                intermediate_.AddIntermediateCode(cur_func_, IntermediateOp::FUNC_BEGIN, "", "");
                MainFun();
                intermediate_.AddIntermediateCode(cur_func_, IntermediateOp::FUNC_END, "", "");
                symbol_table_.LookupSymbol("", cur_func_).first->dim0 = var_size_;
                cur_func_.clear();
            }
            else {
                cur_func_ = name_;
                var_size_ = 0;
                intermediate_.AddIntermediateCode(cur_func_, IntermediateOp::FUNC_BEGIN, "", "");
                FunDefinitionWithoutRet();
                intermediate_.AddIntermediateCode(cur_func_, IntermediateOp::FUNC_END, "", "");
                symbol_table_.LookupSymbol("", cur_func_).first->dim0 = var_size_;
                cur_func_.clear();
            }
            break;
        default:
            break;
        }

        if (main_found) break;

        type_ = lexer_.get_symbol();
        lexer_.ProcessNextSymbol();

        name_ = lexer_.get_lower_token();
        lexer_.ProcessNextSymbol();
    }

    if (print_info_) output_->emplace_back("<程序>");
}

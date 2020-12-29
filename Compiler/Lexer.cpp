#include <cctype>
#include <unordered_map>
#include <algorithm>

#include "Lexer.h"

const std::unordered_map<std::string, LexicalSymbol> str2reserved_name = {
    {"const", LexicalSymbol::CONSTTK},
    {"int", LexicalSymbol::INTTK},
    {"char", LexicalSymbol::CHARTK},
    {"void", LexicalSymbol::VOIDTK},
    {"main", LexicalSymbol::MAINTK},
    {"if", LexicalSymbol::IFTK},
    {"else", LexicalSymbol::ELSETK},
    {"switch", LexicalSymbol::SWITCHTK},
    {"case", LexicalSymbol::CASETK},
    {"default", LexicalSymbol::DEFAULTTK},
    {"while", LexicalSymbol::WHILETK},
    {"for", LexicalSymbol::FORTK},
    {"scanf", LexicalSymbol::SCANFTK},
    {"printf", LexicalSymbol::PRINTFTK},
    {"return", LexicalSymbol::RETURNTK}
};

const std::unordered_map<char, LexicalSymbol> char2symbol = {
    {'+', LexicalSymbol::PLUS},
    {'-', LexicalSymbol::MINU},
    {'*', LexicalSymbol::MULT},
    {'/', LexicalSymbol::DIV},
    {':', LexicalSymbol::COLON},
    {';', LexicalSymbol::SEMICN},
    {',', LexicalSymbol::COMMA},
    {'(', LexicalSymbol::LPARENT},
    {')', LexicalSymbol::RPARENT},
    {'[', LexicalSymbol::LBRACK},
    {']', LexicalSymbol::RBRACK},
    {'{', LexicalSymbol::LBRACE},
    {'}', LexicalSymbol::RBRACE}
};

const std::unordered_map<LexicalSymbol, std::string> symbol2str = {
    {LexicalSymbol::IDENFR, "IDENFR"},
    {LexicalSymbol::INTCON, "INTCON"},
    {LexicalSymbol::CHARCON, "CHARCON"},
    {LexicalSymbol::STRCON, "STRCON"},
    {LexicalSymbol::CONSTTK, "CONSTTK"},
    {LexicalSymbol::INTTK, "INTTK"},
    {LexicalSymbol::CHARTK, "CHARTK"},
    {LexicalSymbol::VOIDTK, "VOIDTK"},
    {LexicalSymbol::MAINTK, "MAINTK"},
    {LexicalSymbol::IFTK, "IFTK"},
    {LexicalSymbol::ELSETK, "ELSETK"},
    {LexicalSymbol::SWITCHTK, "SWITCHTK"},
    {LexicalSymbol::CASETK, "CASETK"},
    {LexicalSymbol::DEFAULTTK, "DEFAULTTK"},
    {LexicalSymbol::WHILETK, "WHILETK"},
    {LexicalSymbol::FORTK, "FORTK"},
    {LexicalSymbol::SCANFTK, "SCANFTK"},
    {LexicalSymbol::PRINTFTK, "PRINTFTK"},
    {LexicalSymbol::RETURNTK, "RETURNTK"},
    {LexicalSymbol::PLUS, "PLUS"},
    {LexicalSymbol::MINU, "MINU"},
    {LexicalSymbol::MULT, "MULT"},
    {LexicalSymbol::DIV, "DIV"},
    {LexicalSymbol::LSS, "LSS"},
    {LexicalSymbol::LEQ, "LEQ"},
    {LexicalSymbol::GRE, "GRE"},
    {LexicalSymbol::GEQ, "GEQ"},
    {LexicalSymbol::EQL, "EQL"},
    {LexicalSymbol::NEQ, "NEQ"},
    {LexicalSymbol::COLON, "COLON"},
    {LexicalSymbol::ASSIGN, "ASSIGN"},
    {LexicalSymbol::SEMICN, "SEMICN"},
    {LexicalSymbol::COMMA, "COMMA"},
    {LexicalSymbol::LPARENT, "LPARENT"},
    {LexicalSymbol::RPARENT, "RPARENT"},
    {LexicalSymbol::LBRACK, "LBRACK"},
    {LexicalSymbol::RBRACK, "RBRACK"},
    {LexicalSymbol::LBRACE, "LBRACE"},
    {LexicalSymbol::RBRACE, "RBRACE"}
};

LexicalSymbol Lexer::Token2ReservedName() const {
    std::string str;
    str.resize(token_.size());
    std::transform(token_.begin(), token_.end(), str.begin(), ::tolower);
    auto it = str2reserved_name.find(str);
    if (it != str2reserved_name.end()) return it->second;
    else return LexicalSymbol::IDENFR;
}

bool Lexer::ErrorHandler(bool do_next) {
    error_.LogError(line_no_, 'a');
    if (do_next) return ProcessNextSymbol();
    return false;
}

Lexer::Lexer(std::FILE* f, Error& error, bool print_info, std::vector<std::string>* output)
    :f_(f), error_(error), print_info_(print_info), output_(output), line_no_(1), symbol_(LexicalSymbol::END), cnt_(0) {}

bool Lexer::ProcessNextSymbol() {
    prev_line_no_ = line_no_;
    prev_token_ = token_;
    prev_symbol_ = symbol_;
    token_.clear();
    char c = std::fgetc(f_);
    while (std::isspace(c)) {
        if (c == '\n') line_no_++;
        c = std::fgetc(f_);
    }
    if (c == EOF) {
        if (print_info_ && prev_symbol_ != LexicalSymbol::END) {
            output_->emplace_back(symbol2str.find(prev_symbol_)->second + ' ' + prev_token_);
        }
        return false;
    }
    else if (std::isalpha(c) || c == '_') {
        while (std::isalnum(c) || c == '_') {
            token_ += c;
            c = std::fgetc(f_);
        }
        if (c != EOF) std::ungetc(c, f_);
        symbol_ = Token2ReservedName();
    }
    else if (std::isdigit(c)) {
        while (std::isdigit(c)) {
            token_ += c;
            c = std::fgetc(f_);
        }
        if (c != EOF) std::ungetc(c, f_);
        symbol_ = LexicalSymbol::INTCON;
    }
    else if (c == '\'') {
        symbol_ = LexicalSymbol::CHARCON;
        c = std::fgetc(f_);
        if (c == '\'') ErrorHandler();
        else {
            if (!(std::isalnum(c) || c == '_' || c == '+' || c == '-' || c == '*' || c == '/')) {
                ErrorHandler();
            }
            token_ += c;
            c = std::fgetc(f_);
            if (c != '\'') {
                ErrorHandler();
                while (c != '\'' && c != EOF) c = std::fgetc(f_);
            }
        }
    }
    else if (c == '\"') {
        symbol_ = LexicalSymbol::STRCON;
        c = std::fgetc(f_);
        if (c == '\"') ErrorHandler();
        else {
            if (c < 32 || c > 126 || c == 34) ErrorHandler();
            token_ += c;
            while ((c = std::fgetc(f_)) != EOF) {
                if (c == '\"') break;
                else {
                    if (c < 32 || c > 126 || c == 34) ErrorHandler();
                    token_ += c;
                }
            }
        }
    }
    else if (c == '<') {
        token_ += c;
        c = std::fgetc(f_);
        if (c == '=') {
            token_ += c;
            symbol_ = LexicalSymbol::LEQ;
        }
        else {
            if (c != EOF) std::ungetc(c, f_);
            symbol_ = LexicalSymbol::LSS;
        }
    }
    else if (c == '>') {
        token_ += c;
        c = std::fgetc(f_);
        if (c == '=') {
            token_ += c;
            symbol_ = LexicalSymbol::GEQ;
        }
        else {
            if (c != EOF) std::ungetc(c, f_);
            symbol_ = LexicalSymbol::GRE;
        }
    }
    else if (c == '=') {
        token_ += c;
        c = std::fgetc(f_);
        if (c == '=') {
            token_ += c;
            symbol_ = LexicalSymbol::EQL;
        }
        else {
            if (c != EOF) std::ungetc(c, f_);
            symbol_ = LexicalSymbol::ASSIGN;
        }
    }
    else if (c == '!') {
        token_ += c;
        c = std::fgetc(f_);
        if (c != '=')  return ErrorHandler(true);
        token_ += c;
        symbol_ = LexicalSymbol::NEQ;
    }
    else {
        auto it = char2symbol.find(c);
        if (it != char2symbol.end()) {
            token_ += c;
            symbol_ = it->second;
        }
        else return ErrorHandler(true);
    }
    if (print_info_ && prev_symbol_ != LexicalSymbol::END) {
        output_->emplace_back(symbol2str.find(prev_symbol_)->second + ' ' + prev_token_);
    }
    cnt_++;
    return true;
}

int Lexer::GetTokenToInt() const {
    return std::stoi(token_);
}

LexicalSymbol Lexer::get_symbol() const {
    return symbol_;
}

std::string Lexer::get_token() const {
    return token_;
}

std::string Lexer::get_lower_token() const {
    std::string ret;
    ret.resize(token_.size());
    std::transform(token_.begin(), token_.end(), ret.begin(), ::tolower);
    return ret;
}

int Lexer::get_line_no() const {
    return line_no_;
}

LexicalSymbol Lexer::get_prev_symbol() const {
    return prev_symbol_;
}

int Lexer::get_prev_line_no() const {
    return prev_line_no_;
}

unsigned long long Lexer::get_cnt() const {
    return cnt_;
}

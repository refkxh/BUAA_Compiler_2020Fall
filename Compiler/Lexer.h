#pragma once

#include <cstdio>
#include <string>
#include <vector>

#include "Error.h"

enum class LexicalSymbol {
    IDENFR,
    INTCON,
    CHARCON,
    STRCON,
    CONSTTK,
    INTTK,
    CHARTK,
    VOIDTK,
    MAINTK,
    IFTK,
    ELSETK,
    SWITCHTK,
    CASETK,
    DEFAULTTK,
    WHILETK,
    FORTK,
    SCANFTK,
    PRINTFTK,
    RETURNTK,
    PLUS,
    MINU,
    MULT,
    DIV,
    LSS,
    LEQ,
    GRE,
    GEQ,
    EQL,
    NEQ,
    COLON,
    ASSIGN,
    SEMICN,
    COMMA,
    LPARENT,
    RPARENT,
    LBRACK,
    RBRACK,
    LBRACE,
    RBRACE,
    END
};

class Lexer {
private:

    std::FILE* f_;

    Error& error_;

    bool print_info_;
    std::vector<std::string>* output_;
    
    int line_no_;
    std::string token_;
    LexicalSymbol symbol_;

    int prev_line_no_;
    std::string prev_token_;
    LexicalSymbol prev_symbol_;

    unsigned long long cnt_;

    LexicalSymbol Token2ReservedName() const;

    bool ErrorHandler(bool do_next = false);

public:

    Lexer(std::FILE* f, Error& error, bool print_info = false, std::vector<std::string>* output = nullptr);

    bool ProcessNextSymbol();

    int GetTokenToInt() const;

    LexicalSymbol get_symbol() const;
    std::string get_token() const;
    std::string get_lower_token() const;
    int get_line_no() const;

    LexicalSymbol get_prev_symbol() const;
    int get_prev_line_no() const;

    unsigned long long get_cnt() const;
};

#pragma once

#include <cstdio>

class Error {
private:

    bool print_error_;
    bool has_error_;
    std::FILE* f_;

public:

    Error(bool print_error = false, FILE* f = nullptr);

    void LogError(const int line_no, const char cls);

    bool HasError();
};

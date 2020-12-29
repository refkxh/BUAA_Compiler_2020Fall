#include "Error.h"

Error::Error(bool print_error, FILE* f) :print_error_(print_error), has_error_(false), f_(f) {}

void Error::LogError(const int line_no, const char cls) {
    has_error_ = true;
    if (print_error_) fprintf(f_, "%d %c\n", line_no, cls);
}

bool Error::HasError() {
    return has_error_;
}

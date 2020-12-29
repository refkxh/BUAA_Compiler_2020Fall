#include "SymbolTable.h"

SymbolTable::SymbolTable() :string_counter_(0) {}

bool SymbolTable::AddSymbol(const std::string& cur_func, const std::string& name, const SymbolType type, const SymbolClass cls, const int val, int dim0, int dim1, bool need_init) {
    TableEntry tmp;
    tmp.type = type;
    tmp.cls = cls;
    tmp.val = val;
    tmp.dim0 = dim0;
    tmp.dim1 = dim1;
    tmp.need_init = need_init;
    tmp.addr = 0;
    if (cur_func.empty()) {
        if (global_table_.count(name) != 0) return false;
        global_table_[name] = tmp;
        if (cls == SymbolClass::FUNCTION) {
            function_table_[name] = std::unordered_map<std::string, TableEntry>();
        }
    }
    else {
        if (function_table_[cur_func].count(name) != 0) return false;
        if (cur_func == name) return false;
        function_table_[cur_func][name] = tmp;
    }
    return true;
}

std::pair<TableEntry*, bool> SymbolTable::LookupSymbol(const std::string& cur_func, const std::string& name) {
    if (cur_func.empty()) {
        auto it = global_table_.find(name);
        if (it != global_table_.end()) return std::make_pair(&(it->second), true);
        return std::make_pair(nullptr, false);
    }
    else {
        if (function_table_.count(cur_func) == 0) return std::make_pair(nullptr, false);
        auto it1 = function_table_[cur_func].find(name);
        if (it1 != function_table_[cur_func].end()) return std::make_pair(&(it1->second), false);
        auto it2 = global_table_.find(name);
        if (it2 != global_table_.end()) return std::make_pair(&(it2->second), true);
        return std::make_pair(nullptr, false);
    }
}

TableEntry* SymbolTable::GetKthParam(const std::string& func_name, const int k) {
    for (auto &item : function_table_[func_name]) {
        if (item.second.cls == SymbolClass::PARAM && item.second.val == k) return &(item.second);
    }
    return nullptr;
}

std::unordered_map<std::string, TableEntry>& SymbolTable::get_global_table() {
    return global_table_;
}

std::unordered_map<std::string, TableEntry>& SymbolTable::get_function_table(const std::string func_name) {
    return function_table_[func_name];
}

int SymbolTable::AddString(const std::string& str) {
    if (string_table_.count(str) == 0) {
        string_table_[str] = string_counter_;
        return string_counter_++;
    }
    return string_table_[str];
}

std::unordered_map<std::string, int>& SymbolTable::get_string_table() {
    return string_table_;
}

int SymbolTable::GetArrayInitializersSize() const {
    return array_initializers_.size();
}

void SymbolTable::AddArrayInitializer(std::vector<int>& initializer) {
    array_initializers_.emplace_back(initializer);
}

std::vector<int>& SymbolTable::GetKthArrayInitializer(int k) {
    return array_initializers_[k];
}

#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <utility>

enum class SymbolType { INT, CHAR, VOID };
enum class SymbolClass { CONST, VAR, FUNCTION, PARAM };

struct TableEntry {
    SymbolType type;
    SymbolClass cls;
    int val;  // const: val, function: num of params, param: kth, array: no in array_initializers_
    int dim0, dim1;  // array: dims, function: stack size, is not leaf
    bool need_init;
    int addr;
};

class SymbolTable {
private:

    std::unordered_map<std::string, TableEntry> global_table_;
    std::unordered_map<std::string, std::unordered_map<std::string, TableEntry>> function_table_;

    std::vector<std::vector<int>> array_initializers_;

    std::unordered_map<std::string, int> string_table_;
    int string_counter_;

public:

    SymbolTable();

    bool AddSymbol(const std::string& cur_func, const std::string& name, const SymbolType type, const SymbolClass cls, const int val, int dim0 = 0, int dim1 = 0, bool need_init = false);
    std::pair<TableEntry*, bool> LookupSymbol(const std::string& cur_func, const std::string& name);

    TableEntry* GetKthParam(const std::string& func_name, const int k);
    std::unordered_map<std::string, TableEntry>& get_global_table();
    std::unordered_map<std::string, TableEntry>& get_function_table(const std::string func_name);

    int AddString(const std::string& str);
    std::unordered_map<std::string, int>& get_string_table();

    int GetArrayInitializersSize() const;
    void AddArrayInitializer(std::vector<int>& initializer);
    std::vector<int>& GetKthArrayInitializer(int k);
};

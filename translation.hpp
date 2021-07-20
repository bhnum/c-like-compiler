#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <memory>
#include <cassert>
#include <variant>
#include <functional>

using std::string, std::vector, std::map, std::set;
using std::shared_ptr, std::variant, std::function;

#include "location.hpp"
using Location = yy::location;


class SymbolType
{
public:
    virtual ~SymbolType() {}
    virtual string Name() const = 0;
    virtual size_t Width() const = 0;
    virtual size_t AllignedWidth(int alignment = 4) const
    {
        if (Width() == 0) return 0;
        return (((Width() - 1) / alignment) + 1) * alignment;
    };

    virtual bool CompatibleWith(shared_ptr<SymbolType> other) const = 0;
    virtual bool operator==(const SymbolType& other) const = 0;
};

class VoidType : public SymbolType
{
public:
    virtual string Name() const { return "void"; }
    virtual size_t Width() const { return 0; }

    virtual bool CompatibleWith(shared_ptr<SymbolType> other) const
    {
        return *this == *other;
    }

    virtual bool operator==(const SymbolType& other) const
    {
        if (dynamic_cast<const VoidType*>(&other))
            return true;
        return false;
    }
};

class ValueType : public SymbolType
{
public:
    virtual string Allocation(int value) const = 0;
    virtual string Allocation() const { return ".space " + std::to_string(Width()); }

    virtual bool CompatibleWith(shared_ptr<SymbolType> other) const
    {
        if (std::dynamic_pointer_cast<ValueType>(other))
            return true;
        return false;
    }
};

class IntType : public ValueType
{
public:
    virtual string Name() const { return "int"; }
    virtual size_t Width() const { return 4; }
    virtual string Allocation(int value) const { return ".word " + std::to_string(value); }

    virtual bool operator==(const SymbolType& other) const
    {
        if (dynamic_cast<const IntType*>(&other))
            return true;
        return false;
    }
};

class CharType : public ValueType
{
public:
    virtual string Name() const { return "char"; }
    virtual size_t Width() const { return 1; }
    virtual string Allocation(int value) const
    {
        if (isprint(value) && value != '\\' && value != '\'')
            return ".byte " + std::to_string((char)value);
        return ".byte " + std::to_string(value);
    }

    virtual bool operator==(const SymbolType& other) const
    {
        if (dynamic_cast<const CharType*>(&other))
            return true;
        return false;
    }
};

class ArrayType : public SymbolType
{
public:
    ArrayType(shared_ptr<ValueType> underlying_type, size_t size)
        : underlying_type(underlying_type), size(size)
    {
        assert(size > 0);
    }

    virtual size_t Width() const { return underlying_type->Width() * size; }
    virtual string Name() const { return underlying_type->Name() + "[" + std::to_string(size) + "]"; }
    virtual string Allocation() const { return ".space " + std::to_string(Width()); }
    virtual string Allocation(const string& literal) const { return ".asciiz \"" + literal + "\""; }

    virtual bool CompatibleWith(shared_ptr<SymbolType> other) const
    {
        return *this == *other;
    }

    virtual bool operator==(const SymbolType& other) const
    {
        if (auto array = dynamic_cast<const ArrayType*>(&other))
            return underlying_type == array->underlying_type && size == array->size;
        return false;
    }

    shared_ptr<ValueType> underlying_type;
    size_t size;
};

// only usable for function parameters for now
class PointerType : public SymbolType
{
private:
    size_t pointer_width = 4;
public:
    PointerType(shared_ptr<ValueType> underlying_type)
        : underlying_type(underlying_type) {}

    virtual size_t Width() const { return pointer_width; }
    virtual string Name() const { return underlying_type->Name() + "*"; }

    virtual bool CompatibleWith(shared_ptr<SymbolType> other) const
    {
        if (*this == *other)
            return true;
        if (auto array = std::dynamic_pointer_cast<ArrayType>(other))
            return underlying_type->Width() == array->underlying_type->Width();
        return false;
    }
    
    virtual bool operator==(const SymbolType& other) const
    {
        if (auto pointer = dynamic_cast<const PointerType*>(&other))
            return underlying_type == pointer->underlying_type;
        return false;
    }

    shared_ptr<ValueType> underlying_type;
};


// common types
inline const std::shared_ptr<VoidType> void_type = std::make_shared<VoidType>();
inline const std::shared_ptr<CharType> char_type = std::make_shared<CharType>();
inline const std::shared_ptr<IntType> int_type = std::make_shared<IntType>();
inline const std::shared_ptr<PointerType> char_pointer_type = std::make_shared<PointerType>(char_type);
inline const std::shared_ptr<PointerType> int_pointer_type = std::make_shared<PointerType>(int_type);

inline bool is_value_type(std::shared_ptr<SymbolType> type)
{
    return std::dynamic_pointer_cast<ValueType>(type) != nullptr;
}

inline bool is_array_type(std::shared_ptr<SymbolType> type)
{
    return std::dynamic_pointer_cast<ArrayType>(type) != nullptr;
}

inline bool is_pointer_type(std::shared_ptr<SymbolType> type)
{
    return std::dynamic_pointer_cast<PointerType>(type) != nullptr;
}

inline std::shared_ptr<ArrayType> as_array_type(std::shared_ptr<SymbolType> type)
{
    return std::dynamic_pointer_cast<ArrayType>(type);
}

inline std::shared_ptr<PointerType> as_pointer_type(std::shared_ptr<SymbolType> type)
{
    return std::dynamic_pointer_cast<PointerType>(type);
}


class CompileError : public std::runtime_error
{
public:
    CompileError(const Location& l, const std::string& m)
        : std::runtime_error(m), location (l) {}

    CompileError (const CompileError& s)
        : std::runtime_error(s.what()), location (s.location) {}

    Location location;
};


class Code
{
private:
    vector<variant<shared_ptr<string>, function<string()>>> lines;
public:
    Code() {}

    Code(const char* str)
    {
        lines.push_back(std::make_shared<string>(str));
    }

    Code(const string& str)
    {
        lines.push_back(std::make_shared<string>(str));
    }
    
    Code(const function<string()>& func)
    {
        lines.push_back(func);
    }

    friend Code& operator+=(Code& left, const Code& right);
    friend Code operator+(Code left, const Code& right);
    friend std::ostream& operator<<(std::ostream& out, const Code& code);
};

inline Code& operator+=(Code& left, const Code& right)
{
    left.lines.insert(left.lines.end(), right.lines.begin(), right.lines.end());
    return left;
}

inline Code operator+(Code left, const Code& right)
{
    return left += right;
}

inline std::ostream& operator<<(std::ostream& out, const Code& code)
{
    for (auto v : code.lines)
    {
        if (v.index() == 0) // string
            out << *std::get<shared_ptr<string>>(v);
        else // function
            out << std::get<function<string()>>(v)();
    }
    return out;
}

inline const size_t indent_length = 2;
inline const string& tab = "    ";


class Symbol
{
public:
    Symbol(const string& name, shared_ptr<SymbolType> type, const Location& loc)
        : name(name), type(type), location(loc) {}
    virtual ~Symbol() {}

    string name;
    shared_ptr<SymbolType> type;
    Location location;

    virtual Code LoadValue(const string& reg) = 0;
    virtual Code SaveValue(const string& reg) = 0;
    virtual Code LoadAddress(const string& reg) = 0;
    
    virtual Code LoadElementValue(const string& index_reg, const string& dest_reg) = 0;
    virtual Code SaveElementValue(const string& index_reg, const string& source_reg) = 0;

protected:
    string ReadableName()
    {
        if (name.empty())
            return "result";
        return  "symbol \"" + name + "\"";
    }
};


class GlobalSymbol : public Symbol
{
public:
    GlobalSymbol(const string& name, shared_ptr<SymbolType> type, const Location& loc)
        : Symbol(name, type, loc) {}
        
    virtual Code LoadAddress(const string& reg);
};


class FieldSymbol : public GlobalSymbol
{
public:
    FieldSymbol(const string& name, shared_ptr<SymbolType> type, const Location& loc)
        : GlobalSymbol(name, type, loc) {}
        
    virtual Code LoadValue(const string& reg);

    virtual Code SaveValue(const string& reg);
    
    virtual Code LoadElementValue(const string& index_reg, const string& dest_reg);

    virtual Code SaveElementValue(const string& index_reg, const string& source_reg);
};


class FunctionSymbol : public GlobalSymbol
{
public:
    FunctionSymbol(const string& name, shared_ptr<SymbolType> type,
        vector<shared_ptr<SymbolType>> param_types, const Location& loc)
        : GlobalSymbol(name, type, loc), param_types(param_types) {}

    vector<shared_ptr<SymbolType>> param_types;
        
    virtual Code LoadValue(const string& reg)
    {
        throw CompileError(location, ReadableName() + " is not a variable");
    }

    virtual Code SaveValue(const string& reg)
    {
        throw CompileError(location, ReadableName() + " is not a variable");
    }
    
    virtual Code LoadElementValue(const string& index_reg, const string& dest_reg)
    {
        throw CompileError(location, ReadableName() + " is not a indexable");
    }

    virtual Code SaveElementValue(const string& index_reg, const string& source_reg)
    {
        throw CompileError(location, ReadableName() + " is not a indexable");
    }
};


class VariableSymbol : public Symbol
{
public:
    VariableSymbol(const string& name, shared_ptr<SymbolType> type, int offset,
        shared_ptr<int> stack_depth, const Location& loc)
        : Symbol(name, type, loc), offset(offset), stack_depth(stack_depth) {}

    int offset;
    shared_ptr<int> stack_depth;
    
    virtual Code LoadValue(const string& reg);

    virtual Code SaveValue(const string& reg);

    virtual Code LoadAddress(const string& reg);

    virtual Code LoadElementValue(const string& index_reg, const string& dest_reg);

    virtual Code SaveElementValue(const string& index_reg, const string& source_reg);

private:
    Code StackOffset()
    {
        auto stack_depth = this->stack_depth;
        int offset = this->offset;
        return Code([stack_depth, offset]() { return std::to_string(*stack_depth - offset); });
    }
};


class VoidSymbol : public Symbol
{
public:
    VoidSymbol(const Location& loc) : Symbol("void", void_type, loc) {}

    virtual Code LoadValue(const string& reg) { InvalidAccess(); }
    virtual Code SaveValue(const string& reg) { InvalidAccess(); }
    virtual Code LoadAddress(const string& reg) { InvalidAccess(); }
    
    virtual Code LoadElementValue(const string& index_reg, const string& dest_reg) { InvalidAccess(); }
    virtual Code SaveElementValue(const string& index_reg, const string& source_reg) { InvalidAccess(); }

private:
    [[noreturn]] void InvalidAccess()
    {
        throw CompileError(location, "type of result is \"void\"");
    }
};


class GlobalContext
{
public:
    string current_section = "code";

    shared_ptr<FieldSymbol> DeclareField(const FieldSymbol& field);

    shared_ptr<FunctionSymbol> DeclareFunction(const FunctionSymbol& function);

    shared_ptr<Symbol> operator[](const string& name) const;

    string NewLabel()
    {
        static size_t i = 0;
        return "$L" + std::to_string(++i);
    }

    function<void(const Location&, const string&, const string&)> printer;

    map<string, shared_ptr<GlobalSymbol>> symbols;
};


class FunctionContext
{
public:
    FunctionContext(GlobalContext& global_context, FunctionSymbol& symbol)
        : global_context(global_context), function_symbol(symbol),
        epilouge_label("$" + symbol.name + "_epilouge") {}

    void DeclareParameter(const string& name, shared_ptr<SymbolType> type, const Location& loc);

    shared_ptr<Symbol> operator[](const string& name) const;

    void UpdateStackDepth(int depth = 0)
    {
        *stack_depth = std::max(*stack_depth, context_depth + depth);
    }

    GlobalContext& global_context;
    FunctionSymbol& function_symbol;

    string epilouge_label;

    int context_depth = 0;
    shared_ptr<int> stack_depth = std::make_shared<int>(0);
    vector<shared_ptr<VariableSymbol>> symbols;

    static const int stack_alignment = 4;
};


class LocalContext
{
public:
    LocalContext(FunctionContext& function_context)
        : previous_context(nullptr),
        function_context(function_context),
        global_context(function_context.global_context) {}
        
    LocalContext(LocalContext& previous_context)
        : previous_context(&previous_context),
        function_context(previous_context.function_context),
        global_context(previous_context.global_context) {}

    void DeclareVariable(const string& name, shared_ptr<SymbolType> type, const Location& loc);

    shared_ptr<Symbol> operator[](const string& name);

    void UpdateStackDepth(int depth = 0)
    {
        if (previous_context == nullptr)
            function_context.UpdateStackDepth(context_depth + depth);
        else
            previous_context->UpdateStackDepth(context_depth + depth);
    }

    int CumulativeDepth()
    {
        if (previous_context == nullptr)
            return function_context.context_depth + context_depth;
        else
            return previous_context->CumulativeDepth() + context_depth;
    }

    LocalContext* previous_context;
    FunctionContext& function_context;
    GlobalContext& global_context;
    
    int context_depth = 0;
    vector<shared_ptr<VariableSymbol>> symbols;
    set<shared_ptr<Symbol>> referenced_symbols;

    string break_label;
    string LastBreakLabel()
    {
        if (!break_label.empty())
            return break_label;
        if (previous_context != nullptr)
            return previous_context->LastBreakLabel();
        return "";
    }
    
    string continue_label;
    string LastContinueLabel()
    {
        if (!continue_label.empty())
            return continue_label;
        if (previous_context != nullptr)
            return previous_context->LastContinueLabel();
        return "";
    }
};


class ExpressionContext
{
public:
    ExpressionContext(LocalContext& local_context)
        : local_context(local_context) {}
        
    ExpressionContext(ExpressionContext& expression_context)
        : local_context(expression_context.local_context), context_depth(expression_context.context_depth) {}

    LocalContext& local_context;
    
    shared_ptr<VariableSymbol> NewTemp(const Location& loc)
    {
        return NewTemp(std::make_shared<IntType>(), loc);
    }

    shared_ptr<VariableSymbol> NewTemp(shared_ptr<SymbolType> type, const Location& loc);

    int context_depth = 0;
};


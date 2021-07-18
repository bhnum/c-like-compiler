#include "translation.hpp"

#include <algorithm>

Code GlobalSymbol::LoadAddress(const string& reg)
{
    return tab + "la " + reg + ", " + name + "\n";
}

Code FieldSymbol::LoadValue(const string& reg)
{
    if (is_array_type(type))
        return LoadAddress(reg);
    return tab + "lw " + reg + ", " + name + "\n";
}

Code FieldSymbol::SaveValue(const string& reg)
{
    if (is_array_type(type))
        throw CompileError(location, ReadableName() + " of type \"" + type->Name() + "\" is not assignable");
    return tab + "sw " + reg + ", " + name + "\n";
}

Code FieldSymbol::LoadElementValue(const string& index_reg, const string& dest_reg)
{
    if (is_array_type(type))
    {
        auto underlying_type = (is_array_type(type) ?
            as_array_type(type)->underlying_type : as_pointer_type(type)->underlying_type);

        Code code;
        if (underlying_type->Width() == 1)
            code = tab + "lb " + dest_reg + ", " + name + "(" + index_reg + ")\n";
        else if (underlying_type->Width() == 4)
        {
            code = tab + "mul " + index_reg + ", " + index_reg + ", "
                + std::to_string(underlying_type->Width()) + "\n";
            code += tab + "lw " + dest_reg + ", " + name + "(" + index_reg + ")\n";
        }
        else
            throw CompileError(location, "unsupported type width");

        return code;
    }
    else
        throw CompileError(location, ReadableName() + " of type " + type->Name() + " is not indexable");
}

Code FieldSymbol::SaveElementValue(const string& index_reg, const string& source_reg)
{
    if (is_array_type(type))
    {
        auto underlying_type = (is_array_type(type) ?
            as_array_type(type)->underlying_type : as_pointer_type(type)->underlying_type);

        Code code;
        if (underlying_type->Width() == 1)
            code = tab + "sb " + source_reg + ", " + name + "(" + index_reg + ")\n";
        else if (underlying_type->Width() == 4)
        {
            code = tab + "mul " + index_reg + ", " + index_reg + ", "
                + std::to_string(underlying_type->Width()) + "\n";
            code += tab + "sw " + source_reg + ", " + name + "(" + index_reg + ")\n";
        }
        else
            throw CompileError(location, "unsupported type width");

        return code;
    }
    else
        throw CompileError(location, ReadableName() + " of type " + type->Name() + " is not indexable");
}

Code VariableSymbol::LoadValue(const string& reg)
{
    if (is_array_type(type))
        return LoadAddress(reg);
    return tab + "lw " + reg + ", " + StackOffset() + "($sp)\n";
}

Code VariableSymbol::SaveValue(const string& reg)
{
    if (is_array_type(type))
        throw CompileError(location, ReadableName() + " of type \"" + type->Name() + "\" is not assignable");
    return tab + "sw " + reg + ", " + StackOffset() + "($sp)\n";
}

Code VariableSymbol::LoadAddress(const string& reg)
{
    return tab + "addu " + reg + ", $sp, " + StackOffset() + "\n";
}

Code VariableSymbol::LoadElementValue(const string& index_reg, const string& dest_reg)
{
    if (is_array_type(type) || is_pointer_type(type))
    {
        auto underlying_type = (is_array_type(type) ?
            as_array_type(type)->underlying_type : as_pointer_type(type)->underlying_type);

        Code code;
        if (underlying_type->Width() == 1)
        {
            if (is_array_type(type))
            {
                code = tab + "addu " + index_reg + ", $sp, " + index_reg + "\n";
                code += tab + "lb " + dest_reg + ", " + StackOffset() + "(" + index_reg + ")\n";
            }
            else
            {
                code = LoadValue("$t0");
                code += tab + "addu " + index_reg + ", $t0, " + index_reg + "\n";
                code += tab + "lb " + dest_reg + ", " + "(" + index_reg + ")\n";
            }
        }
        else if (underlying_type->Width() == 4)
        {
            code = tab + "mul " + index_reg + ", " + index_reg + ", "
                + std::to_string(underlying_type->Width()) + "\n";

            if (is_array_type(type))
            {
                code += tab + "addu " + index_reg + ", $sp, " + index_reg + "\n";
                code += tab + "lw " + dest_reg + ", " + StackOffset() + "(" + index_reg + ")\n";
            }
            else
            {
                code = LoadValue("$t0");
                code += tab + "addu " + index_reg + ", $t0, " + index_reg + "\n";
                code += tab + "lw " + dest_reg + ", " + "(" + index_reg + ")\n";
            }
        }
        else
            throw CompileError(location, "unsupported type width");

        return code;
    }
    else
        throw CompileError(location, ReadableName() + " of type " + type->Name() + " is not indexable");
}

Code VariableSymbol::SaveElementValue(const string& index_reg, const string& source_reg)
{
    if (is_array_type(type) || is_pointer_type(type))
    {
        auto underlying_type = (is_array_type(type) ?
            as_array_type(type)->underlying_type : as_pointer_type(type)->underlying_type);

        Code code;
        if (underlying_type->Width() == 1)
        {
            if (is_array_type(type))
            {
                code = tab + "addu " + index_reg + ", $sp, " + index_reg + "\n";
                code += tab + "sb " + source_reg + ", " + StackOffset() + "(" + index_reg + ")\n";
            }
            else
            {
                code = LoadValue("$t0");
                code += tab + "addu " + index_reg + ", $t0, " + index_reg + "\n";
                code += tab + "sb " + source_reg + ", " + "(" + index_reg + ")\n";
            }
        }
        else if (underlying_type->Width() == 4)
        {
            code = tab + "mul " + index_reg + ", " + index_reg + ", "
                + std::to_string(underlying_type->Width()) + "\n";
            if (is_array_type(type))
            {
                code += tab + "addu " + index_reg + ", $sp, " + index_reg + "\n";
                code += tab + "sw " + source_reg + ", " + StackOffset() + "(" + index_reg + ")\n";
            }
            else
            {
                code = LoadValue("$t0");
                code += tab + "addu " + index_reg + ", $t0, " + index_reg + "\n";
                code += tab + "sw " + source_reg + ", " + "(" + index_reg + ")\n";
            }
        }
        else
            throw CompileError(location, "unsupported type width");

        return code;
    }
    else
        throw CompileError(location, ReadableName() + " of type " + type->Name() + " is not indexable");
}

shared_ptr<FieldSymbol> GlobalContext::DeclareField(const FieldSymbol& field)
{
    if (symbols.find(field.name) != symbols.end())
        throw CompileError(field.location, "redeclaration of global variable \"" + field.name + "\"");
    auto symbol = std::make_shared<FieldSymbol>(field);
    symbols[field.name] = symbol;
    return symbol;
}

shared_ptr<FunctionSymbol> GlobalContext::DeclareFunction(const FunctionSymbol& function)
{
    if (symbols.find(function.name) != symbols.end())
        throw CompileError(function.location, "redeclaration of function \"" + function.name + "\"");
    auto symbol = std::make_shared<FunctionSymbol>(function);
    symbols[function.name] = symbol;
    return symbol;
}

shared_ptr<Symbol> GlobalContext::operator[](const string& name) const
{
    auto it = symbols.find(name);
    if (it != symbols.end())
        return it->second;
    return nullptr;
}

void FunctionContext::DeclareParameter(const string& name, shared_ptr<SymbolType> type, const Location& loc)
{
    if (std::find_if(symbols.begin(), symbols.end(), [&name](auto s) { return s->name == name; }) != symbols.end())
        throw CompileError(loc, "redeclaration of function parameter \"" + name + "\"");
    symbols.push_back(std::make_shared<VariableSymbol>(name, type, context_depth, stack_depth, loc));

    context_depth += type->AllignedWidth(stack_alignment);
    UpdateStackDepth();
}

shared_ptr<Symbol> FunctionContext::operator[](const string& name) const
{
    auto it = std::find_if(symbols.begin(), symbols.end(), [&name](auto s) { return s->name == name; });
    if (it != symbols.end())
        return *it;
    return global_context[name];
}

void LocalContext::DeclareVariable(const string& name, shared_ptr<SymbolType> type, const Location& loc)
{
    if (std::find_if(symbols.begin(), symbols.end(), [&name](auto s) { return s->name == name; }) != symbols.end())
        throw CompileError(loc, "redeclaration of local variable \"" + name + "\"");
    int stack_offset = CumulativeDepth() +
        type->AllignedWidth(function_context.stack_alignment) - function_context.stack_alignment;
    symbols.push_back(std::make_shared<VariableSymbol>(name, type, stack_offset, function_context.stack_depth, loc));

    context_depth += type->AllignedWidth(function_context.stack_alignment);
    UpdateStackDepth();
}

shared_ptr<Symbol> LocalContext::operator[](const string& name) const
{
    auto it = std::find_if(symbols.begin(), symbols.end(), [&name](auto s) { return s->name == name; });
    if (it != symbols.end())
        return *it;
    if (previous_context != nullptr)
        return (*previous_context)[name];
    return function_context[name];
}

shared_ptr<VariableSymbol> ExpressionContext::NewTemp(shared_ptr<SymbolType> type, const Location& loc)
{
    int stack_offset = local_context.CumulativeDepth() + context_depth;
    auto temp = std::make_shared<VariableSymbol>("", type, stack_offset,
        local_context.function_context.stack_depth, loc);

    context_depth += local_context.function_context.stack_alignment;
    local_context.UpdateStackDepth(context_depth);

    return temp;
}



#include "ast.hpp"

#include <fstream>
#include <sstream>


std::pair<Code, shared_ptr<Symbol>> IntegralCast::Evaluate(ExpressionContext& ctx)
{
    string set_label = ctx.local_context.global_context.NewLabel(),
        clear_label = ctx.local_context.global_context.NewLabel(),
        assign_label = ctx.local_context.global_context.NewLabel();
    Code code = exp->Evaluate(ctx, set_label, clear_label);

    auto symbol = ctx.NewTemp(exp->location);
    code += set_label + ":\n";
    code += tab + "li $v0, 1\n";
    code += tab + "b " + assign_label + "\n";
    code += clear_label + ":\n";
    code += tab + "move $v0, $zero\n";
    code += assign_label + ":\n";
    code += symbol->SaveValue("$v0");
    return std::make_pair(code, symbol);
};

Code LogicalCast::Evaluate(ExpressionContext& ctx, const string& true_label, const string& false_label)
{
    ExpressionContext inner = ctx;
    auto [code, symbol] = exp->Evaluate(inner);

    code += symbol->LoadValue("$v0");
    code += tab + "beq $v0, $zero, " + false_label + "\n";
    code += tab + "b " + true_label + "\n";
    return code;
};

std::pair<Code, shared_ptr<Symbol>> UnaryIntegralExpression::Evaluate(ExpressionContext& ctx)
{
    ExpressionContext inner = ctx;
    auto [code, symbol0] = exp->Evaluate(inner);

    auto symbol = ctx.NewTemp(location);

    code += symbol0->LoadValue("$v0");
    code += tab + op_to_instruction.at(op) + " $v0, $v0\n";
    code += symbol->SaveValue("$v0");
    return std::make_pair(code, symbol);
}

std::pair<Code, shared_ptr<Symbol>> BinaryIntegralExpression::Evaluate(ExpressionContext& ctx)
{
    // warn about division by zero
    int den;
    if (exp2->Precomputable(den) && den == 0)
        ctx.local_context.global_context.printer(location, "divide by zero", "warning");

    ExpressionContext inner = ctx;
    auto [code1, symbol1] = exp1->Evaluate(inner);
    auto [code2, symbol2] = exp2->Evaluate(inner);

    auto symbol = ctx.NewTemp(location);
    Code code = code1 + code2;

    code += symbol1->LoadValue("$v0");
    code += symbol2->LoadValue("$v1");
    code += tab + op_to_instruction.at(op) + " $v0, $v0, $v1\n";
    code += symbol->SaveValue("$v0");
    return std::make_pair(code, symbol);
}

std::pair<Code, shared_ptr<Symbol>> ConstantExpression::Evaluate(ExpressionContext& ctx)
{
    auto symbol = ctx.NewTemp(location);
    Code code = tab + "li $v0, " + std::to_string(value) + "\n";
    code += symbol->SaveValue("$v0");
    return std::make_pair(code, symbol);
}

std::pair<Code, shared_ptr<Symbol>> VariableExpression::Evaluate(ExpressionContext& ctx)
{
    auto symbol = ctx.local_context[name];
    if (!symbol)
        throw CompileError(location, "undefined symbol \"" + name + "\"");
    return std::make_pair(Code(), symbol);
}

std::pair<Code, shared_ptr<Symbol>> ArrayAccessExpression::Evaluate(ExpressionContext& ctx)
{
    auto symbol = ctx.local_context[name];
    if (!symbol)
        throw CompileError(location, "undefined symbol \"" + name + "\"");

    ExpressionContext inner = ctx;
    auto [code, index_symbol] = index->Evaluate(inner);

    if (is_array_type(symbol->type))
        code += EnsureIndexInRange(ctx, symbol, index_symbol);

    auto temp = ctx.NewTemp(location);
    code += index_symbol->LoadValue("$v0");
    code += symbol->LoadElementValue("$v0", "$v0");
    code += temp->SaveValue("$v0");
    
    return std::make_pair(code, temp);
}

Code ArrayAccessExpression::Assign(ExpressionContext& ctx, shared_ptr<Symbol> value)
{
    auto symbol = ctx.local_context[name];
    if (!symbol)
        throw CompileError(location, "undefined symbol \"" + name + "\"");

    auto [code, index_symbol] = index->Evaluate(ctx);

    if (is_array_type(symbol->type))
        code += EnsureIndexInRange(ctx, symbol, index_symbol);

    code += value->LoadValue("$v0");
    code += index_symbol->LoadValue("$v1");
    code += symbol->SaveElementValue("$v1", "$v0");
    return code;
}

Code ArrayAccessExpression::EnsureIndexInRange(ExpressionContext& ctx,
        shared_ptr<Symbol> array_symbol, shared_ptr<Symbol> index_symbol)
{
    auto array_type = as_array_type(array_symbol->type);

    // compile-time check
    int index_value;
    if (index->Precomputable(index_value) && (index_value < 0 || index_value >= int(array_type->size)))
        throw CompileError(location, "array index is out of bounds");

    // runtime check (might consider changing to a break instruction)
    string error_label = ctx.local_context.global_context.NewLabel();
    string end_label = ctx.local_context.global_context.NewLabel();
    Code code;
    code += tab + "# runtime array index bounds check\n";
    code += index_symbol->LoadValue("$t0");
    code += tab + "bltz $t0, " + error_label + "\n";
    code += tab + "bgeu $t0, " + std::to_string(array_type->size) + ", " + error_label + "\n";
    code += tab + "b " + end_label + "\n";
    code += error_label + ":\n";
    code += tab + "jal " + ctx.local_context["$out_of_bounds_error"]->name + "\n";
    code += end_label + ":\n";
    return code;
}

std::pair<Code, shared_ptr<Symbol>> AssignmentExpression::Evaluate(ExpressionContext& ctx)
{
    ExpressionContext inner = ctx;
    auto [code, value] = exp->Evaluate(inner);

    code += left->Assign(inner, value);

    // auto symbol = ctx.NewTemp(location);
    // code += value->LoadValue("$v0");
    // code += symbol->SaveValue("$v0");
    // return std::make_pair(code, symbol);
    return std::make_pair(code, value);
}

std::pair<Code, shared_ptr<Symbol>> FunctionCallExpression::Evaluate(ExpressionContext& ctx)
{
    auto symbol = ctx.local_context[name];
    if (!symbol)
        throw CompileError(location, "function \"" + name + "\" is not defined");

    auto function_symbol = std::dynamic_pointer_cast<FunctionSymbol>(symbol);
    if (!function_symbol)
        throw CompileError(location, "symbol \"" + name + "\" is a function");

    if (function_symbol->param_types.size() != args.size())
        throw CompileError(location, "incorrect number of arguments");

    ExpressionContext inner = ctx;
    vector<shared_ptr<Symbol>> symbols;
    Code code;

    for (size_t i = 0; i < args.size(); i++)
    {
        auto [c, s] = args[i]->Evaluate(inner);

        if (!function_symbol->param_types[i]->CompatibleWith(s->type))
            throw CompileError(location, "argument of type " + function_symbol->param_types[i]->Name() +
                " is not compatible with type " + s->type->Name());

        symbols.push_back(s);
        code += c;
    }
    
    for (size_t i = 0; i < symbols.size(); i++)
    {
        auto pt = function_symbol->param_types[i];
        string reg = "$a" + std::to_string(i);

        code += symbols[i]->LoadValue(reg);
        if (*pt == *char_type)
            code += tab + "and " + reg + ", " + reg + ", 0xff\n";
    }

    code += tab + "jal " + function_symbol->name + "\n";

    shared_ptr<Symbol> result;
    if (*function_symbol->type == *void_type)
        result = std::make_shared<VoidSymbol>(location);
    else
    {
        result = ctx.NewTemp(location);
        code += result->SaveValue("$v0");
    }

    return std::make_pair(code, result);
}

Code UnaryLogicalExpression::Evaluate(ExpressionContext& ctx, const string& true_label, const string& false_label)
{
    return exp->Evaluate(ctx, false_label, true_label);
}

Code BinaryLogicalExpression::Evaluate(ExpressionContext& ctx, const string& true_label, const string& false_label)
{
    string inner_label = ctx.local_context.global_context.NewLabel();

    if (op == "&&")
    {
        Code code = exp1->Evaluate(ctx, inner_label, false_label);
        code += inner_label + ":\n";
        code += exp2->Evaluate(ctx, true_label, false_label);
        return code;
    }
    if (op == "||")
    {
        Code code = exp1->Evaluate(ctx, true_label, inner_label);
        code += inner_label + ":\n";
        code += exp2->Evaluate(ctx, true_label, false_label);
        return code;
    }
    
    assert(false); // must not happen
}

Code RelationalExpression::Evaluate(ExpressionContext& ctx, const string& true_label, const string& false_label)
{
    ExpressionContext inner = ctx;
    auto [code1, symbol1] = exp1->Evaluate(inner);
    auto [code2, symbol2] = exp2->Evaluate(inner);

    Code code = code1 + code2;
    code += symbol1->LoadValue("$v0");
    code += symbol2->LoadValue("$v1");
    code += tab + op_to_instruction.at(op) + " $v0, $v1, " + true_label + "\n";
    code += tab + "b " + false_label + "\n";
    return code;
}

Code VariableDeclaration::Compile(LocalContext& ctx)
{
    ctx.DeclareVariable(name, type, location);
    return Code();
}

Code ContinueStatement::Compile(LocalContext& ctx)
{
    string label = ctx.LastContinueLabel();
    if (label.empty())
        throw CompileError(location, "no outer loop exists");
    return tab + "b " + label + "\n";
}

Code BreakStatement::Compile(LocalContext& ctx)
{
    string label = ctx.LastBreakLabel();
    if (label.empty())
        throw CompileError(location, "no outer loop or switch statement exists");
    return tab + "b " + label + "\n";
}

Code ReturnStatement::Compile(LocalContext& ctx)
{
    SymbolType& return_type = *ctx.function_context.function_symbol.type;

    Code code;
    if (exp != nullptr && (return_type == *int_type || return_type == *char_type))
    {
        int value;
        if (exp->Precomputable(value))
        {
            if (return_type == *char_type) value &= 0xff;
            code += tab + "li $v0, " + std::to_string(value) + "\n";
        }
        else
        {
            ExpressionContext inner = ctx;
            auto [exp_code, symbol] = exp->Evaluate(inner);
            code += exp_code;
            code += symbol->LoadValue("$v0");
            if (return_type == *char_type)
                code += tab + "and $v0, $v0, 0xff\n";
        }
    }
    else if (!(exp == nullptr && return_type == *void_type))
        throw CompileError(location, "return value type does not match function return type");

    code += tab + "b " + ctx.function_context.epilouge_label + "\n";
    return code;
}

Code IfElseStatement::Compile(LocalContext& ctx)
{
    string label = ctx.global_context.NewLabel();
    string then_label = label + "_then", else_label = label + "_else", end_label = label + "_end";

    Code code;
    ExpressionContext inner = ctx;
    code += condition->Evaluate(inner, then_label, else_label);
    code += then_label + ":\n";
    code += then_block->Compile(ctx);
    code += tab + "b " + end_label + "\n";
    code += else_label + ":\n";
    code += else_block->Compile(ctx);
    code += end_label + ":\n";
    return code;
}

Code SwitchStatement::Compile(LocalContext& parent_ctx)
{
    LocalContext ctx = parent_ctx;

    string label = ctx.global_context.NewLabel();
    string case_label = label + "_case", default_label = label + "_default", end_label = label + "_end";

    ExpressionContext inner = ctx;
    auto [code, symbol] = exp->Evaluate(inner);

    ctx.break_label = end_label;

    code += symbol->LoadValue("$v0");
    for (size_t i = 0; i < case_values.size(); i++)
        if (case_values[i] != nullptr)
            code += tab + "beq $v0, " + std::to_string(*case_values[i]) + ", " + case_label + std::to_string(i) + "\n";
    code += tab + "b " + default_label + "\n";
    
    for (size_t i = 0; i < case_bodies.size(); i++)
    {
        if (case_values[i] != nullptr)
            code += case_label + std::to_string(i) + ":\n";
        else
            code += default_label + ":\n";

        for (size_t j = 0; j < case_bodies[i].size(); j++)
            code += case_bodies[i][j]->Compile(ctx);
    }
    code += end_label + ":\n";

    return code;
}

Code WhileStatement::Compile(LocalContext& ctx)
{
    string label = ctx.global_context.NewLabel();
    string loop_label = label + "_loop", body_label = label + "_body", end_label = label + "_end";

    ctx.break_label = end_label;
    ctx.continue_label = loop_label;

    ExpressionContext inner = ctx;

    Code code;
    code += loop_label + ":\n";
    code += condition->Evaluate(inner, body_label, end_label);
    code += body_label + ":\n";
    code += body->Compile(ctx);
    code += tab + "b " + loop_label + "\n";
    code += end_label + ":\n";
    return code;
}

Code ForStatement::Compile(LocalContext& parent_ctx)
{
    LocalContext ctx = parent_ctx;

    string label = ctx.global_context.NewLabel();
    string loop_label = label + "_loop", body_label = label + "_body",
        step_label = label + "_step", end_label = label + "_end";

    ctx.break_label = end_label;
    ctx.continue_label = step_label;

    ExpressionContext inner = ctx;

    Code code;
    for (auto i : initializer)
        code += i->Compile(ctx);
    code += loop_label + ":\n";
    code += condition->Evaluate(inner, body_label, end_label);
    code += body_label + ":\n";
    code += body->Compile(ctx);
    code += step_label + ":\n";
    code += step->Compile(ctx);
    code += tab + "b " + loop_label + "\n";
    code += end_label + ":\n";
    return code;
}

Code FieldDefinition::Compile(GlobalContext& ctx)
{
    ctx.DeclareField(FieldSymbol(name, type, location));

    Code code;
    if (ctx.current_section != "data")
    {
        ctx.current_section = "data";
        code += ".data\n";
    }

    code += name + ":\n";
    if (auto valuetype = std::dynamic_pointer_cast<ValueType>(type))
    {
        code += tab + valuetype->Allocation(value) + "\n";
    }
    else if (auto arraytype = std::dynamic_pointer_cast<ArrayType>(type))
    {
        if (has_value)
        {
            code += tab + arraytype->Allocation(literal) + "\n";
            if (arraytype->Width() > literal.size() + 1)
                code += tab + ArrayType(arraytype->underlying_type,
                    arraytype->Width() - literal.size() - 1).Allocation() + "\n";
        }
        else
            code += tab + arraytype->Allocation() + "\n";
    }
    else
        assert(false); // must not happen

    return code + "\n";
}

Code FunctionDefinition::Compile(GlobalContext& ctx)
{
    vector<shared_ptr<SymbolType>> param_types;
    std::transform(params.begin(), params.end(), std::back_inserter(param_types), [](auto d) { return d->type; });
    auto symbol = ctx.DeclareFunction(FunctionSymbol(name, type, param_types, location));

    FunctionContext fctx(ctx, *symbol);
    
    fctx.DeclareParameter("$saved_ra", int_type, location);
    fctx.DeclareParameter("$saved_fp", int_type, location);

    for (auto p : params)
        fctx.DeclareParameter(p->name, p->type, p->location);

    Code code;
    if (ctx.current_section != "text")
    {
        ctx.current_section = "text";
        code += ".text\n";
    }
    code += name + ":\n";

    Code body_code = body->Compile(fctx);

    // prolouge
    code += tab + "addu $sp, $sp, " + std::to_string(-*fctx.stack_depth) + "\n";
    code += fctx["$saved_ra"]->SaveValue("$ra");
    code += fctx["$saved_fp"]->SaveValue("$fp");
    code += tab + "move $fp, $sp\n";

    for (size_t i = 0; i < params.size(); i++)
        code += fctx[params[i]->name]->SaveValue("$a" + std::to_string(i));

    code += body_code;

    // epilouge
    code += fctx.epilouge_label + ":\n";
    code += tab + "move $sp, $fp\n";
    code += fctx["$saved_ra"]->LoadValue("$ra");
    code += fctx["$saved_fp"]->LoadValue("$fp");
    code += tab + "addu $sp, $sp, " + std::to_string(*fctx.stack_depth) + "\n";
    code += tab + "jr $ra\n";

    return code + "\n";
}

Code MainFunctionDefinition::Compile(GlobalContext& ctx)
{
    auto symbol = ctx.DeclareFunction(FunctionSymbol(name, type, {}, location));

    FunctionContext fctx(ctx, *symbol);

    Code code;
    if (ctx.current_section != "text")
    {
        ctx.current_section = "text";
        code += ".text\n";
    }

    code += ".globl main\n";
    code += name + ":\n";

    Code body_code = body->Compile(fctx);

    // prolouge
    code += tab + "addu $sp, $sp, " + std::to_string(-*fctx.stack_depth) + "\n";
    code += tab + "move $fp, $sp\n";

    code += body_code;

    // epilouge
    code += fctx.epilouge_label + ":\n";
    code += tab + "move $sp, $fp\n";
    code += tab + "addu $sp, $sp, " + std::to_string(*fctx.stack_depth) + "\n";
    if (*type == *void_type)
        code += tab + "j " + ctx["exit"]->name + "\n";
    else
        code += tab + "j " + ctx["exit2"]->name + "\n";

    return code + "\n";
}

Code Program::Compile(function<void(const Location&, const string&, const string&)> printer)
{
    GlobalContext ctx;
    ctx.printer = printer;

    // define builtin function (syscalls)
    Location builtin_location;
    builtin_location.initialize(&builtin_filename);
    ctx.DeclareFunction(FunctionSymbol("print_string", void_type, { char_pointer_type }, builtin_location));
    ctx.DeclareFunction(FunctionSymbol("print_char", void_type, { char_type }, builtin_location));
    ctx.DeclareFunction(FunctionSymbol("print_int", void_type, { int_type }, builtin_location));
    
    ctx.DeclareFunction(FunctionSymbol("read_string", void_type, { char_pointer_type, int_type }, builtin_location));
    ctx.DeclareFunction(FunctionSymbol("read_char", char_type, { }, builtin_location));
    ctx.DeclareFunction(FunctionSymbol("read_int", int_type, { }, builtin_location));

    ctx.DeclareFunction(FunctionSymbol("exit", void_type, { }, builtin_location));
    ctx.DeclareFunction(FunctionSymbol("exit2", void_type, { int_type }, builtin_location));
    ctx.DeclareFunction(FunctionSymbol("$out_of_bounds_error", void_type, { int_type }, builtin_location));

    ctx.current_section = "text";
    Code code = ".data\n";
    code += ".align 2 # word align\n\n";
    code += ".text\n";
    code += tab + "j main # entry point\n\n";

    for (auto d : definitions)
        code += d->Compile(ctx);

    
    std::ifstream builtinsfile;
    builtinsfile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    try
    {
        builtinsfile.open(builtin_asm_filename);
    }
    catch (const std::ifstream::failure& er)
    {
        throw std::runtime_error("Unable to open file \"" + builtin_asm_filename + "\": " + er.what());
    }

    std::stringstream builtins_buffer;
    builtins_buffer << builtinsfile.rdbuf();
    code += builtins_buffer.str();

    return code;
}

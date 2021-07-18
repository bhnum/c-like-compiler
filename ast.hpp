#pragma once

#include "translation.hpp"


class Statement
{
public:
    Statement(const Location& loc) : location(loc) {}
    virtual ~Statement() {}

    Location location;

    virtual Code Compile(LocalContext& ctx) { return Code(); }

    virtual string Tree(int indent = 0)
    {
        return string(indent, ' ') + "empty statement\n";
    };
};


class Expression : public Statement
{
public:
    Expression(const Location& loc) : Statement(loc) {}
    
    virtual Code Compile(LocalContext& ctx) { assert(false); }
};


class IntegralExpression : public Expression
{
public:
    IntegralExpression(const Location& loc) : Expression(loc) {}

    virtual bool Precomputable(int& result)
    {
        return false;
    }
    
    virtual Code Compile(LocalContext& ctx)
    {
        ExpressionContext inner = ctx;
        return Evaluate(inner).first;
    }
    virtual std::pair<Code, shared_ptr<Symbol>> Evaluate(ExpressionContext& ctx) { assert(false); };
};


class LogicalExpression : public Expression
{
public:
    LogicalExpression(const Location& loc) : Expression(loc) {}
    
    virtual Code Compile(LocalContext& ctx)
    {
        string label = ctx.global_context.NewLabel();
        ExpressionContext inner = ctx;
        return Evaluate(inner, label, label) + (tab + label + ":\n");
    }
    
    virtual Code Evaluate(ExpressionContext& ctx, const string& true_label, const string& false_label) { assert(false); };
};


class IntegralCast : public IntegralExpression
{
public:
    IntegralCast(shared_ptr<LogicalExpression> exp)
        : IntegralExpression(exp->location), exp(exp) {}
    
    shared_ptr<LogicalExpression> exp;
    
    virtual std::pair<Code, shared_ptr<Symbol>> Evaluate(ExpressionContext& ctx);

    static shared_ptr<IntegralExpression> IfNeeded(shared_ptr<Expression> exp)
    {
        if (auto integral = std::dynamic_pointer_cast<IntegralExpression>(exp))
            return integral;
        if (auto logical = std::dynamic_pointer_cast<LogicalExpression>(exp))
            return std::make_shared<IntegralCast>(logical);
        assert(false);  // must not happen
    }

    virtual string Tree(int indent = 0)
    {
        return string(indent, ' ') + "cast to int\n" + exp->Tree(indent + indent_length);
    }
};


class LogicalCast : public LogicalExpression
{
public:
    LogicalCast(shared_ptr<IntegralExpression> exp)
        : LogicalExpression(exp->location), exp(exp) {}
    
    shared_ptr<IntegralExpression> exp;
    
    virtual Code Evaluate(ExpressionContext& ctx, const string& true_label, const string& false_label);

    static shared_ptr<LogicalExpression> IfNeeded(shared_ptr<Expression> exp)
    {
        if (auto logical = std::dynamic_pointer_cast<LogicalExpression>(exp))
            return logical;
        if (auto integral = std::dynamic_pointer_cast<IntegralExpression>(exp))
            return std::make_shared<LogicalCast>(integral);
        assert(false);  // must not happen
    }

    virtual string Tree(int indent = 0)
    {
        return string(indent, ' ') + "cast to bool\n" + exp->Tree(indent + indent_length);
    }
};


class UnaryIntegralExpression : public IntegralExpression
{
public:
    UnaryIntegralExpression(const string& op, shared_ptr<Expression> exp, const Location& loc)
        : IntegralExpression(loc + exp->location), exp(IntegralCast::IfNeeded(exp)), op(op)
    {
        if (op != "+" && op != "-" && op != "~")
            throw std::domain_error("invalid operator");
    }

    shared_ptr<IntegralExpression> exp;
    string op;

    virtual bool Precomputable(int& result);
    
    virtual std::pair<Code, shared_ptr<Symbol>> Evaluate(ExpressionContext& ctx);

    virtual string Tree(int indent = 0)
    {
        return string(indent, ' ') + "unary operator " + op + "\n" + exp->Tree(indent + indent_length);
    }

private:
    static inline const map<string, string> op_to_instruction = 
        {{"+", "move"}, {"-", "negu"}, {"~", "not"}};
};


class BinaryIntegralExpression : public IntegralExpression
{
public:
    BinaryIntegralExpression(const string& op, shared_ptr<Expression> exp1, shared_ptr<Expression> exp2)
        : IntegralExpression(exp1->location + exp2->location),
        exp1(IntegralCast::IfNeeded(exp1)), exp2(IntegralCast::IfNeeded(exp2)), op(op)
    {
        if (op != "+" && op != "-" && op != "*" && op != "/" && op != "&" && op != "|" && op != "^")
            throw std::domain_error("invalid operator");
    }

    shared_ptr<IntegralExpression> exp1, exp2;
    string op;
    
    virtual bool Precomputable(int& result);
    
    virtual std::pair<Code, shared_ptr<Symbol>> Evaluate(ExpressionContext& ctx);

    virtual string Tree(int indent = 0)
    {
        return string(indent, ' ') + "binary operator " + op + "\n" +
            exp1->Tree(indent + indent_length) + exp2->Tree(indent + indent_length);
    }

private:
    static inline const map<string, string> op_to_instruction = 
        {{"+", "addu"}, {"-", "subu"}, {"*", "mul"}, {"/", "divu"}, {"&", "and"}, {"|", "or"}, {"^", "xor"}};;
};


class ConstantExpression : public IntegralExpression
{
public:
    ConstantExpression(int value, const Location& loc)
        : IntegralExpression(loc), value(value) {}

    int value;
    
    virtual bool Precomputable(int& result)
    {
        result = value;
        return true;
    }

    virtual std::pair<Code, shared_ptr<Symbol>> Evaluate(ExpressionContext& ctx);

    virtual string Tree(int indent = 0)
    {
        return string(indent, ' ') + std::to_string(value) + "\n";
    }
};


class StringLiteral : public Expression
{
public:
    StringLiteral(const string& value, const Location& loc)
        : Expression(loc), value(value) {}

    string value;
    
    virtual bool Precomputable(int& result)
    {
        return false;
    }

    virtual std::pair<Code, shared_ptr<Symbol>> Evaluate(ExpressionContext& ctx)
    {
        assert(false); // must not happen
    }

    virtual string Tree(int indent = 0)
    {
        assert(false); // must not happen
    }
};


class LValueExpression : public IntegralExpression
{
public:
    LValueExpression(const Location& loc) : IntegralExpression(loc) {}

    virtual Code Assign(ExpressionContext& ctx, shared_ptr<Symbol> value) { assert(false); }
};


class VariableExpression : public LValueExpression
{
public:
    VariableExpression(const string& name, const Location& loc)
        : LValueExpression(loc), name(name) {}

    string name;
    
    virtual std::pair<Code, shared_ptr<Symbol>> Evaluate(ExpressionContext& ctx);
    
    virtual Code Assign(ExpressionContext& ctx, shared_ptr<Symbol> value)
    {
        auto symbol = ctx.local_context[name];
        if (!symbol)
            throw CompileError(location, "undefined symbol \"" + name + "\"");

        Code code = value->LoadValue("$v0");
        code += symbol->SaveValue("$v0");
        return code;
    }

    virtual string Tree(int indent = 0)
    {
        return string(indent, ' ') + name + "\n";
    }
};


class ArrayAccessExpression : public LValueExpression
{
public:
    ArrayAccessExpression(const string& name, shared_ptr<Expression> index, const Location& loc)
        : LValueExpression(loc), name(name), index(IntegralCast::IfNeeded(index)) {}

    string name;
    shared_ptr<IntegralExpression> index;
    
    virtual std::pair<Code, shared_ptr<Symbol>> Evaluate(ExpressionContext& ctx);
    
    virtual Code Assign(ExpressionContext& ctx, shared_ptr<Symbol> value);

private:
    Code EnsureIndexInRange(ExpressionContext& ctx,
        shared_ptr<Symbol> array_symbol, shared_ptr<Symbol> index_symbol);

public:
    virtual string Tree(int indent = 0)
    {
        return string(indent, ' ') + name + "[ ]\n" + index->Tree(indent + indent_length);
    }
};


class AssignmentExpression : public IntegralExpression
{
public:
    AssignmentExpression(shared_ptr<LValueExpression> left, shared_ptr<Expression> exp)
        : IntegralExpression(left->location + exp->location), left(left), exp(IntegralCast::IfNeeded(exp)) {}
    
    shared_ptr<LValueExpression> left;
    shared_ptr<IntegralExpression> exp;
    
    virtual std::pair<Code, shared_ptr<Symbol>> Evaluate(ExpressionContext& ctx);

    virtual string Tree(int indent = 0)
    {
        return string(indent, ' ') + "assignment =\n" +
            left->Tree(indent + indent_length) + exp->Tree(indent + indent_length);
    }
};


class FunctionCallExpression : public IntegralExpression
{
public:
    FunctionCallExpression(const string& name, const vector<shared_ptr<Expression>>& args, const Location& loc);

    string name;
    vector<shared_ptr<IntegralExpression>> args;
    
    virtual std::pair<Code, shared_ptr<Symbol>> Evaluate(ExpressionContext& ctx);

    virtual string Tree(int indent = 0)
    {
        string str = string(indent, ' ') + "call " + name + "\n";
        if (args.size() > 0)
            for (auto a : args)
                str += a->Tree(indent + indent_length);
        return str;
    }
};


class UnaryLogicalExpression : public LogicalExpression
{
public:
    UnaryLogicalExpression(const string& op, shared_ptr<Expression> exp, const Location& loc)
        : LogicalExpression(loc + exp->location), exp(LogicalCast::IfNeeded(exp)), op(op)
    {
        if (op != "!")
            throw std::domain_error("invalid operator");
    }

    shared_ptr<LogicalExpression> exp;
    string op;
    
    virtual Code Evaluate(ExpressionContext& ctx, const string& true_label, const string& false_label);

    virtual string Tree(int indent = 0)
    {
        return string(indent, ' ') + "unary operator " + op + "\n" + exp->Tree(indent + indent_length);
    }
};


class BinaryLogicalExpression : public LogicalExpression
{
public:
    BinaryLogicalExpression(const string& op, shared_ptr<Expression> exp1, shared_ptr<Expression> exp2)
        : LogicalExpression(exp1->location + exp2->location), 
        exp1(LogicalCast::IfNeeded(exp1)), exp2(LogicalCast::IfNeeded(exp2)), op(op)
    {
        if (op != "&&" && op != "||")
            throw std::domain_error("invalid operator");
    }

    shared_ptr<LogicalExpression> exp1, exp2;
    string op;
    
    virtual Code Evaluate(ExpressionContext& ctx, const string& true_label, const string& false_label);

    virtual string Tree(int indent = 0)
    {
        return string(indent, ' ') + "binary operator " + op + "\n" +
            exp1->Tree(indent + indent_length) + exp2->Tree(indent + indent_length);
    }
};


class RelationalExpression : public LogicalExpression
{
public:
    RelationalExpression(const string& op, shared_ptr<Expression> exp1, shared_ptr<Expression> exp2)
        : LogicalExpression(exp1->location + exp2->location), 
        exp1(IntegralCast::IfNeeded(exp1)), exp2(IntegralCast::IfNeeded(exp2)), op(op)
    {
        if (op != "==" && op != "!=" && op != "<=" && op != ">=" && op != "<" && op != ">")
            throw std::domain_error("invalid operator");
    }

    shared_ptr<IntegralExpression> exp1, exp2;
    string op;
    
    virtual Code Evaluate(ExpressionContext& ctx, const string& true_label, const string& false_label);

    virtual string Tree(int indent = 0)
    {
        return string(indent, ' ') + "relational operator " + op + "\n" +
            exp1->Tree(indent + indent_length) + exp2->Tree(indent + indent_length);
    }

private:
    static inline const map<string, string> op_to_instruction = 
        {{"==", "beq"}, {"!=", "bne"}, {">", "bgt"}, {">=", "bge"}, {"<", "blt"}, {"<=", "ble"}};;
};


class VariableDeclaration : public Statement
{
public:
    VariableDeclaration(const string& name, shared_ptr<SymbolType> type, const Location& loc)
        : Statement(loc), name(name), type(type) {}

    string name;
    shared_ptr<SymbolType> type;

    virtual Code Compile(LocalContext& ctx);

    virtual string Tree(int indent = 0)
    {
        return string(indent, ' ') + name + " : " + type->Name() + "\n";
    }
};


class JumpStatement : public Statement
{
public:
    JumpStatement(const Location& loc) : Statement(loc) {}
};


class ContinueStatement : public JumpStatement
{
public:
    ContinueStatement(const Location& loc) : JumpStatement(loc) {}

    virtual Code Compile(LocalContext& ctx);

    virtual string Tree(int indent = 0)
    {
        return string(indent, ' ') + "continue\n";
    }
};


class BreakStatement : public JumpStatement
{
public:
    BreakStatement(const Location& loc) : JumpStatement(loc) {}

    virtual Code Compile(LocalContext& ctx);

    virtual string Tree(int indent = 0)
    {
        return string(indent, ' ') + "break\n";
    }
};


class ReturnStatement : public JumpStatement
{
public:
    ReturnStatement(const Location& loc) : JumpStatement(loc) {}

    ReturnStatement(shared_ptr<Expression> exp, const Location& loc)
        : JumpStatement(loc), exp(IntegralCast::IfNeeded(exp)) {}

    shared_ptr<IntegralExpression> exp = nullptr; // could be null!

    virtual Code Compile(LocalContext& ctx);

    virtual string Tree(int indent = 0)
    {
        return string(indent, ' ') + "return\n" +
            (exp == nullptr ? "" : exp->Tree(indent + indent_length));
    }
};


class StatementBlock : public Statement
{
public:
    StatementBlock(const Location& loc) : Statement(loc) {}

    StatementBlock(const vector<shared_ptr<Statement>>& statements, const Location& loc)
        : Statement(loc), statements(statements) {}

    StatementBlock(shared_ptr<Statement> statement) : Statement(statement->location)
    {
        statements.push_back(statement);
    }

    vector<shared_ptr<Statement>> statements;

    virtual Code Compile(LocalContext& parent_ctx)
    {
        LocalContext ctx(parent_ctx);
        return CompileOnContext(ctx);
    }

    Code Compile(FunctionContext& fctx)
    {
        LocalContext ctx(fctx);
        return CompileOnContext(ctx);
    }

private:
    Code CompileOnContext(LocalContext& ctx)
    {
        Code code;
        for (auto s : statements)
            code += s->Compile(ctx);
        return code;
    }

public:
    virtual string Tree(int indent = 0)
    {
        string str = string(indent, ' ') + "block\n";
        for (auto s : statements)
            str += s->Tree(indent + indent_length);
        return str;
    }
};


class IfElseStatement : public Statement
{
public:
    IfElseStatement(shared_ptr<Expression> condition, shared_ptr<StatementBlock> then_block,
        shared_ptr<StatementBlock> else_block, const Location& loc)
        : Statement(loc), condition(LogicalCast::IfNeeded(condition)),
        then_block(then_block), else_block(else_block) {}

    shared_ptr<LogicalExpression> condition;
    shared_ptr<StatementBlock> then_block, else_block;

    virtual Code Compile(LocalContext& ctx);

    virtual string Tree(int indent = 0)
    {
        string str = string(indent, ' ') + "if\n";
        str += string(indent + indent_length, ' ') + "condition\n";
        str += condition->Tree(indent + 2 * indent_length);
        str += string(indent + indent_length, ' ') + "then\n";
        str += then_block->Tree(indent + 2 * indent_length);
        str += string(indent + indent_length, ' ') + "else\n";
        str += else_block->Tree(indent + 2 * indent_length);
        return str;
    }
};


class SwitchStatement : public Statement
{
public:
    SwitchStatement(const Location& loc) : Statement(loc) {}
    
    shared_ptr<IntegralExpression> exp;
    vector<shared_ptr<int>> case_values;
    vector<vector<shared_ptr<Statement>>> case_bodies;

    void AddCase(shared_ptr<Expression> value_exp, const Location& loc);

    void AddDefaultCase(const Location& loc);

    void AddStatement(shared_ptr<Statement> statement);

    void SetExpression(shared_ptr<Expression> exp)
    {
        this->exp = IntegralCast::IfNeeded(exp);
    }
    
    virtual Code Compile(LocalContext& parent_ctx);

    virtual string Tree(int indent = 0)
    {
        string str = string(indent, ' ') + "switch\n";
        str += string(indent + indent_length, ' ') + "on\n";
        str += exp->Tree(indent + 2 * indent_length);
        for (size_t i = 0; i < case_bodies.size(); i++)
        {
            str += string(indent + indent_length, ' ') +
                (case_values[i] == nullptr ? "default" : "case " + std::to_string(*case_values[i])) + "\n";
            for (size_t j = 0; j < case_bodies[i].size(); j++)
                str += case_bodies[i][j]->Tree(indent + 2 * indent_length);
        }
        return str;
    }
};


class WhileStatement : public Statement
{
public:
    WhileStatement(shared_ptr<Expression> condition, shared_ptr<StatementBlock> body, const Location& loc)
        : Statement(loc), condition(LogicalCast::IfNeeded(condition)), body(body) {}

    shared_ptr<LogicalExpression> condition;
    shared_ptr<StatementBlock> body;

    virtual Code Compile(LocalContext& ctx);

    virtual string Tree(int indent = 0)
    {
        string str = string(indent, ' ') + "while\n";
        str += string(indent + indent_length, ' ') + "condition\n";
        str += condition->Tree(indent + 2 * indent_length);
        str += string(indent + indent_length, ' ') + "do\n";
        str += body->Tree(indent + 2 * indent_length);
        return str;
    }
};


class ForStatement : public Statement
{
public:
    ForStatement(const vector<shared_ptr<Statement>>& initializer,
        shared_ptr<Expression> condition, shared_ptr<Expression> step,
        shared_ptr<StatementBlock> body, const Location& loc)
        : Statement(loc + body->location), initializer(initializer),
        condition(LogicalCast::IfNeeded(condition)), step(step), body(body) {}

    vector<shared_ptr<Statement>> initializer;
    shared_ptr<LogicalExpression> condition;
    shared_ptr<Expression> step;
    shared_ptr<StatementBlock> body;

    virtual Code Compile(LocalContext& parent_ctx);

    virtual string Tree(int indent = 0)
    {
        string str = string(indent, ' ') + "for\n";
        str += string(indent + indent_length, ' ') + "init\n";
        for (auto i : initializer)
            str += i->Tree(indent + 2 * indent_length);
        str += string(indent + indent_length, ' ') + "condition\n";
        str += condition->Tree(indent + 2 * indent_length);
        str += string(indent + indent_length, ' ') + "step\n";
        str += step->Tree(indent + 2 * indent_length);
        str += string(indent + indent_length, ' ') + "do\n";
        str += body->Tree(indent + 2 * indent_length);
        return str;
    }
};


class Definition
{
public:
    Definition(const Location& loc) : location(loc) {}
    virtual ~Definition() {}

    Location location;

    virtual Code Compile(GlobalContext&) = 0;

    virtual string Tree(int indent = 0) = 0;
};


class FieldDefinition : public Definition
{
public:
    FieldDefinition(const string& name, shared_ptr<SymbolType> type, const Location& loc)
        : Definition(loc), name(name), type(type) {}

    FieldDefinition(const string& name, shared_ptr<SymbolType> type, shared_ptr<Expression> value_expr, const Location& loc);

    string name;
    shared_ptr<SymbolType> type;
    bool has_value = false;
    int value = 0;
    string literal;
    
    virtual Code Compile(GlobalContext& ctx);

    virtual string Tree(int indent = 0)
    {
        string str = string(indent, ' ') + "variable " + name + " : " + type->Name();
        if (has_value)
        {
            if (is_value_type(type))
                str += " = " + std::to_string(value) + "\n";
            else
                str += " = \"" + literal + "\"\n";
        }
        return str;
    }
};


class FunctionDefinition : public Definition
{
public:
    FunctionDefinition(const string& name, shared_ptr<SymbolType> type,
        const vector<shared_ptr<VariableDeclaration>>& params,
        shared_ptr<StatementBlock> body, const Location& loc);

    string name;
    shared_ptr<SymbolType> type;
    vector<shared_ptr<VariableDeclaration>> params;
    shared_ptr<StatementBlock> body;
    
    virtual Code Compile(GlobalContext& ctx);

    virtual string Tree(int indent = 0)
    {
        string str = string(indent, ' ') + "function " + name + " : " + type->Name() + "\n";
        if (params.size() > 0)
        {
            str += string(indent + indent_length, ' ') + "parameters\n";
            for (auto p : params)
                str += p->Tree(indent + 2 * indent_length);
        }
        str += string(indent + indent_length, ' ') + "body\n";
        str += body->Tree(indent + 2 * indent_length);
        return str;
    }
};


class MainFunctionDefinition : public FunctionDefinition
{
public:
    MainFunctionDefinition(shared_ptr<SymbolType> type, shared_ptr<StatementBlock> body, const Location& loc)
        : FunctionDefinition("main", type, vector<shared_ptr<VariableDeclaration>>(), body, loc) {}

    virtual Code Compile(GlobalContext& ctx);
};


class Program
{
public:
    Program(const vector<shared_ptr<Definition>>& definitions)
        : definitions(definitions) {}

    vector<shared_ptr<Definition>> definitions;

    Code Compile(function<void(const Location&, const string&, const string&)> printer);

    virtual string Tree(int indent = 0)
    {
        string str = string(indent, ' ') + "program\n";
        for (auto d : definitions)
            str += d->Tree(indent + indent_length);
        return str;
    }

private:
    static inline string builtin_filename = "builtin";
    static inline const string& builtin_asm_filename = "builtins.asm";
};



#include "ast.hpp"
#include "parser.hpp"


using SyntaxError = yy::parser::syntax_error;


bool UnaryIntegralExpression::Precomputable(int& result)
{
    int a;
    if (exp->Precomputable(a))
    {
        if (op == "+")
            result = a;
        else if (op == "-")
            result = -a;
        else if (op == "~")
            result = ~a;
        return true;
    }
    return false;
}


bool BinaryIntegralExpression::Precomputable(int& result)
{
    int a, b;
    if (exp1->Precomputable(a) && exp2->Precomputable(b))
    {
        if (op == "+")
            result = a + b;
        else if (op == "-")
            result = a - b;
        else if (op == "*")
            result = a * b;
        else if (op == "/")
        {
            if (b == 0)
                return false;
            result = a / b;
        }
        else if (op == "&")
            result = a & b;
        else if (op == "|")
            result = a | b;
        else if (op == "^")
            result = a ^ b;
        return true;
    }
    return false;
}


FunctionCallExpression::FunctionCallExpression(const string& name, const vector<shared_ptr<Expression>>& args, const Location& loc)
    : IntegralExpression(loc), name(name)
{
    if (args.size() > 4)
        throw SyntaxError(args[4]->location + args.rbegin()->get()->location,
            "more that 4 arguments cannot be passed in a function call");
    
    std::transform(args.begin(), args.end(), std::back_inserter(this->args), IntegralCast::IfNeeded);
}


void SwitchStatement::AddCase(shared_ptr<Expression> value_exp, const Location& loc)
{
    auto integral = IntegralCast::IfNeeded(value_exp);
    int value;
    if (!integral->Precomputable(value))
        throw SyntaxError(loc, "case value must be a compile-time constant expression");

    if (std::find_if(case_values.begin(), case_values.end(),
        [value](auto other) { return other != nullptr && *other == value; }) != case_values.end())
        throw SyntaxError(loc, "redecleration of a case with the same value");

    case_values.push_back(std::make_shared<int>(value));
    case_bodies.push_back(vector<shared_ptr<Statement>>());
}


void SwitchStatement::AddDefaultCase(const Location& loc)
{
    if (std::find(case_values.begin(), case_values.end(), nullptr) != case_values.end())
        throw SyntaxError(loc, "redecleration of the default case");

    case_values.push_back(nullptr);
    case_bodies.push_back(vector<shared_ptr<Statement>>());
}


void SwitchStatement::AddStatement(shared_ptr<Statement> statement)
{
    if (case_bodies.size() == 0)
        throw SyntaxError(statement->location, "no case declared before this statement");
    case_bodies.rbegin()->push_back(statement);
}


FieldDefinition::FieldDefinition(const string& name, shared_ptr<SymbolType> type, shared_ptr<Expression> exp, const Location& loc)
    : Definition(loc + exp->location), name(name), type(type)
{
    if (is_value_type(type))
    {
        auto integral = IntegralCast::IfNeeded(exp);
        if (!integral->Precomputable(value))
            throw SyntaxError(this->location, "value assigned to a global variable must be a constant expression");
    }
    else if (is_array_type(type) && *as_array_type(type)->underlying_type == *char_type)
    {
        literal = std::dynamic_pointer_cast<StringLiteral>(exp)->value;
        if (literal.size() + 1 > type->Width())
            throw SyntaxError(this->location, "the assigned string literal does not fit in the array");
    }
    else
        throw SyntaxError(this->location, "a string literal can only initialize an array of characters");

    has_value = true;
}


FunctionDefinition::FunctionDefinition(const string& name, shared_ptr<SymbolType> type,
    const vector<shared_ptr<VariableDeclaration>>& params, shared_ptr<StatementBlock> body, const Location& loc)
    : Definition(loc), name(name), type(type), params(params), body(body)
{
    if (params.size() > 4)
        throw SyntaxError(params[4]->location + params.rbegin()->get()->location,
            "a function definition cannot have more than 4 input parameters");
}


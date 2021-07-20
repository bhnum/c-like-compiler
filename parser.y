// output a C++ class instead of C
%language "c++"
%require "3.5.1" // minimum version
%defines

%define api.token.constructor
%define api.value.type variant
%define parse.assert

// define the location class for location tracking in the scanner
%locations
%define api.location.file "location.hpp"

// debug mode if specified in the command line arguments
%define parse.trace
%define parse.error verbose
%define parse.lac full

// required imports in the header file
%code requires {
    #include <string>

    class Driver;
    #include "ast.hpp"

    struct UntypedVariable
    {
        string name;
        bool array = false;
        int array_size;
        shared_ptr<Expression> value = nullptr;
        Location location;
    };
}

// the parsing context to use (the class instance)
%param { Driver& driver }

// include the driver class in the .cpp file
%code {
    #include "driver.hpp"
}

// %printer { yyo << $$; } <*>;

// define tokens from the scanner
%define api.token.prefix {TOKEN_}
%token
    MINUS           "-"
    PLUS            "+"
    MULTIPLY        "*"
    DIVIDE          "/"
    ASSIGN          "="
    LEFTPAREN       "("
    RIGHTPAREN      ")"
    LEFTBRACKET     "["
    RIGHTBRACKET    "]"

    LESS            "<"
    GREATER         ">"
    EQUAL           "=="
    NOT_EQUAL       "!="
    LESS_EQUAL      "<="
    GREATER_EQUAL   ">="

    LOGICAL_NOT     "!"
    LOGICAL_AND     "&&"
    LOGICAL_OR      "||"

    BITWISE_NOT     "~"
    BITWISE_AND     "&"
    BITWISE_OR      "|"
    BITWISE_XOR     "^"

    DOT             "."
    COMMA           ","
    COLON           ":"

    INT             "int"
    CHAR            "char"
    IF              "if"
    ELSE            "else"
    ELSEIF          "elseif"
    WHILE           "while"
    CONTINUE        "continue"
    BREAK           "break"
    SWITCH          "switch"
    CASE            "case"
    DEFAULT         "default"
    FOR             "for"
    RETURN          "return"
    VOID            "void"
    MAIN            "main"
;

// specify token types
%token <string> IDENTIFIER "identifier";
%token <uint32_t> INT_CONST "integer constant";
%token <char> CHAR_CONST "char constant";
%token <string> STRING_CONST "string constant";
%token EOF 0 "end of file";

%start Start;

// operator precedence
%precedence "="
%left "||"
%left "&&"
%left "|"
%left "^"
%left "&"
%left "==" "!="
%left "<" "<=" ">" ">="
%left "+" "-";
%left "*" "/";
%precedence UnaryPlus UnaryMinus "!" "~";


// types
%type <vector<shared_ptr<Definition>>> DefinitionList;
%type <shared_ptr<FunctionDefinition>> FunctionDefinition;
%type <shared_ptr<MainFunctionDefinition>> MainDefinition;
%type <vector<shared_ptr<FieldDefinition>>> FieldDefinition;
%type <shared_ptr<ValueType>> TypeSpecifier;

%type <vector<shared_ptr<VariableDeclaration>>> ParameterList;
%type <shared_ptr<VariableDeclaration>> ParameterDeclaration;

%type <shared_ptr<StatementBlock>> StatementBlock;
%type <vector<shared_ptr<Statement>>> StatementList;
%type <shared_ptr<Statement>> Statement
%type <vector<shared_ptr<Statement>>> VariableDefinition;
%type <vector<UntypedVariable>> VariableDeclarationList;
%type <UntypedVariable> VariableDeclaration;

%type <shared_ptr<Statement>> SelectionStatement;
%type <vector<std::tuple<shared_ptr<Expression>, shared_ptr<StatementBlock>, Location>>> ElseIfList;
%type <shared_ptr<StatementBlock>> OptionalElse;
%type <shared_ptr<SwitchStatement>> CaseStatementBlock;
%type <shared_ptr<SwitchStatement>> CaseStatementList;

%type <shared_ptr<Statement>> IterationStatement;
%type <vector<shared_ptr<Statement>>> ForInitializer;
%type <shared_ptr<Expression>> OptionalExpression;
%type <shared_ptr<JumpStatement>> JumpStatement;

%type <shared_ptr<Expression>> Expression;
%type <shared_ptr<ArrayAccessExpression>> ArrayAccess;
%type <shared_ptr<FunctionCallExpression>> FunctionCall;
%type <vector<shared_ptr<Expression>>> ArgumentList;
%type <shared_ptr<AssignmentExpression>> Assignment;


%%
Start: DefinitionList MainDefinition {
            $1.push_back($2);
            driver.ast = std::make_shared<Program>($1);
        };

DefinitionList: %empty { $$ = vector<shared_ptr<Definition>>(); }
    | DefinitionList FunctionDefinition { $$ = $1; $$.push_back($2); }
    | DefinitionList FieldDefinition {
            $$ = $1; $$.insert($$.end(), $2.begin(), $2.end());
        }
    ;

FunctionDefinition: TypeSpecifier IDENTIFIER "(" ParameterList ")" StatementBlock {
            $$ = std::make_shared<FunctionDefinition>($2, $1, $4, $6, @1 + @5);
        }
    | VOID IDENTIFIER "(" ParameterList ")" StatementBlock {
            $$ = std::make_shared<FunctionDefinition>($2, void_type, $4, $6, @1 + @5);
        }
    ;

MainDefinition: TypeSpecifier MAIN "(" ")" StatementBlock {
            $$ = std::make_shared<MainFunctionDefinition>($1, $5, @1 + @4);
        }
    | VOID MAIN "(" ")" StatementBlock {
            $$ = std::make_shared<MainFunctionDefinition>(void_type, $5, @1 + @4);
        }
    ;

FieldDefinition : TypeSpecifier VariableDeclarationList "." {
            $$ = vector<shared_ptr<FieldDefinition>>();
            for (auto var : $2)
            {
                if (var.array)
                {
                    auto arraytype = std::make_shared<ArrayType>($1, var.array_size);
                    if (!var.value)
                        $$.push_back(std::make_shared<FieldDefinition>(var.name, arraytype, var.location));
                    else
                        $$.push_back(std::make_shared<FieldDefinition>(var.name, arraytype, var.value, var.location));
                }
                else
                {
                    if (!var.value)
                        $$.push_back(std::make_shared<FieldDefinition>(var.name, $1, var.location));
                    else
                        $$.push_back(std::make_shared<FieldDefinition>(var.name, $1, var.value, var.location));
                }
            }
        };

TypeSpecifier: INT { $$ = int_type; }
    | CHAR { $$ = char_type; }
    ;

ParameterList: %empty { $$ = vector<shared_ptr<VariableDeclaration>>(); }
    | ParameterDeclaration { $$ = vector<shared_ptr<VariableDeclaration>>(); $$.push_back($1); }
    | ParameterList "," ParameterDeclaration { $$ = $1; $$.push_back($3); }
    ;

ParameterDeclaration
    : TypeSpecifier IDENTIFIER { $$ = std::make_shared<VariableDeclaration>($2, $1, @1 + @2); }
    | TypeSpecifier IDENTIFIER "[" "]" {
            auto type = std::make_shared<PointerType>($1);
            $$ = std::make_shared<VariableDeclaration>($2, type, @1 + @4);
        }
    ;

StatementBlock: "<" StatementList ">" { $$ = std::make_shared<StatementBlock>($2, @1 + @3); };

StatementList: %empty { $$ = vector<shared_ptr<Statement>>(); }
    | StatementList Statement { $$ = $1; $$.push_back($2); }
    | StatementList VariableDefinition "." { $$ = $1; $$.insert($$.end(), $2.begin(), $2.end()); }
    ;

Statement: "." { $$ = std::make_shared<Statement>(@1); }
    | Expression "." { $$ = $1; }
    | JumpStatement "." { $$ = $1; }
    | SelectionStatement { $$ = $1; }
    | IterationStatement { $$ = $1; }
    | StatementBlock { $$ = $1; }
    ;

VariableDefinition: TypeSpecifier VariableDeclarationList {
            $$ = vector<shared_ptr<Statement>>();
            for (auto var : $2)
            {
                if (var.array)
                {
                    if (var.value != nullptr)
                        throw yy::parser::syntax_error(var.value->location, "initializing local array variables is not supported");
                    auto arraytype = std::make_shared<ArrayType>($1, var.array_size);
                    $$.push_back(std::make_shared<VariableDeclaration>(var.name, arraytype, var.location));
                }
                else
                {
                    $$.push_back(std::make_shared<VariableDeclaration>(var.name, $1, var.location));
                    if (var.value)
                    {
                        auto var_exp = std::make_shared<VariableExpression>(var.name, var.location);
                        $$.push_back(std::make_shared<AssignmentExpression>(var_exp, var.value));
                    }
                }
            }
        };

VariableDeclarationList
    : VariableDeclaration { $$ = vector<UntypedVariable>(); $$.push_back($1); }
    | VariableDeclarationList "," VariableDeclaration { $$ = $1; $$.push_back($3); }
    ;

VariableDeclaration
    : IDENTIFIER { $$ = { .name = $1, .location = @1}; }
    | IDENTIFIER "=" Expression { $$ = { .name = $1, .value = $3, .location = @1 + @3}; }
    | IDENTIFIER "[" Expression "]" {
            $$ = { .name = $1, .array = true, .location = @1 + @4};
            auto casted = ValueCast::IfNeeded($3);
            if (!casted->Precomputable($$.array_size))
                throw yy::parser::syntax_error($3->location, "array size must be a constant expression");
            if ($$.array_size <= 0)
                throw yy::parser::syntax_error($3->location, "array size cannot be negative or zero");
        }
    | IDENTIFIER "[" Expression "]" "=" STRING_CONST {
            $$ = { .name = $1, .array = true, .value = std::make_shared<StringLiteral>($6, @6), .location = @1 + @4};
            auto casted = ValueCast::IfNeeded($3);
            if (!casted->Precomputable($$.array_size))
                throw yy::parser::syntax_error($3->location, "array size must be a constant expression");
            if ($$.array_size <= 0)
                throw yy::parser::syntax_error($3->location, "array size cannot be negative or zero");
        }
    | IDENTIFIER "[" "]" "=" STRING_CONST {
            $$ = { .name = $1, .array = true, .array_size = int($5.size() + 1),
                .value = std::make_shared<StringLiteral>($5, @5), .location = @1 + @3};
        }
    ;

SelectionStatement
    : IF "(" Expression ")" StatementBlock ElseIfList OptionalElse {
            auto last_else = $7;
            for (auto i = $6.rbegin(); i != $6.rend(); i++)
            {
                auto ifelse = std::make_shared<IfElseStatement>(std::get<0>(*i), std::get<1>(*i), last_else, std::get<2>(*i));
                last_else = std::make_shared<StatementBlock>(ifelse);
            }
            $$ = std::make_shared<IfElseStatement>($3, $5, last_else, $3->location);
        }
    | SWITCH "(" Expression ")" CaseStatementBlock { $$ = $5; $5->SetExpression($3); $5->location = @1 + @4; }
    ;
 
ElseIfList: %empty { $$ = vector<std::tuple<shared_ptr<Expression>, shared_ptr<StatementBlock>, Location>>(); }
    | ElseIfList ELSEIF "(" Expression ")" StatementBlock { $$ = $1; $$.push_back(std::make_tuple($4, $6, @2 + @5)); }
    ;

OptionalElse : %empty { $$ = std::make_shared<StatementBlock>(@$); }
    | ELSE StatementBlock { $$ = $2; }

CaseStatementBlock: "<" CaseStatementList ">" { $$ = $2; };

CaseStatementList: %empty { $$ = std::make_shared<SwitchStatement>(@$); }
    | CaseStatementList CASE Expression ":" { $$ = $1; $$->AddCase($3, @2 + @4); }
    | CaseStatementList DEFAULT ":" { $$ = $1; $$->AddDefaultCase(@2 + @3); }
    | CaseStatementList Statement { $$ = $1; $$->AddStatement($2); }
    | CaseStatementList VariableDefinition "." {
            auto csl = $1;
            $$ = csl;
            std::for_each($2.begin(), $2.end(), [csl](auto s) { csl->AddStatement(s); });
        }
    ;

IterationStatement
    : WHILE "(" Expression ")" StatementBlock { $$ = std::make_shared<WhileStatement>($3, $5, @1 + @4); }
    | FOR "(" ForInitializer "." OptionalExpression "." OptionalExpression ")" StatementBlock {
            $$ = std::make_shared<ForStatement>($3, $5, $7, $9, @1 + @8);
        }
    ;

ForInitializer
    : OptionalExpression { $$ = vector<shared_ptr<Statement>>(); $$.push_back($1); }
    | VariableDefinition { $$ = $1; }
    ;

OptionalExpression: %empty { $$ = std::make_shared<ConstantExpression>(1, @$); }
    | Expression { $$ = $1; }
    ;

JumpStatement
    : CONTINUE { $$ = std::make_shared<ContinueStatement>(@1); }
    | BREAK { $$ = std::make_shared<BreakStatement>(@1); }
    | RETURN { $$ = std::make_shared<ReturnStatement>(@1); }
    | RETURN Expression { $$ = std::make_shared<ReturnStatement>($2, @1); }
    ;

Expression
    : INT_CONST { $$ = std::make_shared<ConstantExpression>($1, @1); }
    | CHAR_CONST { $$ = std::make_shared<ConstantExpression>($1, @1); }
    | IDENTIFIER { $$ = std::make_shared<VariableExpression>($1, @1); }
    | ArrayAccess { $$ = $1; }
    | FunctionCall { $$ = $1; }

    | Expression "+" Expression { $$ = std::make_shared<BinaryValueExpression>("+", $1, $3); }
    | Expression "-" Expression { $$ = std::make_shared<BinaryValueExpression>("-", $1, $3); }
    | Expression "*" Expression { $$ = std::make_shared<BinaryValueExpression>("*", $1, $3); }
    | Expression "/" Expression { $$ = std::make_shared<BinaryValueExpression>("/", $1, $3); }
    | "+" Expression %prec UnaryPlus { $$ = std::make_shared<UnaryValueExpression>("+", $2, @1); }
    | "-" Expression %prec UnaryMinus { $$ = std::make_shared<UnaryValueExpression>("-", $2, @1); }

    | Expression "&" Expression { $$ = std::make_shared<BinaryValueExpression>("&", $1, $3); }
    | Expression "|" Expression { $$ = std::make_shared<BinaryValueExpression>("|", $1, $3); }
    | Expression "^" Expression { $$ = std::make_shared<BinaryValueExpression>("^", $1, $3); }
    | "~" Expression { $$ = std::make_shared<UnaryValueExpression>("~", $2, @1); }

    | Expression "&&" Expression { $$ = std::make_shared<BinaryBooleanExpression>("&&", $1, $3); }
    | Expression "||" Expression { $$ = std::make_shared<BinaryBooleanExpression>("||", $1, $3); }
    | "!" Expression { $$ = std::make_shared<UnaryBooleanExpression>("!", $2, @1); }

    | Expression "==" Expression { $$ = std::make_shared<RelationalExpression>("==", $1, $3); }
    | Expression "!=" Expression { $$ = std::make_shared<RelationalExpression>("!=", $1, $3); }
    | Expression ">" Expression { $$ = std::make_shared<RelationalExpression>(">", $1, $3); }
    | Expression ">=" Expression { $$ = std::make_shared<RelationalExpression>(">=", $1, $3); }
    | Expression "<" Expression { $$ = std::make_shared<RelationalExpression>("<", $1, $3); }
    | Expression "<=" Expression { $$ = std::make_shared<RelationalExpression>("<=", $1, $3); }

    | LEFTPAREN Expression RIGHTPAREN { $$ = $2; $$->location = @1 + @3; }
    | Assignment { $$ = $1; }
    ;

ArrayAccess: IDENTIFIER "[" Expression "]" { $$ = std::make_shared<ArrayAccessExpression>($1, $3, @1 + @4); };

FunctionCall: IDENTIFIER "(" ArgumentList ")" { $$ = std::make_shared<FunctionCallExpression>($1, $3, @1 + @4); };

ArgumentList: %empty { $$ = vector<shared_ptr<Expression>>(); }
    | Expression { $$ = vector<shared_ptr<Expression>>(); $$.push_back($1); }
    | ArgumentList "," Expression { $$ = $1; $$.push_back($3); }
    ;

Assignment
    : IDENTIFIER "=" Expression {
            auto var = std::make_shared<VariableExpression>($1, @1);
            $$ = std::make_shared<AssignmentExpression>(var, $3);
        }
    | ArrayAccess "=" Expression { $$ = std::make_shared<AssignmentExpression>($1, $3); }
    ;

%%

// redirect errors to Driver's error printing
void yy::parser::error(const location_type& location, const std::string& message)
{
    driver.PrintError(location, message);
}

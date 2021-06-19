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
}

// the parsing context to use (the class instance)
%param { Driver& driver }

// include the driver class in the .cpp file
%code {
    #include "driver.hpp"
}

%printer { yyo << $$; } <*>;

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
%token <std::string> IDENTIFIER "identifier";
%token <uint32_t> INT_CONST "integer constant";
%token <char> CHAR_CONST "char constant";
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


%%
Start: DefinitionList;

DefinitionList: MainDefinition
    | Definition DefinitionList
    ;

Definition
    : FunctionDefinition
    | VariableDefinition "."
    ;

FunctionDefinition: TypeSpecifier IDENTIFIER "(" ParameterList ")" StatementBlock
    | VOID IDENTIFIER "(" ParameterList ")" StatementBlock
    ;

MainDefinition: TypeSpecifier MAIN "(" ParameterList ")" StatementBlock
    | VOID MAIN "(" ParameterList ")" StatementBlock
    ;

TypeSpecifier: INT | CHAR;

ParameterList: %empty
    | ParameterDeclaration
    | ParameterDeclaration "," ParameterList
    ;

ParameterDeclaration
    : TypeSpecifier IDENTIFIER
    | TypeSpecifier IDENTIFIER "[" "]"
    ;

StatementBlock: "<" StatementList ">";

StatementList: %empty
    | Statement StatementList
    ;

Statement: "."
    | Expression "."
    | VariableDefinition "."
    | JumpStatement "."
    | SelectionStatement
    | IterationStatement
    | StatementBlock
    ;

VariableDefinition: TypeSpecifier VariableDeclarartionList;

VariableDeclarartionList
    : VariableDeclarartion
    | VariableDeclarartion "," VariableDeclarartionList
    ;

VariableDeclarartion
    : IDENTIFIER
    | IDENTIFIER "=" Expression
    | IDENTIFIER "[" Expression "]"
    ;

SelectionStatement
    : IF "(" Expression ")" StatementBlock ElseIfList
    | IF "(" Expression ")" StatementBlock ElseIfList ELSE StatementBlock
    | SWITCH "(" Expression ")" CaseStatementBlock
    ;
 
ElseIfList: %empty
    | ELSEIF "(" Expression ")" StatementBlock ElseIfList
    ;

CaseStatementBlock: "<" CaseStatementList ">";

CaseStatementList: %empty
    | CASE Expression ":" CaseStatementList
    | DEFAULT ":" CaseStatementList
    | Statement CaseStatementList
    ;

IterationStatement
    : WHILE "(" Expression ")" StatementBlock
    | FOR "(" ForInitializer "." OptionalExpression "." OptionalExpression ")" StatementBlock
    ;

ForInitializer
    : OptionalExpression
    | VariableDefinition
    ;

OptionalExpression: %empty
    | Expression
    ;

JumpStatement
    : CONTINUE
    | BREAK
    | RETURN
    | RETURN Expression
    ;

Expression
    : INT_CONST
    | CHAR_CONST
    | IDENTIFIER
    | ArrayAccess
    | FunctionCall

    | Expression "+" Expression
    | Expression "-" Expression
    | Expression "*" Expression
    | Expression "/" Expression
    | "+" Expression %prec UnaryPlus
    | "-" Expression %prec UnaryMinus

    | Expression "&" Expression
    | Expression "|" Expression
    | Expression "^" Expression
    | "~" Expression

    | Expression "&&" Expression
    | Expression "||" Expression
    | "!" Expression

    | Expression "==" Expression
    | Expression "!=" Expression
    | Expression ">" Expression
    | Expression ">=" Expression
    | Expression "<" Expression
    | Expression "<=" Expression

    | LEFTPAREN Expression RIGHTPAREN
    | Assingment
    ;

ArrayAccess: IDENTIFIER "[" Expression "]";

FunctionCall: IDENTIFIER "(" ArgumentList ")";

ArgumentList: %empty
    | Expression { }
    | Expression "," ArgumentList { }
    ;

Assingment
    : IDENTIFIER "=" Expression
    | ArrayAccess "=" Expression

%%

// redirect errors to Driver's error printing
void yy::parser::error(const location_type& location, const std::string& message)
{
    driver.print_error(location, message);
}

#pragma once

#include <string>
#include "parser.hpp"
#include "ast.hpp"

// define the yylex function for the scanner and parser
#define YY_DECL yy::parser::symbol_type yylex(Driver& driver)
YY_DECL;

class Driver
{
public:
    Driver() {}

    // whether to generate scanner debug traces
    bool trace_scanning = false;
    // whether to generate parser debug traces
    bool trace_parsing = false;

    std::string input_filename;
    std::string friendly_filename;
    std::string tokens_filename = "tokens.txt";
    std::string ast_filename = "ast.txt";
    std::string program_filename = "out.asm";

    shared_ptr<Program> ast;

    int Parse();

    int Scan();

    int Compile();

    // this method is called whenever a syntax error occurs in the parser or in the scanner
    static void PrintError(const yy::parser::location_type& location,
        const std::string& message, const std::string& type = "error");
};

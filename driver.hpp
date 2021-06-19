#pragma once

#include <string>
#include "parser.hpp"

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
    std::string tokens_out_filename = "tokens.txt";

    int parse();

    int scan();

    // this method is called whenever a syntax error occurs in the parser or in the scanner
    void print_error(const yy::parser::location_type& location, const std::string& message);
};

#pragma once

#include <string>
#include <iostream>
#include <memory>

#include "parser.hpp"

// helper class
class NullBuffer : public std::ostream
{
public:
    int overflow(int c) { return c; }
};

class Scanner
{
public:
    // pass filename = "" to read from standard input
    // pass tokens_out_filename = "" to not output token list to file
    Scanner(const std::string& filename = "", const std::string& tokens_out_filename = "tokens.txt", bool trace_scanning = false);
    ~Scanner();

    std::string filename;
    std::string friendly_filename; // for error printing
    std::string tokens_out_filename;
    yy::location location; // for location tracking
    bool trace_scanning;
    std::shared_ptr<std::ostream> tokens_out;
};

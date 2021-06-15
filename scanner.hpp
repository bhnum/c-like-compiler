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
    Scanner(const std::string& filename = "", const std::string& tokens_out_filename = "tokens.txt", bool trace_scanning = false);
    ~Scanner();

    std::string filename;
    std::string friendly_filename;
    std::string tokens_out_filename;
    yy::location location;
    bool trace_scanning;
    std::shared_ptr<std::ostream> tokens_out;
};


#include "driver.hpp"
#include "scanner.hpp"

#include <iomanip>
#include <fstream>

int Driver::parse()
{
    // initialize the scanner and parser and perform parsing
    Scanner scanner(input_filename, tokens_out_filename, trace_scanning);
    yy::parser parse(*this);
    parse.set_debug_level(trace_parsing);
    int result = parse();
    return result;
}

int Driver::scan()
{
    // initialize the scanner
    Scanner scanner(input_filename, tokens_out_filename, trace_scanning);
    try
    {
        // yylex() returns on every scanned token
        // repeat scanning until EOF is encountered
        while (yylex(*this).type != yy::parser::token::TOKEN_EOF);
    }
    catch (const yy::parser::syntax_error& er)
    {
        // print the scan error and return error code
        print_error(er.location, er.what());
        return 1;
    }
    return 0;
}

void Driver::print_error(const yy::parser::location_type& location, const std::string& message)
{
    // print error line and description
    std::cerr << location << ": " << "error: " << message << std::endl;

    // print lines containing the error only for real files, not standard input
    if (input_filename.empty())
        return;
        
    // read the line containing the error and the line before
    std::ifstream input_file(input_filename);
    std::string line, last_line;
    for (int i = 0; i < location.end.line; i++)
    {
        last_line = line;
        std::getline(input_file, line);
    }
    input_file.close();
    
    // decide were to start the error marker
    auto begin_column = location.begin.column;
    if (location.begin.line != location.end.line)
        begin_column = 1;

    // print the line before the error only if the error is not on line 1
    if (location.end.line > 1)
        std::cerr << std::setw(5) << location.end.line - 1 << " | " << last_line << "\n";

    // print the error line
    std::cerr << std::setw(5) << location.end.line << " | " << line << "\n";

    // print the error marker ^~~~
    std::cerr << std::string(5, ' ') << "" << " | " << std::string(begin_column - 1, ' ')
        << '^' << std::string(std::max(0, location.end.column - begin_column - 1), '~') << std::endl;
}

#include "driver.hpp"
#include "scanner.hpp"

#include <iomanip>
#include <fstream>

int Driver::parse()
{
    Scanner scanner(input_filename, tokens_out_filename, trace_scanning);
    yy::parser parse(*this);
    parse.set_debug_level(trace_parsing);
    int res = parse();
    return res;
}

int Driver::scan()
{
    Scanner scanner(input_filename, tokens_out_filename, trace_scanning);
    try
    {
        while (yylex(*this).type != yy::parser::token::TOKEN_EOF);
    }
    catch (const yy::parser::syntax_error& er)
    {
        print_error(er.location, er.what());
        return 1;
    }
    return 0;
}

void Driver::print_error(const yy::parser::location_type& location, const std::string& message)
{
    std::cerr << location << ": " << "error: " << message << std::endl;

    if (input_filename.empty())
        return;
        
    std::ifstream input_file(input_filename);
    std::string line, last_line;
    for (int i = 0; i < location.end.line; i++)
    {
        last_line = line;
        std::getline(input_file, line);
    }
    input_file.close();
    
    auto begin_column = location.begin.column;
    if (location.begin.line != location.end.line)
        begin_column = 1;

    if (location.end.line > 1)
        std::cerr << std::setw(5) << location.end.line - 1 << " | " << last_line << "\n";
    std::cerr << std::setw(5) << location.end.line << " | " << line << "\n"
        << std::string(5, ' ') << "" << " | " << std::string(begin_column - 1, ' ')
        << '^' << std::string(location.end.column - begin_column - 1, '~') << std::endl;
}

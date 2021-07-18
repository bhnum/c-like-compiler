#include "driver.hpp"
#include "scanner.hpp"

#include <iomanip>
#include <fstream>

int Driver::Parse()
{
    // friendly filename for error reporting
    if (input_filename.empty())
        friendly_filename = "stdin";
    else
        friendly_filename = input_filename;

    // initialize the scanner and parser and perform parsing
    Scanner scanner(input_filename, friendly_filename, tokens_filename, trace_scanning);
    yy::parser parse(*this);
    parse.set_debug_level(trace_parsing);
    int result = parse();
    return result;
}

int Driver::Scan()
{
    // friendly filename for error reporting
    if (input_filename.empty())
        friendly_filename = "stdin";
    else
        friendly_filename = input_filename;

    // initialize the scanner
    Scanner scanner(input_filename, friendly_filename, tokens_filename, trace_scanning);
    try
    {
        // yylex() returns on every scanned token
        // repeat scanning until EOF is encountered
        while (yylex(*this).type != yy::parser::token::TOKEN_EOF);
    }
    catch (const yy::parser::syntax_error& er)
    {
        // print the scan error and return error code
        PrintError(er.location, er.what());
        return 1;
    }
    return 0;
}

int Driver::Compile()
{
    int parse_result = Parse();
    if (parse_result != 0) // error
        return parse_result;
        
    std::ofstream outfile;
    outfile.exceptions(std::ofstream::failbit | std::ofstream::badbit);
    try
    {
        outfile.open(program_filename, std::ofstream::trunc);
    }
    catch (const std::ofstream::failure& er)
    {
        throw std::runtime_error("Unable to open file \"" + program_filename + "\": " + er.what());
    }
    
    std::ofstream astfile;
    astfile.exceptions(std::ofstream::failbit | std::ofstream::badbit);
    try
    {
        astfile.open(ast_filename, std::ofstream::trunc);
    }
    catch (const std::ofstream::failure& er)
    {
        throw std::runtime_error("Unable to open file \"" + program_filename + "\": " + er.what());
    }

    astfile << ast->Tree();

    try
    {
        outfile << ast->Compile(PrintError);
    }
    catch(const CompileError& er)
    {
       PrintError(er.location, er.what());
       return 1;
    }

    outfile.close();
    
    return 0;
}

void Driver::PrintError(const yy::parser::location_type& location,
    const std::string& message, const std::string& type)
{
    // print error line and description
    std::cerr << location << ": " << type << ": " << message << std::endl;

    // print lines containing the error only for real files, not standard input
    if (location.begin.filename->empty())
        return;
        
    // read the line containing the error and the line before
    std::ifstream input_file(*location.begin.filename);
    std::string line, last_line;
    for (int i = 0; i < location.end.line; i++)
    {
        last_line = line;
        std::getline(input_file, line);
    }
    if (input_file.fail()) // unable to read file
        return;

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

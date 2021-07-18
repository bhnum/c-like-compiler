#include <iostream>
#include <string>
#include "driver.hpp"

// if _SCAN_ONLY is defined, parsing must be skipped
#if _PARSE_ONLY
bool parse_only = true;
#if _SCAN_ONLY
bool scan_only = true;
#else
bool scan_only = false;
#endif
#else
bool parse_only = false;
bool scan_only = false;
#endif

int main(int argc, char* argv[])
{
    Driver driver;

    // parse input arguments and store the configuration in driver
    for (int i = 1; i < argc; i++)
    {
        // show help
        if (argv[i] == std::string("-h"))
        {
            std::cout << "Usage: parser [filename] [-scan-only]\n"
                << "Do not specify filename to read from standard input" << std::endl;
        }

        // enable parse tracing
        if (argv[i] == std::string("-p"))
            driver.trace_parsing = true;
            
        // enable scan tracing
        else if (argv[i] == std::string("-s"))
            driver.trace_scanning = true;
            
        // do not output tokens to file
        else if (argv[i] == std::string("-nt"))
            driver.tokens_filename = "";

        // output tokens to the specified file
        else if (argv[i] == std::string("-t"))
        {
            i++;
            if (i < argc)
                driver.tokens_filename = argv[i];
            else
            {
                std::cerr << "Missing filename for argument -t" << std::endl;
                return EXIT_FAILURE;
            }
        }

        // output the AST to the specified file
        else if (argv[i] == std::string("-a"))
        {
            i++;
            if (i < argc)
                driver.program_filename = argv[i];
            else
            {
                std::cerr << "Missing filename for argument -a" << std::endl;
                return EXIT_FAILURE;
            }
        }

        // output filename
        else if (argv[i] == std::string("-o"))
        {
            i++;
            if (i < argc)
                driver.ast_filename = argv[i];
            else
            {
                std::cerr << "Missing filename for argument -o" << std::endl;
                return EXIT_FAILURE;
            }
        }

        // read from standard input
        else if (argv[i] == std::string("-"))
            driver.input_filename = "";

        // read from the specified file
        else
            driver.input_filename = argv[i];
    }

    // only scan, skip parsing if _SCAN_ONLY is defined
    if (scan_only)
    {
        // driver.scan() can throw exeptions such as file not found
        try
        {
            // if result is not 0, the scanner has encountered an error
            int result = driver.Scan();
            if (result != 0)
                return EXIT_FAILURE;
        }
        catch (const std::exception& ex)
        {
            // print error and return
            std::cerr << ex.what() << std::endl;
            return EXIT_FAILURE;
        }
        return EXIT_SUCCESS;
    }

    // only parse, skip compile if _PARSE_ONLY is defined
    if (parse_only)
    {
        // driver.parse() can throw exeptions such as file not found
        try
        {
            // if result is not 0, the parser has encountered an error
            int result = driver.Parse();
            if (result != 0)
                return EXIT_FAILURE;
        }
        catch (const std::exception& ex)
        {
            // print error and return
            std::cerr << ex.what() << std::endl;
            return EXIT_FAILURE;
        }
        return EXIT_SUCCESS;
    }

    // driver.compile() can throw exeptions such as file not found
    try
    {
        // if result is not 0, the compiler has encountered an error
        int result = driver.Compile();
        if (result != 0)
            return EXIT_FAILURE;
    }
    catch (const std::exception& ex)
    {
       // print error and return
       std::cerr << ex.what() << std::endl;
       return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
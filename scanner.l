%option noyywrap nounput noinput batch debug

%{ // C++
    #include "scanner.hpp"
    #include "driver.hpp"

    #include <cerrno>
    #include <climits>
    #include <cstdlib>
    #include <cstring> // strerror
    #include <fstream>

    static Scanner* scanner = nullptr;
    
    // a number symbol corresponding to the value in str
    static yy::parser::symbol_type make_INT_CONST(const std::string &str, const yy::parser::location_type& loc)
    {
        try
        {
            uint32_t n = stol(str, nullptr, 10);
            return yy::parser::make_INT_CONST(n, loc);
        }
        catch (const std::out_of_range& ex)
        {
            throw yy::parser::syntax_error(loc, "integer is out of range: " + str);
        }
    }
%}

identifier  [_a-zA-Z][_a-zA-Z0-9]*
intconst   [0-9]+
charconst  '.'
blank       [ \t\r]

%{
    // code run each time a pattern is matched
    #define YY_USER_ACTION loc.columns(yyleng);
%}

%x ml_comment

%%
%{
    std::ostream& tokens_out = *scanner->tokens_out;

    // a handy shortcut to the location held by the driver.
    yy::location& loc = scanner->location;

    // code run each time yylex is called.
    loc.step();
%}

 /* operators */
"-"     { tokens_out << "TOKEN_MINUS\n";            return yy::parser::make_MINUS(loc);            }
"+"     { tokens_out << "TOKEN_PLUS\n";             return yy::parser::make_PLUS(loc);             }
"*"     { tokens_out << "TOKEN_MULTIPLY\n";         return yy::parser::make_MULTIPLY(loc);         }
"/"     { tokens_out << "TOKEN_DIVIDE\n";           return yy::parser::make_DIVIDE(loc);           }
"="     { tokens_out << "TOKEN_ASSIGN\n";           return yy::parser::make_ASSIGN(loc);           }
"("     { tokens_out << "TOKEN_LEFTPAREN\n";        return yy::parser::make_LEFTPAREN(loc);        }
")"     { tokens_out << "TOKEN_RIGHTPAREN\n";       return yy::parser::make_RIGHTPAREN(loc);       }
"["     { tokens_out << "TOKEN_LEFTBRACKET\n";      return yy::parser::make_LEFTBRACKET(loc);      }
"]"     { tokens_out << "TOKEN_RIGHTBRACKET\n";     return yy::parser::make_RIGHTBRACKET(loc);     }
"<"     { tokens_out << "TOKEN_LESS\n";             return yy::parser::make_LESS(loc);             }
">"     { tokens_out << "TOKEN_GREATER\n";          return yy::parser::make_GREATER(loc);          }
"=="    { tokens_out << "TOKEN_EQUAL\n";            return yy::parser::make_EQUAL(loc);            }
"!="    { tokens_out << "TOKEN_NOT_EQUAL\n";        return yy::parser::make_NOT_EQUAL(loc);        }
"<="    { tokens_out << "TOKEN_LESS_EQUAL\n";       return yy::parser::make_LESS_EQUAL(loc);       }
">="    { tokens_out << "TOKEN_GREATER_EQUAL\n";    return yy::parser::make_GREATER_EQUAL(loc);    }
"!"     { tokens_out << "TOKEN_LOGICAL_NOT\n";      return yy::parser::make_LOGICAL_NOT(loc);      }
"&&"    { tokens_out << "TOKEN_LOGICAL_AND\n";      return yy::parser::make_LOGICAL_AND(loc);      }
"||"    { tokens_out << "TOKEN_LOGICAL_OR\n";       return yy::parser::make_LOGICAL_OR(loc);       }
"~"     { tokens_out << "TOKEN_BITWISE_NOT\n";      return yy::parser::make_BITWISE_NOT(loc);      }
"&"     { tokens_out << "TOKEN_BITWISE_AND\n";      return yy::parser::make_BITWISE_AND(loc);      }
"|"     { tokens_out << "TOKEN_BITWISE_OR\n";       return yy::parser::make_BITWISE_OR(loc);       }
"^"     { tokens_out << "TOKEN_BITWISE_XOR\n";      return yy::parser::make_BITWISE_XOR(loc);      }
"."     { tokens_out << "TOKEN_DOT\n";              return yy::parser::make_DOT(loc);              }
","     { tokens_out << "TOKEN_COMMA\n";            return yy::parser::make_COMMA(loc);            }
":"     { tokens_out << "TOKEN_COLON\n";            return yy::parser::make_COLON(loc);            }

 /* keywords */
int         { tokens_out << "TOKEN_INT\n";          return yy::parser::make_INT(loc);          }
char        { tokens_out << "TOKEN_CHAR\n";         return yy::parser::make_CHAR(loc);         }
if          { tokens_out << "TOKEN_IF\n";           return yy::parser::make_IF(loc);           }
else        { tokens_out << "TOKEN_ELSE\n";         return yy::parser::make_ELSE(loc);         }
elseif      { tokens_out << "TOKEN_ELSEIF\n";       return yy::parser::make_ELSEIF(loc);       }
while       { tokens_out << "TOKEN_WHILE\n";        return yy::parser::make_WHILE(loc);        }
continue    { tokens_out << "TOKEN_CONTINUE\n";     return yy::parser::make_CONTINUE(loc);     }
break       { tokens_out << "TOKEN_BREAK\n";        return yy::parser::make_BREAK(loc);        }
switch      { tokens_out << "TOKEN_SWITCH\n";       return yy::parser::make_SWITCH(loc);       }
case        { tokens_out << "TOKEN_CASE\n";         return yy::parser::make_CASE(loc);         }
default     { tokens_out << "TOKEN_DEFAULT\n";      return yy::parser::make_DEFAULT(loc);      }
for         { tokens_out << "TOKEN_FOR\n";          return yy::parser::make_FOR(loc);          }
return      { tokens_out << "TOKEN_RETURN\n";       return yy::parser::make_RETURN(loc);       }
void        { tokens_out << "TOKEN_VOID\n";         return yy::parser::make_VOID(loc);         }
main        { tokens_out << "TOKEN_MAIN\n";         return yy::parser::make_MAIN(loc);         }

 /* constants */
{intconst}  { tokens_out << "TOKEN_INT_CONST\n";        return make_INT_CONST(yytext, loc); }
{charconst} { tokens_out << "TOKEN_CHAR_CONST\n";       return yy::parser::make_CHAR_CONST(yytext[1], loc); }

 /* identifier */
{identifier}    { tokens_out << "TOKEN_IDENTIFIER\n";   return yy::parser::make_IDENTIFIER(yytext, loc); }

 /* track current location */
{blank}+    { loc.step(); }
\n+         { loc.lines(yyleng); loc.step(); }

 /* comments */
"$$".*$             { }
"$*"                { BEGIN(ml_comment); }
<ml_comment>(.)     { loc.step(); }
<ml_comment>(\n)    { loc.lines(yyleng); loc.step(); }
<ml_comment>"*$"    { loc.step(); BEGIN(INITIAL); }

 /* invalid characters */
.   { throw yy::parser::syntax_error(loc, "invalid character: " + std::string(yytext)); }

 /* end of file token */
<<EOF>>     { return yy::parser::make_EOF(loc); }

%%


Scanner::Scanner(const std::string& filename, const std::string& tokens_out_filename, bool trace_scanning)
    : filename(filename), tokens_out_filename(tokens_out_filename), trace_scanning(trace_scanning)
{
    friendly_filename = filename.empty() ?  "standard input" : filename;
    this->location.initialize(&friendly_filename);
    scanner = this;

    yy_flex_debug = trace_scanning;
    if (filename.empty())
        yyin = stdin;
    else if (!(yyin = fopen(filename.c_str(), "r")))
        throw std::runtime_error("Unable to open file \"" + filename + "\": " + strerror(errno));


    if (tokens_out_filename.empty())
        this->tokens_out = std::make_shared<NullBuffer>();
    else
    {
        auto tokens_out = std::make_shared<std::ofstream>();
        tokens_out->exceptions(std::ifstream::failbit | std::ifstream::badbit);
        try
        {
            tokens_out->open(tokens_out_filename, std::ofstream::trunc);
        }
        catch (const std::ofstream::failure& er)
        {
            throw std::runtime_error("Unable to open file \"" + filename + "\": " + er.what());
        }
        this->tokens_out = tokens_out;
    }
}

Scanner::~Scanner()
{
    fclose(yyin);

    scanner = nullptr;
}

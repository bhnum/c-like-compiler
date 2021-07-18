.DEFAULT_GOAL := compiler

headers = parser.hpp scanner.hpp driver.hpp location.hpp ast.hpp translation.hpp
sources = parser.cpp scanner.cpp driver.cpp main.cpp ast.cpp codegen.cpp translation.cpp

.PHONY : all compiler parser scanner clean

all: compiler parser scanner

compiler: compile
parser: parse
scanner: scan
	
compile: $(headers) $(sources)
	g++ $(sources) -o compile -Wall -lm -g -std=c++17

parse: $(headers) $(sources)
	g++ $(sources) -o parse -Wall -lm -g -std=c++17 -D _PARSE_ONLY
	
scan: $(headers) $(sources)
	g++ $(sources) -o scan -Wall -lm -g -std=c++17 -D _SCAN_ONLY

parser.cpp parser.hpp location.hpp: parser.y driver.hpp
	bison -o parser.cpp parser.y --defines=parser.hpp -Wall

scanner.cpp: scanner.l scanner.hpp parser.hpp location.hpp
	flex -o scanner.cpp scanner.l

clean:
	rm -f scanner.cpp parser.hpp parser.cpp location.hpp parse scan compile tokens.txt ast.txt out.asm

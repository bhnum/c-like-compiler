all: parser scanner

headers = parser.hpp scanner.hpp driver.hpp location.hpp
sources = parser.cpp scanner.cpp driver.cpp main.cpp

parser: $(headers) $(sources)
	g++ $(sources) -o parser -Wall -lm -g
	
scanner: $(headers) $(sources)
	g++ $(sources) -o scanner -Wall -lm -g -D _SCAN_ONLY

parser.cpp parser.hpp location.hpp: parser.y driver.hpp
	bison -o parser.cpp parser.y --defines=parser.hpp -Wall

scanner.cpp: scanner.l scanner.hpp parser.hpp location.hpp
	flex -o scanner.cpp scanner.l

.PHONY : clean
clean:
	rm -f scanner.cpp parser.hpp parser.cpp location.hpp parser scanner token*

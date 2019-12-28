.PHONY = all clean cleanall

all: compiler.tab.cpp compiler.l.c
	g++ -std=c++11 -o compiler compiler.tab.c compiler.l.c ast/node.cpp

compiler.tab.cpp: compiler.y
	bison -d compiler.y

compiler.l.c: compiler.l
	flex -o compiler.l.c compiler.l

clean:
	rm -f *.tab.c *.tab.hpp *.l.c

cleanall: clean
	rm -f compiler

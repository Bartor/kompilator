.PHONY = all clean cleanall

all: compiler.tab.cpp compiler.l.c
	g++ -std=c++11 -o compiler front/compiler.tab.cpp front/compiler.l.c front/ast/node.cpp main.cpp

compiler.tab.cpp: front/compiler.y
	bison -d -o front/compiler.tab.cpp front/compiler.y

compiler.l.c: front/compiler.l
	flex -o front/compiler.l.c front/compiler.l

clean:
	rm -f front/compiler.tab.cpp front/compiler.tab.hpp front/compiler.l.c

cleanall: clean
	rm -f compiler

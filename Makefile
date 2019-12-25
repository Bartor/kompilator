.PHONY = all clean cleanall

all: compiler.tab.cpp compiler.l.c
	g++ -o compiler compiler.tab.c compiler.l.c

compiler.tab.cpp: compiler.y
	bison -d compiler.y

compiler.l.c: compiler.l
	flex -o compiler.l.c compiler.l

clean:
	rm -f *.tab.c *.tab.hpp *.l.c

cleanall: clean
	rm -f compiler

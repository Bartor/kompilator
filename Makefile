.PHONY = all clean cleanall

all: compiler.tab.cpp compiler.l.c
	g++ -o compiler compiler.tab.cpp compiler.l.cc

compiler.tab.cpp: compiler.ypp
	bison -d compiler.ypp

compiler.l.c: compiler.l
	flex -o compiler.l.cc compiler.l

clean:
	rm -f *.tab.cpp *.tab.hpp *.l.cc

cleanall: clean
	rm -f compiler

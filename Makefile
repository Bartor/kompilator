.PHONY = all clean cleanall

all: compiler.tab.cpp compiler.l.c
	g++ -std=c++17 -o kompilator front/compiler.tab.c front/compiler.l.c front/*/*.cpp middle/*/*.cpp back/*/*.cpp main.cpp

compiler.tab.cpp: front/compiler.y
	bison -d -o front/compiler.tab.c front/compiler.y

compiler.l.c: front/compiler.l
	flex -o front/compiler.l.c front/compiler.l

clean:
	rm -f */*.tab.h* */*.tab.c* */*.l.c*

cleanall: clean
	rm -f compiler

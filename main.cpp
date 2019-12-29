#include <iostream>
#include "front/ast/node.hpp"

extern DeclarationList *declarations;
extern CommandList *commands;
extern int yyparse();
extern FILE *yyin;

int main(int argc, char **argv) {
    if (argc != 2) {
        std::cerr << "No source file specified" << std::endl;
        return 1;
    }

    FILE *source = fopen(argv[1], "r");
    if (!source) {
        std::cerr << "Can't open " << argv[1] << std::endl;
        return 1;
    }

    yyin = source;
    yyparse();

    if (commands == nullptr) commands = new CommandList();
    if (declarations == nullptr) declarations = new DeclarationList();

    Program *program = new Program(*declarations, *commands);

    std::cout << program->toString() << std::endl;
}
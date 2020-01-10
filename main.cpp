#include <iostream>
#include <fstream>
#include "front/ast/node.h"
#include "middle/abstract_assembler/AbstractAssembler.h"
#include "middle/ast_optimizer/ASTOptimizer.h"

extern DeclarationList *declarations;
extern CommandList *commands;
extern ConstantList *constants;

extern int yyparse();

extern FILE *yyin;

int main(int argc, char **argv) {
    if (argc < 2) {
        std::cerr << "Usage: compiler <source> [destination] [optimize=(0|1)]" << std::endl;
        return 1;
    }

    FILE *source = fopen(argv[1], "r");
    if (!source) {
        std::cerr << "Can't open " << argv[1] << std::endl;
        return 1;
    }

    bool optimize = argc < 4 ? false : atoi(argv[3]) != 0;
    std::cout << "- Analyzing file " << argv[1] << (optimize ? " with optimization -" : " without optimization -") << std::endl;

    std::cout << "- Parsing -" << std::endl;

    yyin = source;
    yyparse();

    std::cout << "Done" << std::endl;

    if (commands == nullptr) commands = new CommandList();
    if (declarations == nullptr) declarations = new DeclarationList();

    Program *program = new Program(*declarations, *commands, *constants);

    std::cout << "-=- A S T -=-" << std::endl;
    std::cout << program->toString() << std::endl;

    if (optimize) {
        std::cout << "- AST Optimization -" << std::endl;
        ASTOptimizer *astOptimizer = new ASTOptimizer(program);
        astOptimizer->optimize();

        std::cout << "-=- OPTIMIZED A S T -=-" << std::endl;
        std::cout << program->toString() << std::endl;
    }

    AbstractAssembler *assembler = new AbstractAssembler(*program);

    std::cout << std::endl << "- Assembling -" << std::endl;

    try {
        InstructionList &assembled = assembler->assemble();

        std::cout << std::endl << "-=- A S M -=-" << std::endl;

        std::ofstream output;
        output.open(argv[2] ? argv[2] : "a.out");
        for (const auto &ins : assembled.getInstructions()) {
            if (!ins->stub) {
                std::cout << std::setbase(10) << ins->getAddress() << ": " << ins->toAssemblyCode(true) << std::endl;
                output << ins->toAssemblyCode(true) << std::endl;
            }
        }

        output.close();
    } catch (std::string errorMessage) {
        std::cout << "=!= COMPILATION ERROR =!=" << std::endl << errorMessage << std::endl;
        return 1;
    }

    return 0;
}
#include <iostream>
#include <fstream>
#include "front/ast/node.h"
#include "middle/abstract_assembler/AbstractAssembler.h"
#include "middle/ast_optimizer/ASTOptimizer.h"
#include "middle/peephole/PeepholeOptimizer.h"

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

    std::cout << argv << std::endl;
    bool optimize = false, verbose = false;
    for (int i = 0; i < argc; i++) {
        if (argv[i][0] == '-' && argv[i][1] == 'v') verbose = true;
        if (argv[i][0] == '-' && argv[i][1] == 'o') optimize = true;
    }
    std::cout << "- Analyzing file " << argv[1] << (optimize ? " with optimization -" : " without optimization -") << std::endl;

    std::cout << "- Parsing -" << std::endl;

    yyin = source;
    yyparse();

    std::cout << "Done" << std::endl;

    if (commands == nullptr) commands = new CommandList();
    if (declarations == nullptr) declarations = new DeclarationList();

    Program *program = new Program(*declarations, *commands, *constants);

    if (verbose)std::cout << "-=- A S T -=-" << std::endl;
    if (verbose) std::cout << program->toString() << std::endl;

    if (optimize) {
        std::cout << "- AST Optimization -" << std::endl;
        ASTOptimizer *astOptimizer = new ASTOptimizer(program);
        astOptimizer->optimize();

        if (verbose) std::cout << "-=- OPTIMIZED A S T -=-" << std::endl;
        if (verbose) std::cout << program->toString() << std::endl;
    }

    AbstractAssembler *assembler = new AbstractAssembler(*program);

    std::cout << std::endl << "- Assembling -" << std::endl;

    try {
        InstructionList &assembled = assembler->assemble();

        if (verbose) std::cout << std::endl << "-=- A S M -=-" << std::endl;

        std::ofstream output;
        output.open(argv[2] ? argv[2] : "a.out");

        for (const auto &ins : assembled.getInstructions()) {
            if (!ins->stub) {
                if (verbose) std::cout << std::setbase(10) << ins->getAddress() << ": " << ins->toAssemblyCode(true) << std::endl;
                if (!optimize) output << ins->toAssemblyCode(true) << std::endl;
            }
        }

        if (optimize) {
            std::cout << "- ASM Optimization -" << std::endl;
            PeepholeOptimizer *peepholeOptimizer = new PeepholeOptimizer(assembled);

            peepholeOptimizer->optimize();
            assembled.seal(false);

            if (verbose) std::cout << std::endl << "-=- OPTIMIZED A S M -=-" << std::endl;

            for (const auto &ins : assembled.getInstructions()) {
                if (!ins->stub) {
                    if (verbose) std::cout << std::setbase(10) << ins->getAddress() << ": " << ins->toAssemblyCode(true) << std::endl;
                    output << ins->toAssemblyCode(true) << std::endl;
                }
            }
            output.close();
        }

    } catch (std::string errorMessage) {
        std::cout << "=!= COMPILATION ERROR =!=" << std::endl << errorMessage << std::endl;
        return 1;
    }

    return 0;
}
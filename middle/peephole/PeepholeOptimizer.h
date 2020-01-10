#ifndef COMPILER_PEEPHOLEOPTIMIZER_H
#define COMPILER_PEEPHOLEOPTIMIZER_H

#include "../../back/asm/asm.h"
#include "../../back/asm/InstructionList.h"

#include <iostream>

class PeepholeOptimizer {
private:
    InstructionList &instructions;

    bool removeUselessStoreLoads();

    bool removeUselessStoreLoadis();
public:
    PeepholeOptimizer(InstructionList &instructionList) : instructions(instructionList) {}

    void optimize();
};

#endif //COMPILER_PEEPHOLEOPTIMIZER_H

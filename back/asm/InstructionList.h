#include "asm.h"
#include <vector>

#ifndef COMPILER_INSTRUCTIONLIST_H
#define COMPILER_INSTRUCTIONLIST_H

class InstructionList {
private:
    Stub *afterLast;
    Stub *first;
    std::vector<Instruction *> instructions;
public:
    Stub *end();
    Instruction *start();

    std::vector<Instruction *> &getInstructions();

    InstructionList &append(InstructionList &list);

    InstructionList &append(Instruction *instruction);

    void seal();

    InstructionList();
};


#endif //COMPILER_INSTRUCTIONLIST_H

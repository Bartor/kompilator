#include "asm.h"
#include <vector>

#ifndef COMPILER_INSTRUCTIONLIST_H
#define COMPILER_INSTRUCTIONLIST_H

/**
 * A container for assembler instructions, allows appending
 * single instructions and other lists to its end as well as
 * getting the last element (represented by a Stub instruction
 * object) for the sake of jumps.
 */
class InstructionList {
private:
    Stub *afterLast;
    std::vector<Instruction *> instructions;
public:
    Stub *end();
    Instruction *start();

    std::vector<Instruction *> &getInstructions();

    InstructionList &append(InstructionList &list);

    InstructionList &append(Instruction *instruction);

    /**
     * "Seals" the list by giving each instruction on it a proper address,
     * excluding the Stubs which receive the address of the next instruction
     * on the list. Can be called multiple after modifying the list (e.g. by
     * the PeepholeOptimizer) to reapply the addresses.
     * @param addHalt Decided if Halt instruction will be added to list's end.
     */
    void seal(bool addHalt);

    InstructionList();
};


#endif //COMPILER_INSTRUCTIONLIST_H

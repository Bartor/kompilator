#ifndef COMPILER_PEEPHOLEOPTIMIZER_H
#define COMPILER_PEEPHOLEOPTIMIZER_H

#include "../../back/asm/asm.h"
#include "../../back/asm/InstructionList.h"

#include <iostream>

/**
 * An optimizer removing useless asm instructions on a
 * compiled set of them.
 */
class PeepholeOptimizer {
private:
    InstructionList &instructions;

    /**
     * Removes a LOAD x instruction which follows directly
     * a STORE x instruction as long as any jump doesn't
     * reference it.
     * @return True if any LOAD was removed; false otherwise.
     */
    bool removeUselessStoreLoads();

    bool removeUselessStoreLoadis();
public:
    PeepholeOptimizer(InstructionList &instructionList) : instructions(instructionList) {}

    void optimize(bool verbose);
};

#endif //COMPILER_PEEPHOLEOPTIMIZER_H

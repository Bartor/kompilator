#include "Constant.h"

InstructionList &Constant::generateConstant() {
    InstructionList &instructions = *new InstructionList();

    // TODO optimize this monstrosity
    instructions.instructions.push_back(new Sub(*new ResolvableAddress(0))); // SUB 0
    for (int i = 0; i < value; i++) {
        instructions.instructions.push_back(new Inc()); // INC
    }
    instructions.instructions.push_back(new Store(address)); // STORE address

    return instructions;
}
#include "Constant.h"

InstructionList &Constant::generateConstant() {
    InstructionList &instructions = *new InstructionList();

    // TODO optimize this monstrosity
    instructions.append(new Sub(*new ResolvableAddress(0))); // SUB 0
    for (int i = 0; i < value; i++) {
        instructions.append(new Inc()); // INC
    }
    instructions.append(new Store(address)); // STORE address

    return instructions;
}

ResolvableAddress &Constant::getAddress() {
    return address;
}

std::string Constant::toString() {
    return "Constant<" + std::to_string(value) + ">";
}
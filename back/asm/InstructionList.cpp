#include "InstructionList.h"

InstructionList::InstructionList() {
    afterLast = new Stub();
    instructions.push_back(afterLast); // last element stub
}

std::vector<Instruction *> &InstructionList::getInstructions() {
    return instructions;
}

Stub *InstructionList::end() {
    return afterLast;
}

Instruction *InstructionList::start() {
    return instructions.front();
}

InstructionList &InstructionList::append(InstructionList &list) {
    end()->redirect(list.end());
    instructions.insert(instructions.end() - 1, list.instructions.begin(),
                        list.instructions.end());
    return *this;
}

void InstructionList::seal() {
    instructions.push_back(new Halt());

    long long counter = 0;
    for (const auto &ins : instructions) { // instruction address resolution
        ins->setAddress(counter);
        if (!ins->stub) {
            counter++; // stubs use next instruction's address
        }
    }
}

InstructionList &InstructionList::append(Instruction *instruction) {
    instructions.insert(instructions.end() - 1, instruction); // maintain the stub end
    return *this;
}
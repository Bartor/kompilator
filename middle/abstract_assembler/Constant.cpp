#include "Constant.h"

InstructionList &Constant::generateConstant(Constants *constantsData, ResolvableAddress &primaryAccumulator, ResolvableAddress &secondaryAccumulator) {
    InstructionList &instructions = *new InstructionList();
    instructions.append(new Sub(primaryAccumulator));

    if (value == -1) {
        instructions.append(new Dec());
    } else if (value == 1) {
        instructions.append(new Inc());
    } else if (value) {
        long long newValue = value < 0 ? -value : value;
        int bitPos = log2(newValue) + 1;

        while (--bitPos > 0) {
            if (newValue & (1 << bitPos)) {
                instructions.append(new Inc());
            }
            instructions.append(new Shift(constantsData->getConstant(1)->getAddress()));
        }
        if (newValue & 1) instructions.append(new Inc());

        if (value < 0) {
            instructions.append(new Store(secondaryAccumulator));
            instructions.append(new Sub(secondaryAccumulator));
            instructions.append(new Sub(secondaryAccumulator));
        }
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
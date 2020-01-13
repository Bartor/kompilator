#include "Constant.h"

char *lltoa(unsigned long long val, int base) {
    static char buf[64] = {0};

    int i = 62;
    int sign = (val < 0);
    if (sign) val = -val;

    if (val == 0) return "0";

    for (; val && i; --i, val /= base) {
        buf[i] = "0123456789abcdef"[val % base];
    }

    return &buf[i + 1];
}

InstructionList &Constant::generateConstant(Constants *constantsData, ResolvableAddress &primaryAccumulator, ResolvableAddress &secondaryAccumulator) {
    InstructionList &instructions = *new InstructionList();
    instructions.append(new Sub(primaryAccumulator));

    if (value == -1) {
        instructions.append(new Dec());
    } else if (value == 1) {
        instructions.append(new Inc());
    } else if (abs(value) < 12) { // todo make this value somehow smart
        long long valCpy = value;

        if (valCpy > 0) {
            while (valCpy-- > 0) instructions.append(new Inc());
        } else {
            while (valCpy++ < 0) instructions.append(new Dec());
        }
    } else if (value) {
        unsigned long long valCpy = value > 0 ? value : -value;
        std::string number = std::string(lltoa(valCpy, 2));

        for (int i = 0; i < number.size(); i++) {
            if (number[i] == '1') instructions.append(new Inc());
            if (i != number.size() - 1) {
                instructions.append(new Shift(constantsData->getConstant(1)->getAddress()));
            }
        }

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
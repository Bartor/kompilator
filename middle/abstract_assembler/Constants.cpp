#include "Constants.h"

Constants::Constants(long long int startAddress) : currentAddress(startAddress) {
    one = new Constant(1, *new ResolvableAddress(currentAddress++));
    minusOne = new Constant(-1, *new ResolvableAddress(currentAddress++));
}

Constant *Constants::addConstant(long long value) {
    if (value == 1 || value == -1) return nullptr;

    for (const auto &constant : constants) {
        if (constant->value == value) {
            return nullptr;
        }
    }

    ResolvableAddress &address = *new ResolvableAddress(currentAddress++); // assign a new address to it
    Constant *constant = new Constant(value, address); // create it with new value

    if (constants.size() == 0) {
        constants.push_back(constant);
    } else {
        for (long long i = 0; i < constants.size(); i++) {
            if (llabs(constants[i]->value) > llabs(value)) {
                constants.insert(constants.begin() + i, constant);
                break;
            }
            if (i == constants.size() - 1) {
                constants.push_back(constant);
                break;
            }
        }
    }
    return constant;
}

void Constants::removeConstant(Constant *constant) {
    for (long long i = 0; i < constants.size(); i++) {
        if (constants[i] == constant) {
            constants.erase(constants.begin() + i);
            break;
        }
    }
}

InstructionList &Constants::oneAndMinusOne(ResolvableAddress &primaryAccumulator) {
    InstructionList &list = *new InstructionList();
    list.append(new Sub(primaryAccumulator))
            .append(new Inc())
            .append(new Store(one->getAddress()))
            .append(new Dec())
            .append(new Dec())
            .append(new Store(minusOne->getAddress()));

    return list;
}

Constant *Constants::getConstant(long long value) {
    if (value == 1) return one;
    else if (value == -1) return minusOne;
    else {
        for (const auto &constant : constants) {
            if (constant->value == value) return constant;
        }
        return nullptr;
    }
}
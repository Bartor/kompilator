#include "Constants.h"

Constant *Constants::addConstant(long long value) {
    for (const auto &constant : constants) {
        if (constant->value == value) {
            return nullptr;
        }
    }

    ResolvableAddress &address = *new ResolvableAddress(currentAddress++); // assign a new address to it
    Constant *constant = new Constant(value, address); // create it with new value
    constants.push_back(constant); // add it
    return (constant);
}

Constant *Constants::getConstant(long long value) {
    for (const auto &constant : constants) {
        if (constant->value == value) return constant;
    }
    return nullptr;
}

long long Constants::size() {
    return constants.size();
}
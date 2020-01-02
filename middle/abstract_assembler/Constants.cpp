#include "Constants.h"

void Constants::addConstant(long long value) {
    bool found = false; // check if we already have this particular constant
    for (const auto &constant : constants) {
        if (constant->value == value) {
            found = true;
            break;
        }
    }
    if (!found) { // if we don't have it, create it
        ResolvableAddress &address = *new ResolvableAddress(currentAddress++); // assign a new address to it
        Constant *constant = new Constant(value, address); // create it with new value
        constants.push_back(constant); // add it
    }
}

Constant *Constants::getConstant(long long value) {
    for (const auto &constant : constants) {
        if (constant->value == value) return constant;
    }
    return nullptr;
}
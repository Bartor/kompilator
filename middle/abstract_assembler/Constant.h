#include "ResolvableAddress.h"
#include "../../back/asm/asm.h"

#ifndef COMPILER_CONSTANT_H
#define COMPILER_CONSTANT_H

class Constant {
private:
    ResolvableAddress &address;
public:
    long long value;

    InstructionList &generateConstant();
    Constant(long long value, ResolvableAddress &address) : value(value), address(address) {}
};

#endif //COMPILER_CONSTANT_H

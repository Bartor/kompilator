#include "ResolvableAddress.h"
#include "../../back/asm/asm.h"
#include "../../back/asm/InstructionList.h"

#include <string>

#ifndef COMPILER_CONSTANT_H
#define COMPILER_CONSTANT_H

class Constant {
private:
    ResolvableAddress &address;
public:
    long long value;

    ResolvableAddress &getAddress();

    InstructionList &generateConstant();

    std::string toString();

    Constant(long long value, ResolvableAddress &address) : value(value), address(address) {}
};

#endif //COMPILER_CONSTANT_H

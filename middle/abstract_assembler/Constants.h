#ifndef COMPILER_CONSTANTS_H
#define COMPILER_CONSTANTS_H

class Constant;

#include <algorithm>
#include "Constant.h"

/**
 * Allows for finding addresses of available constants when compiling.
 */
class Constants {
private:
    Constant *one;
    Constant *minusOne;

    std::vector<Constant *> constants;
    long long currentAddress;
public:
    Constant *addConstant(long long value);

    Constant *getConstant(long long value);

    void removeConstant(Constant *constant);

    std::vector<Constant *> getConstants() {
        return constants;
    }

    InstructionList &oneAndMinusOne(ResolvableAddress &primaryAccumulator);

    Constants(long long startAddress = 2);
};

#endif //COMPILER_CONSTANTS_H

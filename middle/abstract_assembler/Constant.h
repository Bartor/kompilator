#include "ResolvableAddress.h"
#include "../../back/asm/asm.h"
#include "../../back/asm/InstructionList.h"
#include <math.h>
#include <string>

#ifndef COMPILER_CONSTANT_H
#define COMPILER_CONSTANT_H

class ResolvableAddress;
class Constants;

#include "Constants.h"
#include "ResolvableAddress.h"

/**
 * A single constant used in program.
 */
class Constant {
private:
    ResolvableAddress &address;
public:
    long long value;

    ResolvableAddress &getAddress();

    /**
     * Generates a code to create the constant and store it in memory
     * on the assigned address.
     */
    InstructionList &generateConstant(Constants *constantsData, ResolvableAddress &primaryAccumulator, ResolvableAddress &secondaryAccumulator);

    std::string toString();

    Constant(long long value, ResolvableAddress &address) : value(value), address(address) {}
};

#endif //COMPILER_CONSTANT_H

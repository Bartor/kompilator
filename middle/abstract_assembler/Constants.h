#include "Constant.h"

#ifndef COMPILER_CONSTANTS_H
#define COMPILER_CONSTANTS_H

class Constants {
private:
    std::vector<Constant *> constants;
    long long currentAddress;
public:
    void addConstant(long long value);

    Constant *getConstant(long long value);

    Constants(long long startAddress = 2) : currentAddress(startAddress) {}
};

#endif //COMPILER_CONSTANTS_H

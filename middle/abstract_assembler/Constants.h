#ifndef COMPILER_CONSTANTS_H
#define COMPILER_CONSTANTS_H

class Constant;

#include "Constant.h"

class Constants {
private:
    std::vector<Constant *> constants;
    long long currentAddress;
public:
    Constant *addConstant(long long value);

    Constant *getConstant(long long value);

    std::vector<Constant *> getConstants() {
        return constants;
    }

    long long lastAddress();

    Constants(long long startAddress = 2) : currentAddress(startAddress) {}
};

#endif //COMPILER_CONSTANTS_H

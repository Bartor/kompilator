#include "ResolvableAddress.h"
#include <type_traits>
#include <string>

#ifndef COMPILER_VARIABLES_H
#define COMPILER_VARIABLES_H

class Variable {
private:
    ResolvableAddress &address;
public:
    std::string &name;
    long long size;
    bool readOnly;

    ResolvableAddress &getAddress();

    virtual std::string toString() = 0;

    Variable(std::string &name, ResolvableAddress &address, bool readOnly = false)
            : name(name), address(address), readOnly(readOnly) {}

    virtual ~Variable() {}
};

class NumberVariable : public Variable {
public:
    NumberVariable(std::string &name, ResolvableAddress &address, bool readOnly = false)
            : Variable(name, address, readOnly) {
        size = 1;
    }

    virtual std::string toString();
};

class NumberArrayVariable : public Variable {
public:
    long long start;
    long long end;

    NumberArrayVariable(std::string &name, ResolvableAddress &address, long long start, long long end,
                        bool readOnly = false)
            : Variable(name, address, readOnly), start(start), end(end) {
        size = end - start + 1;
    }

    virtual std::string toString();
};

class TemporaryVariable : public Variable {
public:
    TemporaryVariable(std::string &name, ResolvableAddress &address)
            : Variable(name, address, true) {
        size = 1;
    }

    virtual std::string toString();
};

#endif //COMPILER_VARIABLES_H

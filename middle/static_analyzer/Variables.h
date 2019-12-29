#include <string>

#ifndef COMPILER_VARIABLES_H
#define COMPILER_VARIABLES_H

class ResolvableAddress {
private:
    long long address;
public:
    ResolvableAddress(long long startingAddress = 0) : address(address) {}

    long long getAddress();

    void setAddress(long long newAddress);
};

class Variable {
private:
    ResolvableAddress &address;
public:
    std::string &name;

    ResolvableAddress &getAddress();

    Variable(std::string &name) : name(name) {}

    virtual ~Variable() {}
};

class NumberVariable : public Variable {
public:
    NumberVariable(std::string &name) : Variable(name) {}
};

class NumberArrayVariable : public Variable {
public:
    long long start;
    long long end;

    NumberArrayVariable(std::string &name, long long start, long long end) : Variable(name), start(start), end(end) {}
};

#endif //COMPILER_VARIABLES_H

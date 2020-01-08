#include "Variables.h"
#include "../../front/ast/node.h"
#include <vector>
#include <string>
#include <iostream>

#ifndef COMPILER_SCOPEDVARIABLES_H
#define COMPILER_SCOPEDVARIABLES_H

class ScopedVariables {
private:
    std::vector<Variable *> variables;

    long long currentAddress;
public:
    void pushVariableScope(Variable *variable);

    void popVariableScope(long long times = 1);

    ResolvableAddress &resolveAddress(AbstractIdentifier &identifier);

    Variable *resolveVariable(std::string &name);

    ScopedVariables(long long startAddress = 8) : currentAddress(startAddress) {}
};

#endif //COMPILER_SCOPEDVARIABLES_H

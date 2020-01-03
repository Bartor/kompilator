#include "Variables.h"
#include "../../front/ast/node.h"
#include <vector>

#ifndef COMPILER_SCOPEDVARIABLES_H
#define COMPILER_SCOPEDVARIABLES_H

class ScopedVariables {
private:
    std::vector<Variable *> variables;

    Variable *resolveVariable(std::string &name);

    long long currentAddress;
public:
    void pushVariableScope(Variable *variable);

    void popVariableScope(long long times = 1);

    ResolvableAddress &resolveAddress(AbstractIdentifier &identifier);

    ScopedVariables(long long startAddress = 8) : currentAddress(startAddress) {}
};

#endif //COMPILER_SCOPEDVARIABLES_H

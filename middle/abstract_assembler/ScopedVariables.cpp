#include "ScopedVariables.h"

void ScopedVariables::pushVariableScope(Variable *variable) {
    variable->getAddress().setAddress(currentAddress);
    variables.push_back(variable);
    currentAddress += variable->size;
}

void ScopedVariables::popVariableScope(long long times) {
    while (times-- > 0) {
        currentAddress -= variables.back()->size;
        variables.pop_back();
    }
}

Variable *ScopedVariables::resolveVariable(Variable *variableToCompare) {
    for (const auto &variable : variables) {
        if (variable->name == variableToCompare->name) return variable;
    }
    return nullptr;
}
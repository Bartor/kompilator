#include "ScopedVariables.h"

void ScopedVariables::pushVariableScope(Variable *variable) {
    variables.push_back(variable);
}

void ScopedVariables::popVariableScope(long long times = 1) {
    while (times-- > 0) variables.pop_back();
}

Variable *ScopedVariables::resolveVariable(Variable *variableToCompare) {
    for (const auto &variable : variables) {
        if (variable.name == variableToCompare->name) return variable;
    }
    return nullptr;
}
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

Variable *ScopedVariables::resolveVariable(std::string &name) {
    for (const auto &variable : variables) {
        if (variable->name == name) return variable;
    }
    return nullptr;
}

ResolvableAddress &ScopedVariables::resolveAddress(AbstractIdentifier &identifier) {
    Variable *resolved = resolveVariable(identifier.name);

    if (resolved == nullptr) {
        throw "No variable in current scope: " + identifier.toString(0);
    }
    return resolved->getAddress();
}
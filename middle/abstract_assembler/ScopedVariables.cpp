#include "ScopedVariables.h"

void ScopedVariables::pushVariableScope(Variable *variable) {
    if (!dynamic_cast<TemporaryVariable *>(variable)) {
        for (const auto &var : variables) {
            if (var->name == variable->name) throw "Redeclaration of variable " + var->name;
        }
    }

    variable->getAddress().setAddress(currentAddress);
    variables.push_back(variable);
    currentAddress += variable->size;
}

void ScopedVariables::popVariableScope(long long times) {
    while (times-- > 0) {
        Variable *v = variables.back();

        currentAddress -= variables.back()->size;
        variables.pop_back();
    }
}

Variable *ScopedVariables::resolveVariable(std::string &name) {
    for (const auto &variable : variables) {
        if (variable->name == name) return variable;
    }
    throw "No variable in current scope: " + name;
}

ResolvableAddress &ScopedVariables::resolveAddress(AbstractIdentifier &identifier) {
    Variable *resolved = resolveVariable(identifier.name);

    if (resolved == nullptr) {
        throw "No variable in current scope: " + identifier.toString(0);
    }
    return resolved->getAddress();
}
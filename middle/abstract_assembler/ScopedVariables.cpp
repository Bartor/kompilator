#include "ScopedVariables.h"

void ScopedVariables::pushVariableScope(Variable *variable) {
    variable->getAddress().setAddress(currentAddress);
    variables.push_back(variable);
    currentAddress += variable->size;

    std::cout << "+" << variable->toString() << std::endl;
    for (const auto &var : variables) {
        std::cout << "  " << var->toString() << std::endl;
    }
}

void ScopedVariables::popVariableScope(long long times) {
    while (times-- > 0) {
        Variable *v = variables.back();

        currentAddress -= variables.back()->size;
        variables.pop_back();

        std::cout << "-" << v->toString() << std::endl;
        for (const auto &var : variables) {
            std::cout << "  " << var->toString() << std::endl;
        }
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
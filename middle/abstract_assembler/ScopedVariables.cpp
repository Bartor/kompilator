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
    Variable * resolved = nullptr;
    try {
        auto varId = dynamic_cast<VariableIdentifier &>(identifier); // a
        resolved = dynamic_cast<NumberVariable *>(resolveVariable(varId.name));
    } catch (std::bad_cast e) {
        try {
            auto accessId = dynamic_cast<AccessIdentifier &>(identifier);
            resolved = dynamic_cast<NumberArrayVariable *>(resolveVariable(accessId.name));
        } catch (std::bad_cast ee) {
            try {
                auto varAccessId = dynamic_cast<VariableAccessIdentifier &>(identifier);
                resolved = dynamic_cast<NumberArrayVariable *>(resolveVariable(varAccessId.name));
                throw "NOT IMPLEMENTED"; // TODO implement
            } catch (std::bad_cast eee) {}
        }
    }

    if (resolved == nullptr) {
        throw "No variable in current scope: " + identifier.toString(0);
    }
    return resolved->getAddress();
}
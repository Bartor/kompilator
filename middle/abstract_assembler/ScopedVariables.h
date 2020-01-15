#include "Variables.h"
#include "../../front/ast/node.h"
#include <vector>
#include <string>
#include <iostream>

#ifndef COMPILER_SCOPEDVARIABLES_H
#define COMPILER_SCOPEDVARIABLES_H

/**
 * A variable stack; represents variables available in
 * current scope of compilation.
 */
class ScopedVariables {
private:
    std::vector<Variable *> variables;

    long long currentAddress;
public:
    /**
     * Add a new variable to the scope.
     */
    void pushVariableScope(Variable *variable);

    /**
     * Remove variables from the top of the scope.
     * @param times How many variables to remove.
     */
    void popVariableScope(long long times = 1);

    /**
     * Resolves an address of some identifier.
     * @param identifier Identifier we want to get address of.
     * @return Address representing this identifier.
     */
    ResolvableAddress &resolveAddress(AbstractIdentifier &identifier);

    /**
     * Resolves a whole variable (and not only its address).
     * @param name A name of a variable we want to resolve.
     */
    Variable *resolveVariable(std::string &name);

    ScopedVariables(long long startAddress = 8) : currentAddress(startAddress) {}
};

#endif //COMPILER_SCOPEDVARIABLES_H

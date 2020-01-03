#include "../../front/ast/node.h"
#include "../../back/asm/asm.h"
#include "ScopedVariables.h"
#include "Constants.h"
#include <vector>
#include <iostream>
#include <iomanip>

#ifndef COMPILER_ABSTRACTASSEMBLER_H
#define COMPILER_ABSTRACTASSEMBLER_H

class AbstractAssembler {
private:
    Program &program;
    ScopedVariables &scopedVariables = *new ScopedVariables();
    Constants &constants = *new Constants(2);

    void getVariablesFromDeclarations();

    InstructionList &assembleConstants();

    InstructionList &assembleCommands(CommandList &commandList);

    InstructionList &assembleCondition(Condition &condition);

    ResolvableAddress &resolveValue(AbstractValue &value);

public:
    AbstractAssembler(Program &program) : program(program) {}

    InstructionList &assemble();
};

#endif //COMPILER_ABSTRACTASSEMBLER_H

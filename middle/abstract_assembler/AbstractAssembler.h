#include "../../front/ast/node.h"
#include "../../back/asm/asm.h"
#include "../../back/asm/InstructionList.h"
#include "ScopedVariables.h"
#include "Constants.h"
#include <vector>
#include <iostream>
#include <iomanip>
#include <typeinfo>

#ifndef COMPILER_ABSTRACTASSEMBLER_H
#define COMPILER_ABSTRACTASSEMBLER_H

class Resolution {
public:
    bool writable;
    bool indirect;
    long long temporaryVars;
    InstructionList &instructions;
    ResolvableAddress &address;

    Resolution(InstructionList &instructionList, ResolvableAddress &address, bool indirect, long long temporaryVars = 0, bool writable = true)
            : instructions(instructionList), address(address), indirect(indirect), temporaryVars(temporaryVars), writable(writable) {}
};

class AbstractAssembler {
private:
    const long long accumulatorNumber = 3; // this assembler assumes use of three accumulators at the start of the mmry

    ResolvableAddress &primaryAccumulator = *new ResolvableAddress(0); // used mainly by vm instructions
    ResolvableAddress &secondaryAccumulator = *new ResolvableAddress(1); // used with calculations
    ResolvableAddress &expressionAccumulator = *new ResolvableAddress(2); // used to remember loaded value

    Program &program;
    ScopedVariables *scopedVariables;
    Constants *constants;

    void getVariablesFromDeclarations();

    InstructionList &generateBoilerplate();

    InstructionList &assembleConstants();

    InstructionList &assembleCommands(CommandList &commandList);

    InstructionList &assembleCondition(Condition &condition, InstructionList &codeBlock);

    Resolution *assembleExpression(AbstractExpression &expression);

    Resolution *resolve(AbstractValue &value);

    Resolution *resolve(AbstractIdentifier &identifier);
public:
    AbstractAssembler(Program &program) : program(program) {}

    InstructionList &assemble();
};

#endif //COMPILER_ABSTRACTASSEMBLER_H

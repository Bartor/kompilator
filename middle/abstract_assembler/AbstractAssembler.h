#include "../../front/ast/node.h"
#include "../../back/asm/asm.h"
#include "ScopedVariables.h"
#include "Constants.h"
#include <vector>
#include <iostream>
#include <iomanip>

#ifndef COMPILER_ABSTRACTASSEMBLER_H
#define COMPILER_ABSTRACTASSEMBLER_H

class ExpressionResolution {
public:
    virtual ~ExpressionResolution() {}
};

class InstructionExpressionResolution : public ExpressionResolution {
public:
    InstructionList &instructionList;

    InstructionExpressionResolution(InstructionList &instructionList) : instructionList(instructionList) {}
};

class AddressExpressionResolution : public ExpressionResolution {
public:
    ResolvableAddress &address;

    AddressExpressionResolution(ResolvableAddress &address) : address(address) {}
};

class AbstractAssembler {
private:
    ResolvableAddress &accumulator = *new ResolvableAddress(0); // p0 is a start of the memory and and a accumulator
    Program &program;
    ScopedVariables *scopedVariables;
    Constants *constants;

    void getVariablesFromDeclarations();

    InstructionList &generateBoilerplate();

    InstructionList &assembleConstants();

    InstructionList &assembleCommands(CommandList &commandList);

    InstructionList &assembleCondition(Condition &condition);

    ResolvableAddress &resolveValue(AbstractValue &value);

    ExpressionResolution *assembleExpression(AbstractExpression &expression);

public:
    AbstractAssembler(Program &program) : program(program) {}

    InstructionList &assemble();
};

#endif //COMPILER_ABSTRACTASSEMBLER_H

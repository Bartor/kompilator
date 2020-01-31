#include "../../front/ast/node.h"
#include "../../back/asm/asm.h"
#include "../../back/asm/InstructionList.h"
#include "ScopedVariables.h"
#include "Constants.h"
#include <vector>
#include <iostream>
#include <iomanip>
#include <typeinfo>
#include <cstdlib>

#ifndef COMPILER_ABSTRACTASSEMBLER_H
#define COMPILER_ABSTRACTASSEMBLER_H

enum ResolutionType {
    CONSTANT,
    VARIABLE,
    CONSTANT_ARRAY,
    VARIABLE_ARRAY
};

/**
 * A value resolution; it tells how to load a value (using either
 * the instructions when indirect - variable array access - or
 * address when direct - variable, static array access). Tells if a
 * variable can be modified (writable) and what it really is
 * (important with constants).
 */
class Resolution {
public:
    bool writable;
    bool indirect;
    long long temporaryVars;
    InstructionList &instructions;
    ResolvableAddress &address;
    ResolutionType type;

    Resolution(InstructionList &instructionList, ResolvableAddress &address, ResolutionType type, bool indirect, long long temporaryVars = 0, bool writable = true)
            : instructions(instructionList), address(address), indirect(indirect), temporaryVars(temporaryVars), writable(writable), type(type) {}
};

/**
 * A result of something that produces results in accumulator
 * e.g. expressions.
 */
class SimpleResolution {
public:
    long long temporaryVars;
    InstructionList &instructions;

    SimpleResolution(InstructionList &instructionList, long long temporaryVar) : instructions(instructionList), temporaryVars(temporaryVar) {}
};

/**
 * A main, monolithic compiler class transforming AST into asm instruction
 * objects list.
 * @note This class looks as it shouldn't belong to a OOP application;
 * one would say that each command from the list should have an overridden
 * "assemble" method and then this gigantic monstrosity wouldn't need to
 * exist; and that is correct although I decided to do it this way as
 * this actually allows AST to be ABSTRACT (aka each node only holds
 * pure information and doesn't actually KNOW what those information
 * mean). That's why this class looks more like a procedural program
 * wrapped in class rather than a fully fledged OOP. Sorry!
 */
class AbstractAssembler {
private:
    const long long accumulatorNumber = 3; // this assembler assumes use of three accumulators at the start of the mmry

    ResolvableAddress &primaryAccumulator = *new ResolvableAddress(0); // used mainly by vm instructions
    ResolvableAddress &secondaryAccumulator = *new ResolvableAddress(1); // used with calculations
    ResolvableAddress &expressionAccumulator = *new ResolvableAddress(2); // used to remember loaded value

    Program &program;
    ScopedVariables *scopedVariables;
    Constants *constants;

    /**
     * Adds variables declared in Program to scoped variables.
     */
    void getVariablesFromDeclarations(bool verbose);

    /**
     * Adds constants from Program as well as 1 and -1 to constants.
     */
    void prepareConstants(bool verbose);

    /**
     * Generates instruction list used to create and store constants
     * at the beginning of the execution; basically creates a primitive
     * runtime environment for the assembled program.
     * @return A list of instructions generating and storing constants used
     * across the assembled program.
     */
    InstructionList &assembleConstants();

    /**
     * Remove constants not used in given instructions from Constants.
     * @param instructions Instructions to search constants use for.
     */
    void removeUselessConstants(InstructionList &instructions);

    /**
     * Assembles a list of AST nodes; this function differentiates each
     * command and knows what to do with each.
     * @param commandList AST nodes to be compiled.
     * @return Asm instruction list doing what the commands said to.
     */
    SimpleResolution *assembleCommands(CommandList &commandList);

    /**
     * Creates a new instruction block used to jump over or execute
     * a given instruction block.
     * @param condition A condition to be met.
     * @param codeBlock A block to be jumped over or executed if the
     * condition is met.
     */
    SimpleResolution *assembleCondition(Condition &condition, InstructionList &codeBlock);

    /**
     * Creates a instruction block to deal with maths.
     * @param expression A maths to be executed.
     * @return An instruction list loading maths' result into the
     * accumulator.
     */
    SimpleResolution *assembleExpression(AbstractExpression &expression);

    /**
     * Resolves a value; that means it returns the value's address
     * or instruction loading value's address into the accumulator
     * to use LOADI or STOREI on.
     * @param value A value we want to load.
     * @return A resolution object with information needed to load it.
     */
    Resolution *resolve(AbstractValue &value);

    /**
     * Resolves an identifier; it returns its address or
     * instructions used to load the address into the accumulator.
     * @param identifier An identifier we need to load.
     * @return A resolution object with information needed to load it.
     */
    Resolution *resolve(AbstractIdentifier &identifier, bool checkInit);

public:
    AbstractAssembler(Program &program) : program(program) {}

    /**
     * Single-click assembly!
     * @return Ready instruction list (but with Stubs, for optimizations).
     */
    InstructionList &assemble(bool verbose);
};

#endif //COMPILER_ABSTRACTASSEMBLER_H

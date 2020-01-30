#include "AbstractAssembler.h"

std::string TEMPORARY_NAMES = "!TEMP";

void AbstractAssembler::prepareConstants(bool verbose) {
    constants = new Constants(accumulatorNumber);

    for (const auto num : program.constants.constants) {
        Constant *constantPointer = constants->addConstant(num);

        if (constantPointer && verbose) {
            std::cout << constantPointer->toString() << std::endl;
        }
    }
}

InstructionList &AbstractAssembler::assembleConstants() {
    InstructionList &list = constants->oneAndMinusOne(primaryAccumulator);

    for (const auto &constant : constants->getConstants()) {
        InstructionList &generatedCode = constant->generateConstant(constants, primaryAccumulator, secondaryAccumulator);

        list.append(generatedCode);
    }

    return list;
}

void AbstractAssembler::getVariablesFromDeclarations(bool verbose) {
    scopedVariables = new ScopedVariables(
            1 // next address...
            + accumulatorNumber // ...offset by accumulators...
            + program.constants.constants.size() * 2 // ...constants times two for additional special constants (1024 = 2^10, so we may need 10 as well)
            + program.declarations.declarations.size() * 2 // ...and possible constants (array starting addresses + array starting indexes)
    );

    for (const auto &declaration : program.declarations.declarations) {
        if (auto numDecl = dynamic_cast<IdentifierDeclaration *>(declaration)) {
            NumberVariable *var = new NumberVariable(numDecl->name, *new ResolvableAddress());
            scopedVariables->pushVariableScope(var);
            if (verbose) std::cout << var->toString() << std::endl;
        } else if (auto arrDecl = dynamic_cast<ArrayDeclaration *>(declaration)) {
            NumberArrayVariable *var = new NumberArrayVariable(arrDecl->name, *new ResolvableAddress(), arrDecl->start, arrDecl->end);
            scopedVariables->pushVariableScope(var);
            if (verbose) std::cout << var->toString() << std::endl;

            // this is an array start address; it must be included in generated constants to use with indirect
            // variable addressing mode (e.g. a[b])
            // constants are to be generated in the next step, based on program constants
            // this isn't the best solutions and should be changed one day
            program.constants.constants.push_back(var->getAddress().getAddress());
        }
    }
}

SimpleResolution *AbstractAssembler::assembleCommands(CommandList &commandList) {
    InstructionList &instructions = *new InstructionList();

    for (const auto &command : commandList.commands) {
        long long tempVars = 0;

        if (auto commandList = dynamic_cast<CommandList *>(command)) { // NESTED COMMANDLIST
            SimpleResolution *assmebledCommands = assembleCommands(*commandList);
            instructions.append(assmebledCommands->instructions);
            tempVars += assmebledCommands->temporaryVars;
        } else if (auto readNode = dynamic_cast<Read *>(command)) { // READ
            Resolution *idRes = resolve(readNode->identifier, false);

            if (!idRes->writable) throw "Trying to read to non-writable variable " + readNode->identifier.name;

            Get *get = new Get();
            if (idRes->indirect) {
                TemporaryVariable *tempAddressVar = new TemporaryVariable(TEMPORARY_NAMES, *new ResolvableAddress());
                scopedVariables->pushVariableScope(tempAddressVar);

                instructions.append(idRes->instructions).append(new Store(tempAddressVar->getAddress())).append(get).append(new Storei(tempAddressVar->getAddress()));
                tempVars += 1;
            } else {
                instructions.append(get).append(new Store(idRes->address));
            }

            tempVars += idRes->temporaryVars;
        } else if (auto writeNode = dynamic_cast<Write *>(command)) { // WRITE
            Resolution *valRes = resolve(writeNode->value);
            Put *put = new Put();

            if (valRes->indirect) {
                instructions.append(valRes->instructions).append(new Loadi(primaryAccumulator)).append(put);
            } else {
                instructions.append(new Load(valRes->address)).append(put);
            }

            tempVars += valRes->temporaryVars;
        } else if (auto assignNode = dynamic_cast<Assignment *>(command)) { // ASSIGN
            Resolution *idRes = resolve(assignNode->identifier, false);

            if (!idRes->writable) throw "Trying to assign to non-writable variable " + assignNode->identifier.name;

            SimpleResolution *expRes = assembleExpression(assignNode->expression);

            if (idRes->indirect) {
                TemporaryVariable *tempAddressVar = new TemporaryVariable(TEMPORARY_NAMES, *new ResolvableAddress());
                scopedVariables->pushVariableScope(tempAddressVar);

                instructions.append(idRes->instructions).append(new Store(tempAddressVar->getAddress())).append(expRes->instructions).append(new Storei(tempAddressVar->getAddress()));
                tempVars += 1;
            } else {
                instructions.append(expRes->instructions).append(new Store(idRes->address));
            }

            tempVars += idRes->temporaryVars + expRes->temporaryVars;
        } else if (auto ifNode = dynamic_cast<If *>(command)) { // IF
            SimpleResolution *codeResolution = assembleCommands(ifNode->commands); // assemble inner instructions
            SimpleResolution *conditionResolution = assembleCondition(ifNode->condition, codeResolution->instructions); // assemble condition

            instructions.append(conditionResolution->instructions) // add condition code
                    .append(codeResolution->instructions); // add inner block code

            tempVars += conditionResolution->temporaryVars;
        } else if (auto ifElseNode = dynamic_cast<IfElse *>(command)) { // IF ELSE
            SimpleResolution *ifCodeResolution = assembleCommands(ifElseNode->commands);
            SimpleResolution *conditionResolution = assembleCondition(ifElseNode->condition, ifCodeResolution->instructions);
            SimpleResolution *elseCodeResolution = assembleCommands(ifElseNode->elseCommands);

            Jump *jump = new Jump(elseCodeResolution->instructions.end());
            ifCodeResolution->instructions.append(jump);

            instructions.append(conditionResolution->instructions)
                    .append(ifCodeResolution->instructions)
                    .append(elseCodeResolution->instructions);

            tempVars += conditionResolution->temporaryVars;
        } else if (auto whileNode = dynamic_cast<While *>(command)) { // WHILE
            SimpleResolution *codeResolution = assembleCommands(whileNode->commands);

            if (whileNode->doWhile) {
                InstructionList *jumpBlock = new InstructionList();
                Jump *jump = new Jump(codeResolution->instructions.start());
                jumpBlock->append(jump);

                SimpleResolution *conditionResolution = assembleCondition(whileNode->condition, *jumpBlock);
                instructions.append(codeResolution->instructions)
                        .append(conditionResolution->instructions)
                        .append(*jumpBlock);

                tempVars += conditionResolution->temporaryVars;
            } else {
                SimpleResolution *conditionResolution = assembleCondition(whileNode->condition, codeResolution->instructions);
                Jump *jump = new Jump(conditionResolution->instructions.start());
                codeResolution->instructions.append(jump);

                instructions.append(conditionResolution->instructions)
                        .append(codeResolution->instructions);

                tempVars += conditionResolution->temporaryVars;
            }
        } else if (auto forNode = dynamic_cast<For *>(command)) {
            NumberVariable *iterator = new NumberVariable(
                    forNode->variableName,
                    *new ResolvableAddress(),
                    true
            );
            iterator->initialized = true;
            scopedVariables->pushVariableScope(iterator); // create iterator BEFORE commands would use it
            tempVars += 1;

            Resolution *startRes = resolve(forNode->startValue); // resolve start and end
            Resolution *endRes = resolve(forNode->endValue);
            tempVars += startRes->temporaryVars + endRes->temporaryVars;

            ResolvableAddress *iterationEndAddress; // we will need it in comparing

            if (endRes->type == CONSTANT) {
                iterationEndAddress = &endRes->address;
            } else {
                TemporaryVariable *iterationEnd = new TemporaryVariable(TEMPORARY_NAMES, *new ResolvableAddress());
                scopedVariables->pushVariableScope(iterationEnd);
                tempVars += 1;

                iterationEndAddress = &iterationEnd->getAddress();

                instructions.append(endRes->instructions)
                        .append(endRes->indirect ? static_cast<Instruction *>(new Loadi(primaryAccumulator)) : static_cast<Instruction *>(new Load(endRes->address)))
                        .append(new Store(*iterationEndAddress));
            }

            SimpleResolution *codeResolution = assembleCommands(forNode->commands); // assemble iterated commands

            InstructionList &forLoopInstructions = *new InstructionList();
            forLoopInstructions.append(new Sub(*iterationEndAddress))
                    .append(forNode->reversed ? static_cast<Instruction *>(new Jneg(codeResolution->instructions.end())) : static_cast<Instruction *>(new Jpos(codeResolution->instructions.end())));

            codeResolution->instructions.append(new Load(iterator->getAddress()))
                    .append(forNode->reversed ? static_cast<Instruction *>(new Dec()) : static_cast<Instruction *>(new Inc()))
                    .append(new Store(iterator->getAddress()))
                    .append(new Jump(forLoopInstructions.start()));

            instructions.append(startRes->instructions) // load start instructions
                    .append(startRes->indirect ? static_cast<Instruction *>(new Loadi(primaryAccumulator)) : static_cast<Instruction *>(new Load(startRes->address)))
                    .append(new Store(iterator->getAddress())); // store it in the iterator

            instructions.append(forLoopInstructions)
                    .append(codeResolution->instructions);
        }

        scopedVariables->popVariableScope(tempVars);
    }

    return new SimpleResolution(
            instructions,
            0
    );
}

SimpleResolution *AbstractAssembler::assembleCondition(Condition &condition, InstructionList &codeBlock) {
    InstructionList &instructions = *new InstructionList();

    Resolution *lhsResolution = resolve(condition.lhs);
    Resolution *rhsResolution = resolve(condition.rhs);

    bool incResolved = false;
    if (rhsResolution->type == CONSTANT || lhsResolution->type == CONSTANT) {
        bool rhsFlag = rhsResolution->type == CONSTANT;

        NumberValue &constantValue = dynamic_cast<NumberValue &>(rhsFlag ? condition.rhs : condition.lhs);
        bool negative = constantValue.value < 0;
        long long valCopy = llabs(constantValue.value);

        if (valCopy < 10) {
            if (rhsFlag) {
                instructions.append(lhsResolution->instructions)
                        .append(lhsResolution->indirect ? static_cast<Instruction *>(new Loadi(primaryAccumulator)) : static_cast<Instruction *>(new Load(lhsResolution->address)));
                while (valCopy-- > 0) {
                    instructions.append(negative ? static_cast<Instruction *>(new Inc()) : static_cast<Instruction *>(new Dec()));
                }
                incResolved = true;
            } else if (condition.type == EQUAL || condition.type == NOT_EQUAL) {
                incResolved = true;
                instructions.append(rhsResolution->instructions)
                        .append(rhsResolution->indirect ? static_cast<Instruction *>(new Loadi(primaryAccumulator)) : static_cast<Instruction *>(new Load(rhsResolution->address)));
                while (valCopy-- > 0) {
                    instructions.append(negative ? static_cast<Instruction *>(new Inc()) : static_cast<Instruction *>(new Dec()));
                }
            }
        }
    }

    if (!incResolved) {
        if (rhsResolution->indirect) {
            Store *store = new Store(expressionAccumulator);
            Sub *sub = new Sub(expressionAccumulator);

            instructions.append(rhsResolution->instructions)
                    .append(rhsResolution->indirect ? static_cast<Instruction *>(new Loadi(primaryAccumulator)) : static_cast<Instruction *>(new Load(rhsResolution->address)))
                    .append(store)
                    .append(lhsResolution->instructions)
                    .append(lhsResolution->indirect ? static_cast<Instruction *>(new Loadi(primaryAccumulator)) : static_cast<Instruction *>(new Load(lhsResolution->address)))
                    .append(sub);
        } else {
            Sub *sub = new Sub(rhsResolution->address);
            instructions.append(lhsResolution->instructions)
                    .append(lhsResolution->indirect ? static_cast<Instruction *>(new Loadi(primaryAccumulator)) : static_cast<Instruction *>(new Load(lhsResolution->address)))
                    .append(sub);
        }
    }

    switch (condition.type) {
        case NOT_EQUAL: {
            Jzero *jzero = new Jzero(codeBlock.end()); // jump to end of code block if they don't subtruct to zero

            instructions.append(jzero);
        }
            break;
        case EQUAL: {
            Jzero *jzero = new Jzero(codeBlock.start()); // jump to start of code block if the subtract to zero
            Jump *jump = new Jump(codeBlock.end()); // otherwise, jump to its end

            instructions.append(jzero)
                    .append(jump);
        }
            break;
        case LESS: {
            Jzero *jzero = new Jzero(codeBlock.end()); // jump if it's zero
            Jpos *jpos = new Jpos(codeBlock.end()); // jump to end if a - b < 0 => a < b

            instructions.append(jzero)
                    .append(jpos);
        }
            break;
        case GREATER: {
            Jzero *jzero = new Jzero(codeBlock.end()); // jump if it's zero
            Jneg *jneg = new Jneg(codeBlock.end()); // jump to end if a - b > 0 => a > b

            instructions.append(jzero)
                    .append(jneg);
        }
            break;
        case LESS_OR_EQUAL: {
            Jpos *jpos = new Jpos(codeBlock.end()); // jump to end if a - b < 0 => a < b

            instructions.append(jpos);
        }
            break;
        case GREATER_OR_EQUAL: {
            Jneg *jneg = new Jneg(codeBlock.end()); // jump to end if a - b > 0 => a > b

            instructions.append(jneg);
        }
            break;
    }

    return new SimpleResolution(
            instructions,
            lhsResolution->temporaryVars + rhsResolution->temporaryVars
    );
}

/***
 * Creates instructions which load result of an expression into accumulator
 * @param expression
 * @return
 */
SimpleResolution *AbstractAssembler::assembleExpression(AbstractExpression &expression) {
    try {
        UnaryExpression &unaryExpression = dynamic_cast<UnaryExpression &>(expression);
        Resolution *valueResolution = resolve(unaryExpression.value);
        if (valueResolution->indirect) {
            valueResolution->instructions.append(new Loadi(primaryAccumulator));
        } else {
            valueResolution->instructions.append(new Load(valueResolution->address));
        }
        return new SimpleResolution(valueResolution->instructions, valueResolution->temporaryVars);
    } catch (std::bad_cast e) {
        try {
            BinaryExpression &binaryExpression = dynamic_cast<BinaryExpression &>(expression);
            InstructionList &instructionList = *new InstructionList();

            long long tempVars = 0;

            Resolution *lhsResolution = resolve(binaryExpression.lhs);
            Resolution *rhsResolution = resolve(binaryExpression.rhs);

            Instruction *lhsLoad = lhsResolution->indirect ? static_cast<Instruction *>(new Loadi(primaryAccumulator)) : static_cast<Instruction *>(new Load(lhsResolution->address));
            Instruction *rhsLoad = rhsResolution->indirect ? static_cast<Instruction *>(new Loadi(primaryAccumulator)) : static_cast<Instruction *>(new Load(rhsResolution->address));

            bool modulo = false;
            switch (binaryExpression.type) {
                case ADDITION: {
                    bool incResolved = false;
                    if (rhsResolution->type == CONSTANT || lhsResolution->type == CONSTANT) {
                        NumberValue &constantValue = dynamic_cast<NumberValue &>(rhsResolution->type == CONSTANT ? binaryExpression.rhs : binaryExpression.lhs);

                        if (llabs(constantValue.value) < 10) {
                            incResolved = true;

                            instructionList.append(rhsResolution->type == CONSTANT ? lhsResolution->instructions : rhsResolution->instructions)
                                    .append(rhsResolution->type == CONSTANT ? lhsLoad : rhsLoad);


                            long long valCopy = constantValue.value;

                            while (valCopy != 0) {
                                if (valCopy < 0) {
                                    instructionList.append(new Dec());
                                } else {
                                    instructionList.append(new Inc());
                                }
                                valCopy = valCopy + (valCopy < 0 ? 1 : -1);
                            }
                        }
                    }

                    if (incResolved) break;

                    if (rhsResolution->indirect) {
                        Store *storeLeft = new Store(secondaryAccumulator);
                        Add *add = new Add(secondaryAccumulator);

                        instructionList.append(lhsResolution->instructions)
                                .append(lhsLoad)
                                .append(storeLeft)
                                .append(rhsResolution->instructions)
                                .append(rhsLoad)
                                .append(add);
                    } else {
                        Add *add = new Add(rhsResolution->address);

                        instructionList.append(lhsResolution->instructions)
                                .append(lhsLoad)
                                .append(add);
                    }
                }
                    break;
                case SUBTRACTION: {
                    bool incResolved = false;
                    if (rhsResolution->type == CONSTANT) {
                        NumberValue &constantValue = dynamic_cast<NumberValue &>(binaryExpression.rhs);

                        if (llabs(constantValue.value) < 10) {
                            incResolved = true;

                            instructionList.append(lhsResolution->instructions)
                                    .append(lhsLoad);

                            long long valCopy = constantValue.value;

                            while (valCopy != 0) {
                                if (valCopy < 0) {
                                    instructionList.append(new Inc());
                                } else {
                                    instructionList.append(new Dec());
                                }
                                valCopy = valCopy + (valCopy < 0 ? 1 : -1);
                            }
                        }
                    }

                    if (incResolved) break;

                    if (rhsResolution->indirect) {
                        Sub *sub = new Sub(expressionAccumulator);
                        Store *storeRight = new Store(secondaryAccumulator);

                        instructionList.append(rhsResolution->instructions)
                                .append(rhsLoad)
                                .append(storeRight)
                                .append(lhsResolution->instructions)
                                .append(lhsLoad)
                                .append(sub);
                    } else {
                        Sub *sub = new Sub(rhsResolution->address);

                        instructionList.append(lhsResolution->instructions)
                                .append(lhsLoad)
                                .append(sub);
                    }
                }
                    break;
                case MODULO:
                    modulo = true;
                case DIVISION: {
                    bool incResolved = false;
                    if (rhsResolution->address.getAddress() == lhsResolution->address.getAddress()) { // if they are the same thing, return 1 or -1
                        incResolved = true;
                        if (modulo) {
                            instructionList.append(new Sub(primaryAccumulator));
                        } else {
                            if (rhsResolution->type == CONSTANT) {
                                NumberValue &numberValue = dynamic_cast<NumberValue &>(binaryExpression.rhs);

                                instructionList.append(new Sub(primaryAccumulator));
                                if (numberValue.value > 0) {
                                    instructionList.append(new Inc());
                                }
                                if (numberValue.value != 0) {
                                    instructionList.append(new Dec());
                                }
                            } else {
                                Sub *resetAccToZero = new Sub(primaryAccumulator);

                                instructionList.append(rhsResolution->instructions)
                                        .append(rhsLoad)
                                        .append(new Jzero(resetAccToZero))
                                        .append(new Sub(primaryAccumulator))
                                        .append(new Inc())
                                        .append(new Jump(instructionList.end()))
                                        .append(resetAccToZero);
                            }
                        }
                    } else if (rhsResolution->type == CONSTANT || lhsResolution->type == CONSTANT) { // know constants optimization
                        bool rhsFlag = rhsResolution->type == CONSTANT;

                        NumberValue &constantValue = dynamic_cast<NumberValue &>(rhsFlag ? binaryExpression.rhs : binaryExpression.lhs);
                        bool negative = constantValue.value < 0;
                        long long valCopy = llabs(constantValue.value);

                        if (rhsFlag && valCopy == 1) { // a/1, a/-1
                            incResolved = true;

                            if (modulo) {
                                instructionList.append(new Sub(primaryAccumulator));
                            } else {
                                instructionList.append(lhsResolution->instructions)
                                        .append(lhsLoad);

                                if (negative) {
                                    instructionList.append(new Store(expressionAccumulator))
                                            .append(new Sub(expressionAccumulator))
                                            .append(new Sub(expressionAccumulator));
                                }
                            }
                        } else if (rhsFlag && valCopy == 2) {
                            incResolved = true;

                            if (modulo) { // a mod 2
                                instructionList.append(lhsResolution->instructions)
                                        .append(lhsLoad)
                                        .append(new Jzero(instructionList.end()))
                                        .append(new Store(expressionAccumulator))
                                        .append(new Shift(constants->getConstant(-1)->getAddress()))
                                        .append(new Shift(constants->getConstant(1)->getAddress()))
                                        .append(new Sub(expressionAccumulator));
                                if (!negative) {
                                    instructionList.append(new Jzero(instructionList.end()))
                                            .append(new Inc())
                                            .append(new Inc());
                                }
                            } else { // a div 2
                                InstructionList &positiveABlock = *new InstructionList();
                                positiveABlock.append(new Shift(constants->getConstant(-1)->getAddress()))
                                        .append(new Jump(instructionList.end()));

                                instructionList.append(lhsResolution->instructions)
                                        .append(lhsLoad)
                                        .append(new Jzero(instructionList.end()))
                                        .append(new Jpos(positiveABlock.start()))
                                        .append(new Store(expressionAccumulator))
                                        .append(new Shift(constants->getConstant(1)->getAddress()))
                                        .append(new Shift(constants->getConstant(-1)->getAddress()))
                                        .append(new Sub(expressionAccumulator)); // parity check

                                InstructionList &evenA = *new InstructionList();
                                evenA.append(new Load(expressionAccumulator))
                                        .append(new Shift(constants->getConstant(-1)->getAddress()))
                                        .append(new Jump(instructionList.end()));

                                instructionList.append(new Jzero(evenA.start()))
                                        .append(new Load(expressionAccumulator))
                                        .append(new Shift(constants->getConstant(-1)->getAddress()))
                                        .append(new Dec())
                                        .append(new Jump(instructionList.end()))
                                        .append(evenA)
                                        .append(positiveABlock);
                            }
                        } else if (valCopy == 0) { // a = 0 or b = 0
                            incResolved = true;
                            instructionList.append(new Sub(primaryAccumulator));
                        }
                    }

                    if (incResolved) break;


                    TemporaryVariable *scaledDivisor = new TemporaryVariable(TEMPORARY_NAMES, *new ResolvableAddress());
                    TemporaryVariable *sign = new TemporaryVariable(TEMPORARY_NAMES, *new ResolvableAddress());
                    TemporaryVariable *divisor = new TemporaryVariable(TEMPORARY_NAMES, *new ResolvableAddress());
                    TemporaryVariable *dividend = new TemporaryVariable(TEMPORARY_NAMES, *new ResolvableAddress());
                    TemporaryVariable *multiple = new TemporaryVariable(TEMPORARY_NAMES, *new ResolvableAddress());
                    TemporaryVariable *remain = new TemporaryVariable(TEMPORARY_NAMES, *new ResolvableAddress());
                    scopedVariables->pushVariableScope(scaledDivisor);
                    scopedVariables->pushVariableScope(sign);
                    scopedVariables->pushVariableScope(divisor);
                    scopedVariables->pushVariableScope(dividend);
                    scopedVariables->pushVariableScope(multiple);
                    scopedVariables->pushVariableScope(remain);
                    tempVars += 6;

                    InstructionList &zeroResultBlock = *new InstructionList();
                    zeroResultBlock.append(new Sub(primaryAccumulator));

                    InstructionList &firstWhileBlock = *new InstructionList();
                    firstWhileBlock.append(new Load(scaledDivisor->getAddress()))
                            .append(new Sub(dividend->getAddress()));

                    InstructionList &doWhileBlock = *new InstructionList();
                    doWhileBlock.append(new Load(remain->getAddress()))
                            .append(new Sub(scaledDivisor->getAddress()));

                    firstWhileBlock.append(new Jpos(doWhileBlock.start()))
                            .append(new Jzero(doWhileBlock.start()))
                            .append(new Load(scaledDivisor->getAddress()))
                            .append(new Shift(constants->getConstant(1)->getAddress()))
                            .append(new Store(scaledDivisor->getAddress()))
                            .append(new Load(multiple->getAddress()))
                            .append(new Shift(constants->getConstant(1)->getAddress()))
                            .append(new Store(multiple->getAddress()))
                            .append(new Jump(firstWhileBlock.start()));

                    InstructionList &afterIfBlock = *new InstructionList();
                    afterIfBlock.append(new Load(scaledDivisor->getAddress()))
                            .append(new Shift(constants->getConstant(-1)->getAddress()))
                            .append(new Store(scaledDivisor->getAddress()))
                            .append(new Load(multiple->getAddress()))
                            .append(new Shift(constants->getConstant(-1)->getAddress()))
                            .append(new Store(multiple->getAddress()));

                    InstructionList &loadResultBlock = *new InstructionList();
                    if (modulo) {
                        loadResultBlock.append(new Load(remain->getAddress()))
                                .append(new Jzero(instructionList.end()))
                                .append(new Load(sign->getAddress()));

                        InstructionList &negativeSignBlock = *new InstructionList();
                        negativeSignBlock.append(new Load(divisor->getAddress()))
                                .append(new Jzero(instructionList.end()));

                        InstructionList &aNegative = *new InstructionList();
                        aNegative.append(new Sub(remain->getAddress()))
                                .append(new Jump(zeroResultBlock.end()));

                        InstructionList &bNegative = *new InstructionList();
                        bNegative.append(new Add(remain->getAddress()))
                                .append(new Jump(zeroResultBlock.end()));

                        negativeSignBlock.append(new Jpos(bNegative.end()))
                                .append(bNegative)
                                .append(aNegative);

                        InstructionList &positiveSignBlock = *new InstructionList();
                        positiveSignBlock.append(new Load(divisor->getAddress()))
                                .append(new Jzero(instructionList.end()));

                        InstructionList &bothNegativeBlock = *new InstructionList();
                        bothNegativeBlock.append(new Sub(primaryAccumulator))
                                .append(new Sub(remain->getAddress()))
                                .append(new Jump(zeroResultBlock.end()));

                        InstructionList &bothPositiveBlock = *new InstructionList();
                        bothPositiveBlock.append(new Load(remain->getAddress()))
                                .append(new Jump(zeroResultBlock.end()));

                        positiveSignBlock.append(new Jpos(bothPositiveBlock.start()))
                                .append(bothNegativeBlock)
                                .append(bothPositiveBlock);

                        loadResultBlock.append(new Jzero(negativeSignBlock.end()))
                                .append(negativeSignBlock)
                                .append(positiveSignBlock);
                    } else {
                        loadResultBlock.append(new Load(sign->getAddress()));

                        InstructionList &negativeResultBlock = *new InstructionList();
                        negativeResultBlock.append(new Load(remain->getAddress()));

                        InstructionList &remainBlock = *new InstructionList();
                        remainBlock.append(new Sub(primaryAccumulator))
                                .append(new Sub(expressionAccumulator))
                                .append(new Dec())
                                .append(new Jump(zeroResultBlock.end()));

                        negativeResultBlock.append(new Jzero(remainBlock.end()))
                                .append(remainBlock)
                                .append(new Sub(primaryAccumulator))
                                .append(new Sub(expressionAccumulator))
                                .append(new Jump(zeroResultBlock.end()));

                        loadResultBlock.append(new Jzero(negativeResultBlock.end()))
                                .append(negativeResultBlock)
                                .append(new Load(expressionAccumulator))
                                .append(new Jump(zeroResultBlock.end()));
                    }

                    afterIfBlock.append(new Jzero(loadResultBlock.start()));
                    doWhileBlock.append(new Jneg(afterIfBlock.start()));

                    InstructionList &ifBlock = *new InstructionList();
                    ifBlock.append(new Store(remain->getAddress()))
                            .append(new Load(expressionAccumulator))
                            .append(new Add(multiple->getAddress()))
                            .append(new Store(expressionAccumulator));

                    doWhileBlock.append(ifBlock)
                            .append(afterIfBlock)
                            .append(new Jump(doWhileBlock.start()));

                    InstructionList &initBlock = *new InstructionList();
                    initBlock.append(new Sub(primaryAccumulator))
                            .append(new Store(remain->getAddress()))
                            .append(new Store(sign->getAddress()))
                            .append(new Inc())
                            .append(new Store(multiple->getAddress()))
                            .append(lhsResolution->instructions)
                            .append(lhsLoad)
                            .append(new Jzero(zeroResultBlock.start()))
                            .append(new Store(sign->getAddress()));

                    InstructionList &negativeABlock = *new InstructionList();
                    if (lhsResolution->indirect) {
                        negativeABlock.append(new Store(expressionAccumulator))
                                .append(new Sub(expressionAccumulator))
                                .append(new Sub(expressionAccumulator));
                    } else {
                        negativeABlock.append(new Sub(lhsResolution->address))
                                .append(new Sub(lhsResolution->address));
                    }

                    initBlock.append(new Jpos(negativeABlock.end()))
                            .append(negativeABlock)
                            .append(new Store(dividend->getAddress()))
                            .append(new Store(remain->getAddress()))
                            .append(rhsResolution->instructions)
                            .append(rhsLoad)
                            .append(new Jzero(zeroResultBlock.start()))
                            .append(new Store(divisor->getAddress())); // store divisor in original form

                    InstructionList &negativeBBlock = *new InstructionList();
                    if (rhsResolution->indirect) {
                        negativeBBlock.append(new Store(expressionAccumulator))
                                .append(new Sub(expressionAccumulator))
                                .append(new Sub(expressionAccumulator));
                    } else {
                        negativeBBlock.append(new Sub(rhsResolution->address))
                                .append(new Sub(rhsResolution->address));
                    }

                    InstructionList &eraseSign = *new InstructionList();
                    eraseSign.append(new Sub(primaryAccumulator))
                            .append(new Store(sign->getAddress()))
                            .append(new Sub(primaryAccumulator))
                            .append(new Store(expressionAccumulator))
                            .append(new Jump(firstWhileBlock.start()));

                    negativeBBlock.append(new Store(scaledDivisor->getAddress()))
                            .append(new Load(sign->getAddress()))
                            .append(new Jpos(eraseSign.end()))
                            .append(eraseSign)
                            .append(new Inc())
                            .append(new Store(sign->getAddress()))
                            .append(new Sub(primaryAccumulator))
                            .append(new Store(expressionAccumulator))
                            .append(new Jump(firstWhileBlock.start()));

                    initBlock.append(new Jpos(negativeBBlock.end()))
                            .append(negativeBBlock)
                            .append(new Store(scaledDivisor->getAddress()))
                            .append(new Sub(primaryAccumulator))
                            .append(new Store(expressionAccumulator))
                            .append(new Load(sign->getAddress()));

                    InstructionList &toggleSignForPositiveBlock = *new InstructionList();
                    toggleSignForPositiveBlock.append(new Sub(primaryAccumulator))
                            .append(new Store(sign->getAddress()))
                            .append(new Jump(firstWhileBlock.start()));

                    initBlock.append(new Jneg(toggleSignForPositiveBlock.end()))
                            .append(toggleSignForPositiveBlock)
                            .append(new Jump(firstWhileBlock.start()));

                    instructionList.append(initBlock)
                            .append(firstWhileBlock)
                            .append(doWhileBlock)
                            .append(loadResultBlock)
                            .append(zeroResultBlock);
                }
                    break;
                case MULTIPLICATION: { // A * B
                    bool incResolved = false;
                    if (rhsResolution->type == CONSTANT || lhsResolution->type == CONSTANT) { // know constants optimization
                        bool rhsFlag = rhsResolution->type == CONSTANT;

                        NumberValue &constantValue = dynamic_cast<NumberValue &>(rhsFlag ? binaryExpression.rhs : binaryExpression.lhs);
                        bool negative = constantValue.value < 0;
                        long long valCopy = llabs(constantValue.value);

                        if (valCopy && (valCopy & (valCopy - 1)) == 0) { // 2^n
                            incResolved = true;

                            long long power = 0;
                            while (valCopy = valCopy >> 1) power++;

                            constants->addConstant(power);

                            instructionList.append(rhsFlag ? lhsResolution->instructions : rhsResolution->instructions)
                                    .append(rhsFlag ? lhsLoad : rhsLoad)
                                    .append(new Shift(constants->getConstant(power)->getAddress()));

                            if (negative) {
                                instructionList.append(new Store(expressionAccumulator))
                                        .append(new Sub(expressionAccumulator))
                                        .append(new Sub(expressionAccumulator));
                            }
                        } else if (valCopy == 1) { // 1, -1
                            instructionList.append(rhsFlag ? lhsResolution->instructions : rhsResolution->instructions)
                                    .append(rhsFlag ? lhsLoad : rhsLoad);

                            if (negative) {
                                instructionList.append(new Store(expressionAccumulator))
                                        .append(new Sub(expressionAccumulator))
                                        .append(new Sub(expressionAccumulator));
                            }
                        } else if (valCopy == 0) { // just 0 lol
                            instructionList.append(new Sub(primaryAccumulator));
                        }
                    }

                    if (incResolved) break;

                    TemporaryVariable *temporaryA = new TemporaryVariable(TEMPORARY_NAMES, *new ResolvableAddress());
                    TemporaryVariable *temporaryB = new TemporaryVariable(TEMPORARY_NAMES, *new ResolvableAddress());
                    scopedVariables->pushVariableScope(temporaryA);
                    scopedVariables->pushVariableScope(temporaryB);

                    TemporaryVariable *copiedA = new TemporaryVariable(TEMPORARY_NAMES, *new ResolvableAddress());
                    TemporaryVariable *copiedB = new TemporaryVariable(TEMPORARY_NAMES, *new ResolvableAddress());
                    scopedVariables->pushVariableScope(copiedA);
                    scopedVariables->pushVariableScope(copiedB);
                    tempVars += 4;

                    Resolution *aResolution = resolve(binaryExpression.lhs);
                    Resolution *bResolution = resolve(binaryExpression.rhs);

                    InstructionList &loadResultBlock = *new InstructionList();
                    loadResultBlock.append(new Load(expressionAccumulator));

                    InstructionList &whileBlock = *new InstructionList();
                    whileBlock.append(new Load(copiedA->getAddress()))
                            .append(new Jzero(whileBlock.end())) // while (a != 0)
                            .append(new Shift(constants->getConstant(-1)->getAddress()))
                            .append(new Shift(constants->getConstant(1)->getAddress()))
                            .append(new Sub(copiedA->getAddress())); // bit check - a & 1

                    InstructionList &afterIfBlock = *new InstructionList();
                    afterIfBlock.append(new Load(copiedA->getAddress()))
                            .append(new Shift(constants->getConstant(-1)->getAddress()))
                            .append(new Store(copiedA->getAddress())) // a = a << 1
                            .append(new Load(copiedB->getAddress()))
                            .append(new Shift(constants->getConstant(1)->getAddress()))
                            .append(new Store(copiedB->getAddress())) // b = b >> 1
                            .append(new Jump(whileBlock.start())); // continue while

                    whileBlock.append(new Jzero(afterIfBlock.start())); // if bit check failed, skip if

                    InstructionList &ifBlock = *new InstructionList();
                    ifBlock.append(new Load(expressionAccumulator))
                            .append(new Add(copiedB->getAddress()))
                            .append(new Store(expressionAccumulator)); // inside bit bit check if, result = result + b

                    whileBlock.append(ifBlock)
                            .append(afterIfBlock);

                    InstructionList &initBlock = *new InstructionList();
                    initBlock.append(new Sub(primaryAccumulator))
                            .append(new Store(expressionAccumulator)) // reset result
                            .append(aResolution->instructions)
                            .append(aResolution->indirect ? static_cast<Instruction * >(new Loadi(primaryAccumulator)) : static_cast<Instruction *>(new Load(aResolution->address)))
                            .append(new Jzero(loadResultBlock.start())); // skip algorithm, if a was zero

                    InstructionList &aLessThanZeroBlock = *new InstructionList();
                    aLessThanZeroBlock.append(new Store(temporaryA->getAddress()))
                            .append(bResolution->instructions)
                            .append(bResolution->indirect ? static_cast<Instruction *>(new Loadi(primaryAccumulator)) : static_cast<Instruction *>(new Load(bResolution->address)))
                            .append(new Jzero(loadResultBlock.start())) // skip algorithm, if b was zero
                            .append(new Store(temporaryB->getAddress())); // save it for now for swaps

                    InstructionList &aAndBLessThanZeroBlock = *new InstructionList();
                    aAndBLessThanZeroBlock.append(new Sub(temporaryA->getAddress())); // a, b < 0, so b - a > 0 ==> abs(a) > abs(b), swap them!

                    InstructionList &aAndBNegativeSwap = *new InstructionList();
                    aAndBNegativeSwap.append(new Sub(primaryAccumulator))
                            .append(new Sub(temporaryA->getAddress()))
                            .append(new Store(copiedB->getAddress())) // b = -a
                            .append(new Sub(primaryAccumulator))
                            .append(new Sub(temporaryB->getAddress()))
                            .append(new Store(copiedA->getAddress())) // a = -b
                            .append(new Jump(whileBlock.start()));

                    aAndBLessThanZeroBlock.append(new Jneg(aAndBNegativeSwap.end()))
                            .append(aAndBNegativeSwap)
                            .append(new Sub(primaryAccumulator))
                            .append(new Sub(temporaryA->getAddress()))
                            .append(new Store(copiedA->getAddress())) // a = -a
                            .append(new Sub(primaryAccumulator))
                            .append(new Sub(temporaryB->getAddress()))
                            .append(new Store(copiedB->getAddress())) // b = -b
                            .append(new Jump(whileBlock.start()));

                    InstructionList &aNegativeBPositiveSwap = *new InstructionList();
                    aNegativeBPositiveSwap.append(new Load(temporaryA->getAddress()))
                            .append(new Store(copiedB->getAddress())) // a = b
                            .append(new Load(temporaryB->getAddress()))
                            .append(new Store(copiedA->getAddress())) // b = a
                            .append(new Jump(whileBlock.start()));

                    aLessThanZeroBlock.append(new Jneg(aAndBLessThanZeroBlock.start()))
                            .append(new Add(temporaryA->getAddress()))
                            .append(new Jpos(aNegativeBPositiveSwap.end()))
                            .append(aNegativeBPositiveSwap)
                            .append(new Sub(primaryAccumulator))
                            .append(new Sub(temporaryA->getAddress()))
                            .append(new Store(copiedA->getAddress())) // a = -a
                            .append(new Sub(primaryAccumulator))
                            .append(new Sub(temporaryB->getAddress()))
                            .append(new Store(copiedB->getAddress())) // b = -b
                            .append(new Jump(whileBlock.start()))
                            .append(aAndBLessThanZeroBlock);

                    Resolution *newBResolution = resolve(binaryExpression.rhs);
                    initBlock.append(new Jneg(aLessThanZeroBlock.start())) // go to negative a block
                            .append(new Store(copiedA->getAddress())) // or just store a
                            .append(newBResolution->instructions)
                            .append(newBResolution->indirect ? static_cast<Instruction *>(new Loadi(primaryAccumulator)) : static_cast<Instruction *>(new Load(newBResolution->address)))
                            .append(new Jzero(loadResultBlock.start())) // skip algorithm, if b = 0
                            .append(new Store(temporaryB->getAddress())); // store in temporary for now, because when a > b, b * a is faster than a * b

                    InstructionList &storeBSwapEnd = *new InstructionList();
                    storeBSwapEnd.append(new Load(temporaryB->getAddress()))
                            .append(new Store(copiedB->getAddress()))
                            .append(new Jump(whileBlock.start()));

                    InstructionList &positiveANegativeBSwapBlock = *new InstructionList();
                    positiveANegativeBSwapBlock.append(new Add(copiedA->getAddress()))
                            .append(new Jneg(storeBSwapEnd.start())) // just store b, it's abs bigger than a
                            .append(new Sub(primaryAccumulator))
                            .append(new Sub(copiedA->getAddress()))
                            .append(new Store(copiedB->getAddress())) // a = -b
                            .append(new Sub(primaryAccumulator))
                            .append(new Sub(temporaryB->getAddress()))
                            .append(new Store(copiedA->getAddress())) // b = -a
                            .append(new Jump(whileBlock.start()));

                    InstructionList &positiveASwapBlock = *new InstructionList();
                    positiveASwapBlock.append(new Jneg(positiveANegativeBSwapBlock.start()))
                            .append(new Sub(copiedA->getAddress()))
                            .append(new Jpos(storeBSwapEnd.start())) // just store b, it's abs bigger than a
                            .append(new Load(copiedA->getAddress()))
                            .append(new Store(copiedB->getAddress())) // a = b
                            .append(new Load(temporaryB->getAddress()))
                            .append(new Store(copiedA->getAddress())) // b = a
                            .append(new Jump(whileBlock.start()))
                            .append(positiveANegativeBSwapBlock)
                            .append(storeBSwapEnd);

                    initBlock.append(positiveASwapBlock)
                            .append(aLessThanZeroBlock);

                    instructionList.append(initBlock)
                            .append(whileBlock)
                            .append(loadResultBlock);
                }
                    break;
            }

            return new SimpleResolution(
                    instructionList,
                    tempVars + lhsResolution->temporaryVars + rhsResolution->temporaryVars
            );
        } catch (std::bad_cast
                 ee) {}
    }
}

/***
 * Resolves addresses of variables
 * @param identifier
 * @return
 */
Resolution *AbstractAssembler::resolve(AbstractIdentifier &identifier, bool checkInit = true) {
    Variable *var = scopedVariables->resolveVariable(identifier.name);
    if (checkInit && !var->initialized) {
        std::cout << "   [w] Variable " << var->name << " may not have been initialized" << std::endl;
    }
    var->initialized = true; // assume it was initialized at this point

    if (auto numVar = dynamic_cast<NumberVariable *>(var)) {
        try {
            VariableIdentifier &varId = dynamic_cast<VariableIdentifier &>(identifier); // check if the identifier was actually a var id

            return new Resolution(
                    *new InstructionList(),
                    numVar->getAddress(),
                    VARIABLE,
                    false,
                    0, // zero temp vars used
                    !numVar->readOnly // if it's readOnly, it's not writable -- used for iterators
            );
        } catch (std::bad_cast _) {
            throw "Trying to access a number variable like an array";
        }
    } else if (auto arrayVar = dynamic_cast<NumberArrayVariable *>(var)) {
        try { // ACCESS VALUE - a[0]
            AccessIdentifier &accId = dynamic_cast<AccessIdentifier &>(identifier);

            if ((accId.index < arrayVar->start || accId.index > arrayVar->end) && !arrayVar->warned) {
                std::cout << "   [w] Trying to access " + arrayVar->toString() + " at index " + std::to_string(accId.index) << "; you won't be warned about this array anymore" << std::endl;
                arrayVar->warned = true;
            }

            ResolvableAddress &address = *new ResolvableAddress(arrayVar->getAddress().getAddress()); // copy address
            address.setOffset(accId.index - arrayVar->start); // set proper offset

            return new Resolution(
                    *new InstructionList(),
                    address,
                    CONSTANT_ARRAY,
                    false
            );
        } catch (std::bad_cast _) {
            try { // VARIABLE ACCESS VALUE - a[b]
                VariableAccessIdentifier &varAccId = dynamic_cast<VariableAccessIdentifier &>(identifier);

                ResolvableAddress &startValueAddress = constants->getConstant(arrayVar->start)->getAddress(); // arr start
                Variable *variable = scopedVariables->resolveVariable(varAccId.accessName); // "b" variable
                if (!variable->initialized) {
                    std::cout << "   [w] Variable " << variable->name << " may not have been initialized" << std::endl;
                }
                variable->initialized = true; // assume it was initialized at this point

                ResolvableAddress &arrAddressAddress = constants->getConstant(arrayVar->getAddress().getAddress())->getAddress();

                InstructionList &instructionList = *new InstructionList();

                Load *load = new Load(variable->getAddress()); // load value of b
                Sub *sub = new Sub(startValueAddress); // subtract value of starting index
                Add *add = new Add(arrAddressAddress); // add value of array address

                instructionList.append(load)
                        .append(sub)
                        .append(add);

                return new Resolution(
                        instructionList,
                        *new ResolvableAddress(),
                        VARIABLE_ARRAY,
                        true
                );
            } catch (std::bad_cast __) {
                throw "Trying to use array identifier as variable";
            }
        }
    } else throw "Variable resolution error";
}

/**
 * Resolve address of a value
 */
Resolution *AbstractAssembler::resolve(AbstractValue &value) {
    try { // NUMBER VALUE - 0
        NumberValue &numberValue = dynamic_cast<NumberValue &>(value);
        ResolvableAddress &address = constants->getConstant(numberValue.value)->getAddress();
        return new Resolution(
                *new InstructionList(),
                address,
                CONSTANT,
                false,
                0
        );
    } catch (std::bad_cast _) {
        try {
            IdentifierValue &identifierValue = dynamic_cast<IdentifierValue &>(value);
            return resolve(identifierValue.identifier);
        } catch (std::bad_cast __) {
            throw "Value resolution error";
        }
    }
}

InstructionList &AbstractAssembler::assemble(bool verbose) {
    getVariablesFromDeclarations(verbose);
    prepareConstants(verbose);
    SimpleResolution *programCodeResolution = assembleCommands(program.commands);
    InstructionList &instructions = assembleConstants();
    instructions.append(programCodeResolution->instructions);
    instructions.seal(true);
    return instructions;
}
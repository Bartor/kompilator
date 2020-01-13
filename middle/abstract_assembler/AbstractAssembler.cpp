#include "AbstractAssembler.h"

std::string TEMPORARY_NAMES = "!TEMP";

InstructionList &AbstractAssembler::assembleConstants() {
    constants = new Constants(accumulatorNumber);
    InstructionList &list = *new InstructionList();

    std::cout << "Adding 1 and -1 constants" << std::endl;
    Constant *resOne = constants->addConstant(1);
    Constant *resMinusOne = constants->addConstant(-1);

    list.append(resOne->generateConstant(constants, primaryAccumulator, secondaryAccumulator));
    list.append(resMinusOne->generateConstant(constants, primaryAccumulator, secondaryAccumulator));

    for (const auto num : program.constants.constants) {
        Constant *constantAddress = constants->addConstant(num);

        if (constantAddress) {
            std::cout << constantAddress->toString() + " at " << constantAddress->getAddress().getAddress() << std::endl;

            list.append(constantAddress->generateConstant(constants, primaryAccumulator, secondaryAccumulator));
        }
    }

    return list;
}

void AbstractAssembler::getVariablesFromDeclarations() {
    scopedVariables = new ScopedVariables(
            1
            + accumulatorNumber // offset by accumulators...
            + program.constants.constants.size() // ...constants...
            + program.declarations.declarations.size() // ...and possible constants
    );

    for (const auto &declaration : program.declarations.declarations) {
        if (auto numDecl = dynamic_cast<IdentifierDeclaration *>(declaration)) {
            NumberVariable *var = new NumberVariable(numDecl->name, *new ResolvableAddress());
            scopedVariables->pushVariableScope(var);
            std::cout << var->toString() << " at " << var->getAddress().getAddress() << std::endl;
        } else if (auto arrDecl = dynamic_cast<ArrayDeclaration *>(declaration)) {
            NumberArrayVariable *var = new NumberArrayVariable(arrDecl->name, *new ResolvableAddress(), arrDecl->start, arrDecl->end);
            scopedVariables->pushVariableScope(var);
            std::cout << var->toString() << " at " << var->getAddress().getAddress() << std::endl;

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

        if (auto readNode = dynamic_cast<Read *>(command)) { // READ
            Resolution *idRes = resolve(readNode->identifier);

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
            Resolution *idRes = resolve(assignNode->identifier);

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
                        loadResultBlock.append(new Load(sign->getAddress()));

                        InstructionList &negativeSignBlock = *new InstructionList();
                        negativeSignBlock.append(new Load(divisor->getAddress()));

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
                        positiveSignBlock.append(new Load(divisor->getAddress()));

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
                    TemporaryVariable *negativeATemp = new TemporaryVariable(TEMPORARY_NAMES, *new ResolvableAddress());
                    TemporaryVariable *negativeBTemp = new TemporaryVariable(TEMPORARY_NAMES, *new ResolvableAddress());
                    scopedVariables->pushVariableScope(negativeATemp);
                    scopedVariables->pushVariableScope(negativeBTemp);

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
                            .append(new Jzero(whileBlock.end()))
                            .append(new Shift(constants->getConstant(-1)->getAddress()))
                            .append(new Shift(constants->getConstant(1)->getAddress()))
                            .append(new Sub(copiedA->getAddress()));

                    InstructionList &afterIfBlock = *new InstructionList();
                    afterIfBlock.append(new Load(copiedA->getAddress()))
                            .append(new Shift(constants->getConstant(-1)->getAddress()))
                            .append(new Store(copiedA->getAddress()))
                            .append(new Load(copiedB->getAddress()))
                            .append(new Shift(constants->getConstant(1)->getAddress()))
                            .append(new Store(copiedB->getAddress()))
                            .append(new Jump(whileBlock.start()));

                    whileBlock.append(new Jzero(afterIfBlock.start()));

                    InstructionList &ifBlock = *new InstructionList();
                    ifBlock.append(new Load(expressionAccumulator))
                            .append(new Add(copiedB->getAddress()))
                            .append(new Store(expressionAccumulator));

                    whileBlock.append(ifBlock)
                            .append(afterIfBlock);

                    InstructionList &initBlock = *new InstructionList();
                    initBlock.append(new Sub(primaryAccumulator))
                            .append(new Store(expressionAccumulator))
                            .append(aResolution->instructions)
                            .append(aResolution->indirect ? static_cast<Instruction * >(new Loadi(primaryAccumulator)) : static_cast<Instruction *>(new Load(aResolution->address)))
                            .append(new Jzero(loadResultBlock.start()));

                    InstructionList &aLessThanZeroBlock = *new InstructionList();
                    aLessThanZeroBlock.append(new Store(negativeATemp->getAddress()))
                            .append(bResolution->instructions)
                            .append(bResolution->indirect ? static_cast<Instruction *>(new Loadi(primaryAccumulator)) : static_cast<Instruction *>(new Load(bResolution->address)))
                            .append(new Jzero(loadResultBlock.start()));

                    InstructionList &aAndBLessThanZeroBlock = *new InstructionList();
                    if (bResolution->indirect) {
                        aAndBLessThanZeroBlock.append(new Store(negativeBTemp->getAddress()))
                                .append(new Sub(negativeBTemp->getAddress()))
                                .append(new Sub(negativeBTemp->getAddress()))
                                .append(new Store(copiedB->getAddress()));
                    } else {
                        aAndBLessThanZeroBlock.append(new Sub(bResolution->address))
                                .append(new Sub(bResolution->address))
                                .append(new Store(copiedB->getAddress()));
                    }
                    aAndBLessThanZeroBlock.append(new Sub(primaryAccumulator))
                            .append(new Sub(negativeATemp->getAddress()))
                            .append(new Store(copiedA->getAddress()))
                            .append(new Jump(whileBlock.start()));

                    aLessThanZeroBlock.append(new Jneg(aAndBLessThanZeroBlock.start()))
                            .append(new Store(copiedA->getAddress()))
                            .append(new Load(negativeATemp->getAddress()))
                            .append(new Store(copiedB->getAddress()))
                            .append(new Jump(whileBlock.start()));

                    Resolution *newBResolution = resolve(binaryExpression.rhs);
                    initBlock.append(new Jneg(aLessThanZeroBlock.start()))
                            .append(new Store(copiedA->getAddress()))
                            .append(newBResolution->instructions)
                            .append(newBResolution->indirect ? static_cast<Instruction *>(new Loadi(primaryAccumulator)) : static_cast<Instruction *>(new Load(newBResolution->address)))
                            .append(new Jzero(loadResultBlock.start()))
                            .append(new Store(copiedB->getAddress()))
                            .append(new Jump(whileBlock.start()))
                            .append(aLessThanZeroBlock)
                            .append(aAndBLessThanZeroBlock);

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
        } catch (std::bad_cast ee) {}
    }
}

/***
 * Resolves addresses of variables
 * @param identifier
 * @return
 */
Resolution *AbstractAssembler::resolve(AbstractIdentifier &identifier) {
    Variable *var = scopedVariables->resolveVariable(identifier.name);

    if (auto numVar = dynamic_cast<NumberVariable *>(var)) {
        return new Resolution(
                *new InstructionList(),
                numVar->getAddress(),
                VARIABLE,
                false,
                0, // zero temp vars used
                !numVar->readOnly // if it's readOnly, it's not writable -- used for iterators
        );
    } else if (auto arrayVar = dynamic_cast<NumberArrayVariable *>(var)) {
        try { // ACCESS VALUE - a[0]
            AccessIdentifier &accId = dynamic_cast<AccessIdentifier &>(identifier);

            if (accId.index < arrayVar->start || accId.index > arrayVar->end) {
                throw "Trying to access " + arrayVar->toString() + " at index " + std::to_string(accId.index);
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
            } catch (std::bad_cast _) {}
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
        } catch (std::bad_cast _) {
            throw "Value resolution error";
        }
    }
}

InstructionList &AbstractAssembler::assemble() {
    getVariablesFromDeclarations();
    InstructionList &instructions = assembleConstants();
    SimpleResolution *programCodeResolution = assembleCommands(program.commands);
    std::cout << "Assembled programs returning " << programCodeResolution->temporaryVars << " temporary variables " << std::endl;
    instructions.append(programCodeResolution->instructions);
    instructions.seal();
    return instructions;
}
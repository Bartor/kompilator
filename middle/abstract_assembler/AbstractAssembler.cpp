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
            accumulatorNumber // offset by accumulators...
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

            Instruction *store = idRes->indirect ? static_cast<Instruction *>(new Storei(idRes->address)) : static_cast<Instruction *>(new Store(idRes->address));
            Get *get = new Get();

            instructions.append(idRes->instructions)
                    .append(get)
                    .append(store);

            tempVars += idRes->temporaryVars;
        } else if (auto writeNode = dynamic_cast<Write *>(command)) { // WRITE
            Resolution *valRes = resolve(writeNode->value);
            Instruction *load = valRes->indirect ? static_cast<Instruction *>(new Loadi(valRes->address)) : static_cast<Instruction *>(new Load(valRes->address));
            Put *put = new Put();

            instructions.append(valRes->instructions)
                    .append(load)
                    .append(put);

            tempVars += valRes->temporaryVars;
        } else if (auto assignNode = dynamic_cast<Assignment *>(command)) { // ASSIGN
            Resolution *idRes = resolve(assignNode->identifier);

            if (!idRes->writable) throw "Trying to assign to non-writable variable " + assignNode->identifier.name;

            Resolution *expRes = assembleExpression(assignNode->expression);

            Instruction *store = idRes->indirect ? static_cast<Instruction *>(new Storei(idRes->address)) : static_cast<Instruction *>(new Store(idRes->address));
            Instruction *load = expRes->indirect ? static_cast<Instruction *>(new Loadi(expRes->address)) : static_cast<Instruction *>(new Load(expRes->address));

            instructions.append(idRes->instructions)
                    .append(expRes->instructions)
                    .append(load)
                    .append(store);

            tempVars += idRes->temporaryVars;
            tempVars += expRes->temporaryVars;
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

            TemporaryVariable *iterationStart = new TemporaryVariable(TEMPORARY_NAMES, *new ResolvableAddress());
            TemporaryVariable *iterationEnd = new TemporaryVariable(TEMPORARY_NAMES, *new ResolvableAddress());

            scopedVariables->pushVariableScope(iterationEnd);
            scopedVariables->pushVariableScope(iterationStart);

            Resolution *startRes = resolve(forNode->startValue);
            Resolution *endRes = resolve(forNode->endValue);

            Instruction *startLoad = startRes->indirect ? static_cast<Instruction *>(new Loadi(startRes->address)) : static_cast<Instruction *>(new Load(startRes->address));
            Instruction *endLoad = endRes->indirect ? static_cast<Instruction *>(new Loadi(endRes->address)) : static_cast<Instruction *>(new Load(endRes->address));

            Store *startStore = new Store(iterationStart->getAddress());
            Store *endStore = new Store(iterationEnd->getAddress());

            instructions.append(endLoad)
                    .append(endStore)
                    .append(startLoad)
                    .append(startStore);

            SimpleResolution *codeResolution = assembleCommands(forNode->commands); // assemble iterated commands

            Store *storeInI = new Store(iterator->getAddress());
            Sub *subStartEnd = new Sub(iterationEnd->getAddress());
            Instruction *j___ = forNode->reversed ? static_cast<Instruction *>(new Jneg(codeResolution->instructions.end())) : static_cast<Instruction *>(new Jpos(codeResolution->instructions.end()));

            Jump *jumpToSSJ = new Jump(storeInI);

            Load *loadI = new Load(iterator->getAddress());
            Instruction *incDec = forNode->reversed ? static_cast<Instruction *>(new Dec()) : static_cast<Instruction *>(new Inc());

            Jump *jumpToLoadIncSSJ = new Jump(loadI);
            codeResolution->instructions.append(jumpToLoadIncSSJ);

            instructions.append(jumpToSSJ)
                    .append(loadI)
                    .append(incDec)
                    .append(storeInI)
                    .append(subStartEnd)
                    .append(j___)
                    .append(codeResolution->instructions);

            tempVars += 3 + startRes->temporaryVars + endRes->temporaryVars; //  iterator + iterationStart + iterationEnd
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

    Instruction *lhsLoad = lhsResolution->indirect ? static_cast<Instruction *>(new Loadi(lhsResolution->address)) : static_cast<Instruction *>(new Load(lhsResolution->address));
    Instruction *rhsLoad = rhsResolution->indirect ? static_cast<Instruction *>(new Loadi(rhsResolution->address)) : static_cast<Instruction *>(new Load(rhsResolution->address));
    Store *store = new Store(expressionAccumulator);

    if (rhsResolution->indirect) {
        Sub *sub = new Sub(expressionAccumulator);
        instructions.append(rhsResolution->instructions)
                .append(rhsLoad)
                .append(store)
                .append(lhsResolution->instructions)
                .append(lhsLoad);
    } else {
        Sub *sub = new Sub(rhsResolution->address);
        instructions.append(lhsResolution->instructions)
                .append(lhsLoad)
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

Resolution *AbstractAssembler::assembleExpression(AbstractExpression &expression) {
    try {
        UnaryExpression &unaryExpression = dynamic_cast<UnaryExpression &>(expression);
        return resolve(unaryExpression.value);
    } catch (std::bad_cast e) {
        try {
            TemporaryVariable *temp = new TemporaryVariable(TEMPORARY_NAMES, *new ResolvableAddress());
            scopedVariables->pushVariableScope(temp);

            long long tempVars = 1;

            BinaryExpression &binaryExpression = dynamic_cast<BinaryExpression &>(expression);
            InstructionList &instructionList = *new InstructionList();

            Resolution *lhsResolution = resolve(binaryExpression.lhs);
            Resolution *rhsResolution = resolve(binaryExpression.rhs);

            Instruction *lhsLoad = lhsResolution->indirect ? static_cast<Instruction *>(new Loadi(lhsResolution->address)) : static_cast<Instruction *>(new Load(lhsResolution->address));
            Instruction *rhsLoad = rhsResolution->indirect ? static_cast<Instruction *>(new Loadi(rhsResolution->address)) : static_cast<Instruction *>(new Load(rhsResolution->address));
            Store *store = new Store(expressionAccumulator);

            bool modulo = false;
            switch (binaryExpression.type) {
                case ADDITION: {
                    if (rhsResolution->indirect) {
                        Add *add = new Add(expressionAccumulator);

                        instructionList.append(lhsResolution->instructions)
                                .append(lhsLoad)
                                .append(store)
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

                        instructionList.append(rhsResolution->instructions)
                                .append(rhsLoad)
                                .append(store)
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
                    // scaled divisor is to be kept in secondaryAccumulator
                    // dividend is to be kept in expressionAccumulator
                    TemporaryVariable *multiplier = temp; // result is store in temp

                    TemporaryVariable *remain = new TemporaryVariable(TEMPORARY_NAMES, *new ResolvableAddress());
                    TemporaryVariable *result = new TemporaryVariable(TEMPORARY_NAMES, *new ResolvableAddress());
                    scopedVariables->pushVariableScope(remain);
                    scopedVariables->pushVariableScope(result);

                    tempVars += 2; // as mult is kept in default temp for expression, we don't need to push the scope

                    Sub *sub0 = new Sub(primaryAccumulator);
                    Store *storeResult = new Store(result->getAddress());
                    Inc *inc = new Inc();
                    Store *storeMult = new Store(multiplier->getAddress());
                    // load A
                    Store *storeDividend = new Store(expressionAccumulator);
                    Store *storeRemain = new Store(remain->getAddress());
                    // load B
                    // jzero to SUB_END
                    Store *storeSD = new Store(secondaryAccumulator);
                    // jump to WHILE_WITHOUT_LOAD

                    Load *loadSD = new Load(secondaryAccumulator); // WHILE
                    Sub *subDividend = new Sub(expressionAccumulator); // WHILE_WITHOUT_LOAD
                    // jzero to IF
                    // Jpos to IF

                    Add *addDividend = new Add(expressionAccumulator);
                    Shift *shiftByPlusOne = new Shift(constants->getConstant(1)->getAddress());
                    Store *storeSD2 = new Store(secondaryAccumulator);
                    Load *loadMult = new Load(multiplier->getAddress());
                    // shift by +1
                    Store *storeMult2 = new Store(multiplier->getAddress());
                    Jump *jumpToWhile = new Jump(loadSD); // JUMP TO WHILE

                    Load *loadRemain = new Load(remain->getAddress()); // IF
                    Sub *subSD = new Sub(secondaryAccumulator);
                    // jneg to IF_END
                    Store *storeRemain2 = new Store(remain->getAddress());
                    Load *loadResult = new Load(result->getAddress());
                    Add *addMult = new Add(multiplier->getAddress());
                    Store *storeResult2 = new Store(result->getAddress());

                    Load *loadSD2 = new Load(secondaryAccumulator); // IF_END
                    Shift *shiftByMinusOne = new Shift(constants->getConstant(-1)->getAddress());
                    Store *storeSD3 = new Store(secondaryAccumulator);
                    Load *loadMult2 = new Load(multiplier->getAddress());
                    // shift by -1
                    // jzero END
                    Store *storeMult3 = new Store(multiplier->getAddress());
                    Jump *jumpToIf = new Jump(loadRemain); // jump to IF

                    Instruction *loadFinalResult = modulo ? static_cast<Instruction *>(new Load(remain->getAddress())) : static_cast<Instruction *>(new Load(result->getAddress()));
                    // jump past sub
                    Sub *sub0End = new Sub(primaryAccumulator);
                    Stub *stubPastSub = new Stub();

                    //missing jumps
                    Jzero *jzeroToSubEnd = new Jzero(sub0End);
                    Jump *jumpToWhileWithoutLoad = new Jump(subDividend);
                    Jzero *jzeroToIf = new Jzero(loadRemain);
                    Jpos *jposToIf = new Jpos(loadRemain);
                    Jneg *jnegToIfEnd = new Jneg(loadSD2);
                    Jzero *jzeroToEnd = new Jzero(loadFinalResult);
                    Jump *jumpPastSub = new Jump(stubPastSub);

                    instructionList.append(lhsResolution->instructions)
                            .append(lhsLoad)
                            .append(storeDividend)
                            .append(storeRemain)
                            .append(sub0)
                            .append(storeResult)
                            .append(inc)
                            .append(storeMult)
                            .append(rhsResolution->instructions)
                            .append(rhsLoad)
                            .append(jzeroToSubEnd)
                            .append(storeSD)
                            .append(jumpToWhileWithoutLoad)
                            .append(loadSD)
                            .append(subDividend)
                            .append(jzeroToIf)
                            .append(jposToIf)
                            .append(addDividend)
                            .append(shiftByPlusOne)
                            .append(storeSD2)
                            .append(loadMult)
                            .append(shiftByPlusOne)
                            .append(storeMult2)
                            .append(jumpToWhile)
                            .append(loadRemain)
                            .append(subSD)
                            .append(jnegToIfEnd)
                            .append(storeRemain2)
                            .append(loadResult)
                            .append(addMult)
                            .append(storeResult2)
                            .append(loadSD2)
                            .append(shiftByMinusOne)
                            .append(storeSD3)
                            .append(loadMult2)
                            .append(shiftByMinusOne)
                            .append(jzeroToEnd)
                            .append(storeMult3)
                            .append(jumpToIf)
                            .append(loadFinalResult)
                            .append(jumpPastSub)
                            .append(sub0End)
                            .append(stubPastSub);
                }
                    break;
                case MULTIPLICATION: { // A * B
                    // A and B are modified and must be copies before...
                    ResolvableAddress &aAddr = temp->getAddress();
                    ResolvableAddress &bAddr = expressionAccumulator;
                    Store *aStore = new Store(temp->getAddress()); // ... a into temporary

                    instructionList.append(lhsResolution->instructions)
                            .append(lhsLoad)
                            .append(aStore);
                    instructionList.append(rhsResolution->instructions)
                            .append(rhsLoad)
                            .append(store); // ... b into eacc

                    Sub *accumulatorReset = new Sub(primaryAccumulator);
                    Store *storeStartResult = new Store(secondaryAccumulator); // result is in secondary accumulator
                    Load *loadA = new Load(aAddr);
                    // JZERO [LOAD result]1
                    Shift *shiftMinusOne = new Shift(constants->getConstant(-1)->getAddress());
                    Shift *shiftPlusOne = new Shift(constants->getConstant(1)->getAddress());
                    Sub *subA = new Sub(aAddr);
                    // JZERO [LOAD A]2
                    Load *loadResult = new Load(secondaryAccumulator);
                    Add *addB = new Add(bAddr);
                    Store *storeResult = new Store(secondaryAccumulator);
                    Load *loadA2 = new Load(aAddr);
                    // SHIFT -1
                    Store *storeA = new Store(aAddr);
                    Load *loadB = new Load(bAddr);
                    // SHIFT 1
                    Store *storeB = new Store(bAddr);
                    Jump *jumpToLoadA = new Jump(loadA);
                    Load *loadResult2 = new Load(secondaryAccumulator);

                    Jzero *jzeroToLoadLoadA2 = new Jzero(loadA2);
                    Jzero *jzeroToLoadResult2 = new Jzero(loadResult2);

                    instructionList.append(accumulatorReset)
                            .append(storeStartResult)
                            .append(loadA)
                            .append(jzeroToLoadResult2)
                            .append(shiftMinusOne)
                            .append(shiftPlusOne)
                            .append(subA)
                            .append(jzeroToLoadLoadA2)
                            .append(loadResult)
                            .append(addB)
                            .append(storeResult)
                            .append(loadA2)
                            .append(shiftMinusOne)
                            .append(storeA)
                            .append(loadB)
                            .append(shiftPlusOne)
                            .append(storeB)
                            .append(jumpToLoadA)
                            .append(loadResult2);
                }
                    break;
            }

            Store *resultStore = new Store(temp->getAddress());
            instructionList.append(resultStore);

            return new Resolution(
                    instructionList,
                    temp->getAddress(),
                    false,
                    tempVars + lhsResolution->temporaryVars + rhsResolution->temporaryVars
            );
        } catch (std::bad_cast ee) {}
    }
}

Resolution *AbstractAssembler::resolve(AbstractIdentifier &identifier) {
    Variable *var = scopedVariables->resolveVariable(identifier.name);

    if (auto numVar = dynamic_cast<NumberVariable *>(var)) {
        return new Resolution(
                *new InstructionList(),
                numVar->getAddress(),
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
                    false
            );
        } catch (std::bad_cast _) {
            try { // VARIABLE ACCESS VALUE - a[b]
                VariableAccessIdentifier &varAccId = dynamic_cast<VariableAccessIdentifier &>(identifier);

                TemporaryVariable *tempVar = new TemporaryVariable(TEMPORARY_NAMES, *new ResolvableAddress());
                scopedVariables->pushVariableScope(tempVar);

                ResolvableAddress &startValueAddress = constants->getConstant(arrayVar->start)->getAddress(); // arr start
                Variable *variable = scopedVariables->resolveVariable(varAccId.accessName); // "b" variable
                ResolvableAddress &arrAddressAddress = constants->getConstant(arrayVar->getAddress().getAddress())->getAddress();

                InstructionList &instructionList = *new InstructionList();

                Load *load = new Load(variable->getAddress()); // load value of b
                Sub *sub = new Sub(startValueAddress); // subtract value of starting index
                Add *add = new Add(arrAddressAddress); // add value of array address
                Store *store = new Store(tempVar->getAddress()); // store indirect address in new variable

                instructionList.append(load)
                        .append(sub)
                        .append(add)
                        .append(store);

                return new Resolution(
                        instructionList,
                        tempVar->getAddress(),
                        true,
                        1
                );
            } catch (std::bad_cast _) {}
        }
    } else throw "Variable resolution error";
}

Resolution *AbstractAssembler::resolve(AbstractValue &value) {
    try { // NUMBER VALUE - 0
        NumberValue &numberValue = dynamic_cast<NumberValue &>(value);
        ResolvableAddress &address = constants->getConstant(numberValue.value)->getAddress();
        return new Resolution(
                *new InstructionList(),
                address,
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
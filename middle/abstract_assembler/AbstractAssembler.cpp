#include "AbstractAssembler.h"

std::string TEMPORARY_NAMES = "!TEMP";

InstructionList &AbstractAssembler::assembleConstants() {
    constants = new Constants(accumulatorNumber);

    InstructionList &list = *new InstructionList();

    for (const auto num : program.constants.constants) {
        Constant *constantAddress = constants->addConstant(num);

        if (constantAddress) {
            std::cout << constantAddress->toString() + " at " << constantAddress->getAddress().getAddress() << std::endl;

            list.append(constantAddress->generateConstant());
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

InstructionList &AbstractAssembler::generateBoilerplate() {
    InstructionList &instructionList = *new InstructionList();

    // todo generate multiplication, modulo and division code
    return instructionList;
}

InstructionList &AbstractAssembler::assembleCommands(CommandList &commandList) {
    InstructionList &instructions = *new InstructionList();

    for (const auto &command : commandList.commands) {
        if (auto readNode = dynamic_cast<Read *>(command)) { // READ
            Resolution *idRes = resolve(readNode->identifier);

            if (!idRes->writable) throw "Trying to read to non-writable variable " + readNode->identifier.name;

            Instruction *store = idRes->indirect ? static_cast<Instruction *>(new Storei(idRes->address)) : static_cast<Instruction *>(new Store(idRes->address));
            Get *get = new Get();

            instructions.append(idRes->instructions)
                    .append(get)
                    .append(store);

            // scopedVariables->popVariableScope(idRes->temporaryVars);
        } else if (auto writeNode = dynamic_cast<Write *>(command)) { // WRITE
            Resolution *valRes = resolve(writeNode->value);
            Instruction *load = valRes->indirect ? static_cast<Instruction *>(new Loadi(valRes->address)) : static_cast<Instruction *>(new Load(valRes->address));
            Put *put = new Put();

            instructions.append(valRes->instructions)
                    .append(load)
                    .append(put);

            // scopedVariables->popVariableScope(valRes->temporaryVars);
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

            // scopedVariables->popVariableScope(idRes->temporaryVars + expRes->temporaryVars);
        } else if (auto ifNode = dynamic_cast<If *>(command)) { // IF
            InstructionList codeBlock = assembleCommands(ifNode->commands); // assemble inner instructions
            InstructionList &conditionBlock = assembleCondition(ifNode->condition, codeBlock); // assemble condition

            instructions.append(conditionBlock) // add condition code
                    .append(codeBlock); // add inner block code
        } else if (auto ifElseNode = dynamic_cast<IfElse *>(command)) { // IF ELSE
            InstructionList ifCodeBlock = assembleCommands(ifElseNode->commands);
            InstructionList &conditionBlock = assembleCondition(ifElseNode->condition, ifCodeBlock);
            InstructionList elseCodeBlock = assembleCommands(ifElseNode->elseCommands);

            Jump *jump = new Jump(elseCodeBlock.end());
            ifCodeBlock.append(jump);

            instructions.append(conditionBlock)
                    .append(ifCodeBlock)
                    .append(elseCodeBlock);
        } else if (auto whileNode = dynamic_cast<While *>(command)) { // WHILE
            InstructionList codeBlock = assembleCommands(whileNode->commands);

            if (whileNode->doWhile) {
                InstructionList *jumpBlock = new InstructionList();
                Jump *jump = new Jump(codeBlock.start());
                jumpBlock->append(jump);

                InstructionList &conditionBlock = assembleCondition(whileNode->condition, *jumpBlock);
                instructions.append(codeBlock)
                        .append(conditionBlock)
                        .append(*jumpBlock);
            } else {
                InstructionList &conditionBlock = assembleCondition(whileNode->condition, codeBlock);
                Jump *jump = new Jump(conditionBlock.start());
                codeBlock.append(jump);

                instructions.append(conditionBlock)
                        .append(codeBlock);
            }
        } else if (auto forNode = dynamic_cast<For *>(command)) {
            NumberVariable *iterator = new NumberVariable(
                    forNode->variableName,
                    *new ResolvableAddress(),
                    true
            );

            scopedVariables->pushVariableScope(iterator); // create iterator BEFORE commands would use it

            InstructionList &codeBlock = assembleCommands(forNode->commands); // assemble iterated commands

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
                    .append(startStore); // start value is in accumulator

            //scopedVariables->popVariableScope(startRes->temporaryVars + endRes->temporaryVars);

            Store *storeInI = new Store(iterator->getAddress());
            Sub *subStartEnd = new Sub(iterationEnd->getAddress());
            Instruction *j___ = forNode->reversed ? static_cast<Instruction *>(new Jneg(codeBlock.end())) : static_cast<Instruction *>(new Jpos(codeBlock.end()));

            Jump *jumpToSSJ = new Jump(storeInI);

            Load *loadI = new Load(iterator->getAddress());
            Instruction *incDec = forNode->reversed ? static_cast<Instruction *>(new Dec()) : static_cast<Instruction *>(new Inc());

            Jump *jumpToLoadIncSSJ = new Jump(loadI);
            codeBlock.append(jumpToLoadIncSSJ);

            instructions.append(jumpToSSJ)
                    .append(loadI)
                    .append(incDec)
                    .append(storeInI)
                    .append(subStartEnd)
                    .append(j___)
                    .append(codeBlock);

            // scopedVariables->popVariableScope(3);
        }
    }

    return instructions;
}

InstructionList &AbstractAssembler::assembleCondition(Condition &condition, InstructionList &codeBlock) {
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

    // scopedVariables->popVariableScope(lhsResolution->temporaryVars + rhsResolution->temporaryVars);

    return instructions;
}

Resolution *AbstractAssembler::assembleExpression(AbstractExpression &expression) {
    try {
        UnaryExpression &unaryExpression = dynamic_cast<UnaryExpression &>(expression);
        return resolve(unaryExpression.value);
    } catch (std::bad_cast e) {
        try {
            BinaryExpression &binaryExpression = dynamic_cast<BinaryExpression &>(expression);
            InstructionList &instructionList = *new InstructionList();

            TemporaryVariable *temp = new TemporaryVariable(TEMPORARY_NAMES, *new ResolvableAddress());
            scopedVariables->pushVariableScope(temp);

            Resolution *lhsResolution = resolve(binaryExpression.lhs);
            Resolution *rhsResolution = resolve(binaryExpression.rhs);

            Instruction *lhsLoad = lhsResolution->indirect ? static_cast<Instruction *>(new Loadi(lhsResolution->address)) : static_cast<Instruction *>(new Load(lhsResolution->address));
            Instruction *rhsLoad = rhsResolution->indirect ? static_cast<Instruction *>(new Loadi(rhsResolution->address)) : static_cast<Instruction *>(new Load(rhsResolution->address));
            Store *store = new Store(expressionAccumulator);

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

                    throw "NOT IMPLEMENTED";
                }
                    break;
                case DIVISION:
                    throw "NOT IMPLEMENTED";
                    break;
                case MULTIPLICATION:
                    throw "NOT IMPLEMENTED";
                    break;
                case MODULO:
                    throw "NOT IMPLEMENTED";
                    break;
            }

            Store *resultStore = new Store(temp->getAddress());
            instructionList.append(resultStore);

            return new Resolution(
                    instructionList,
                    temp->getAddress(),
                    false,
                    1 + lhsResolution->temporaryVars + rhsResolution->temporaryVars
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
                ResolvableAddress &startValueAddress = constants->getConstant(arrayVar->start)->getAddress(); // arr start
                Variable *variable = scopedVariables->resolveVariable(varAccId.accessName); // "b" variable
                ResolvableAddress &arrAddressAddress = constants->getConstant(arrayVar->getAddress().getAddress())->getAddress();

                InstructionList &instructionList = *new InstructionList();

                TemporaryVariable *tempVar = new TemporaryVariable(TEMPORARY_NAMES, *new ResolvableAddress());
                scopedVariables->pushVariableScope(tempVar);

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
                false
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
    instructions.append(assembleCommands(program.commands));
    instructions.seal();
    return instructions;
}
#include "AbstractAssembler.h"

InstructionList &AbstractAssembler::assembleConstants() {
    constants = new Constants(2);

    InstructionList &list = *new InstructionList();

    for (const auto num : program.constants.constants) {
        Constant *constantAddress = constants->addConstant(num);

        if (constantAddress) {
            std::cout << constantAddress->toString() + " at 0x" << std::setbase(16)
                      << constantAddress->getAddress().getAddress() << std::endl;

            list.append(constantAddress->generateConstant());
        }
    }

    return list;
}

void AbstractAssembler::getVariablesFromDeclarations() {
    scopedVariables = new ScopedVariables(constants->lastAddress());

    for (const auto &declaration : program.declarations.declarations) {
        if (auto numDecl = dynamic_cast<IdentifierDeclaration *>(declaration)) {
            NumberVariable *var = new NumberVariable(numDecl->name, *new ResolvableAddress());
            scopedVariables->pushVariableScope(var);
            std::cout << var->toString() << " at 0x" << std::setbase(16)
                      << var->getAddress().getAddress() << std::endl;
        } else if (auto arrDecl = dynamic_cast<ArrayDeclaration *>(declaration)) {
            NumberArrayVariable *var = new NumberArrayVariable(arrDecl->name, *new ResolvableAddress(), arrDecl->start,
                                                               arrDecl->end);
            scopedVariables->pushVariableScope(var);
            std::cout << var->toString() << " at 0x" << std::setbase(16) << var->getAddress().getAddress() << std::endl;
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
            ResolvableAddress &address = scopedVariables->resolveAddress(readNode->identifier); // variable address
            Get *get = new Get(); // GET
            Store *store = new Store(address); // STORE [address]
            instructions.append(get);
            instructions.append(store);
        } else if (auto writeNode = dynamic_cast<Write *>(command)) {
            ResolvableAddress &address = resolveValue(writeNode->value); // constant, variable, expression result
            Load *load = new Load(address); // LOAD [address]
            Put *put = new Put(); // PUT
            instructions.append(load);
            instructions.append(put);
        } else if (auto assignNode = dynamic_cast<Assignment *>(command)) {
            ResolvableAddress &variableAddress = scopedVariables->resolveAddress(assignNode->identifier); // get address
            ExpressionResolution *resolution = assembleExpression(assignNode->expression); // resolve expression

            if (auto addRes = dynamic_cast<AddressExpressionResolution *>(resolution)) { // it was just an address, load it
                Load *load = new Load(addRes->address);
                instructions.append(load);
            } else if (auto insRes = dynamic_cast<InstructionExpressionResolution *>(resolution)) { // instructions were generated, store p0 after them
                instructions.append(insRes->instructionList);
            }

            Store *store = new Store(variableAddress);
            instructions.append(store);
        } else if (auto ifNode = dynamic_cast<If *>(command)) { // IF
            InstructionList codeBlock = assembleCommands(ifNode->commands); // assemble inner instructions
            InstructionList &conditionBlock = assembleCondition(ifNode->condition, codeBlock); // assemble condition

            instructions.append(conditionBlock) // add condition code
                    .append(codeBlock); // add inner block code
        }
    }

    return instructions;
}

InstructionList &AbstractAssembler::assembleCondition(Condition &condition, InstructionList &codeBlock) {
    InstructionList &instructions = *new InstructionList();

    ResolvableAddress &lhsAddress = resolveValue(condition.lhs);
    ResolvableAddress &rhsAddress = resolveValue(condition.rhs);

    Load *load = new Load(lhsAddress);
    Sub *sub = new Sub(rhsAddress);

    instructions.append(load)
            .append(sub);

    switch (condition.type) {
        case EQUAL: {
            Jzero *jzero = new Jzero(codeBlock.end()); // jump to end of code block if they don't subtruct to zero

            instructions.append(jzero);
        }
            break;
        case NOT_EQUAL: {
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

    return instructions;
}

ExpressionResolution *AbstractAssembler::assembleExpression(AbstractExpression &expression) {
    try {
        UnaryExpression &unaryExpression = dynamic_cast<UnaryExpression &>(expression);
        return new AddressExpressionResolution(resolveValue(unaryExpression.value));
    } catch (std::bad_cast e) {
        try {
            BinaryExpression &binaryExpression = dynamic_cast<BinaryExpression &>(expression);
            InstructionList &instructionList = *new InstructionList();

            ResolvableAddress &lhsAddress = resolveValue(binaryExpression.lhs);
            ResolvableAddress &rhsAddress = resolveValue(binaryExpression.rhs);

            switch (binaryExpression.type) {
                case ADDITION: {
                    Load *load = new Load(lhsAddress);
                    Add *add = new Add(rhsAddress);

                    instructionList.append(load);
                    instructionList.append(add);
                }
                    break;
                case SUBTRACTION: {
                    Load *load = new Load(lhsAddress);
                    Sub *sub = new Sub(rhsAddress);

                    instructionList.append(load);
                    instructionList.append(sub);
                }
                    break;
                case DIVISION:
                    break;
                case MULTIPLICATION:
                    break;
                case MODULO:
                    throw "NOT IMPLEMENTED";
                    break;
            }

            return new InstructionExpressionResolution(instructionList);
        } catch (std::bad_cast ee) {}
    }
}

ResolvableAddress &AbstractAssembler::resolveValue(AbstractValue &value) {
    try {
        NumberValue &numberValue = dynamic_cast<NumberValue &>(value);
        return constants->getConstant(numberValue.value)->getAddress();
    } catch (std::bad_cast e) {
        try {
            IdentifierValue &identifierValue = dynamic_cast<IdentifierValue &>(value);
            return scopedVariables->resolveAddress(identifierValue.identifier);
        } catch (std::bad_cast ee) {
            throw "Value resolution error";
        }
    }
}

InstructionList &AbstractAssembler::assemble() {
    InstructionList &instructions = assembleConstants();
    getVariablesFromDeclarations();
    instructions.append(assembleCommands(program.commands));
    instructions.seal();
    return instructions;
}
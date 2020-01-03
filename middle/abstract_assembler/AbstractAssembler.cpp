#include "AbstractAssembler.h"

InstructionList &AbstractAssembler::assembleConstants() {
    InstructionList &list = *new InstructionList();

    for (const auto num : program.constants.constants) {
        Constant *constantAddress = constants.addConstant(num);

        if (constantAddress) {
            std::cout << constantAddress->toString() + " at 0x" << std::setbase(16)
                      << constantAddress->getAddress().getAddress() << std::endl;

            list.append(constantAddress->generateConstant());
        }
    }

    return list;
}

void AbstractAssembler::getVariablesFromDeclarations() {
    for (const auto &declaration : program.declarations.declarations) {
        if (auto numDecl = dynamic_cast<IdentifierDeclaration *>(declaration)) {
            NumberVariable *var = new NumberVariable(numDecl->name, *new ResolvableAddress());
            scopedVariables.pushVariableScope(var);
            std::cout << var->toString() << " at 0x" << std::setbase(16)
                      << var->getAddress().getAddress() << std::endl;
        } else if (auto arrDecl = dynamic_cast<ArrayDeclaration *>(declaration)) {
            NumberArrayVariable *var = new NumberArrayVariable(arrDecl->name, *new ResolvableAddress(), arrDecl->start,
                                                               arrDecl->end);
            scopedVariables.pushVariableScope(var);
            std::cout << var->toString() << " at 0x" << std::setbase(16) << var->getAddress().getAddress() << std::endl;
        }
    }
}

InstructionList &AbstractAssembler::assembleCommands(CommandList &commandList) {
    InstructionList &instructions = *new InstructionList();

    for (const auto &command : commandList.commands) {
        if (auto readNode = dynamic_cast<Read *>(command)) { // READ
            ResolvableAddress &address = scopedVariables.resolveAddress(readNode->identifier); // variable address
            Get *get = new Get(); // GET
            Store *store = new Store(address); // STORE [address]
            instructions.instructions.push_back(get);
            instructions.instructions.push_back(store);
        } else if (auto writeNode = dynamic_cast<Write *>(command)) {
            ResolvableAddress &address = resolveValue(writeNode->value); // constant, variable, expression result
            Load *load = new Load(address); // LOAD [address]
            Put *put = new Put(); // PUT
            instructions.instructions.push_back(load);
            instructions.instructions.push_back(put);
        } else if (auto ifNode = dynamic_cast<If *>(command)) { // IF
            InstructionList codeBlock = assembleCommands(ifNode->commands);
        }
    }

    return instructions;
}

InstructionList &AbstractAssembler::assembleCondition(Condition &condition) {
    InstructionList &instructions = *new InstructionList();

    return instructions;
}

ResolvableAddress &AbstractAssembler::resolveValue(AbstractValue &value) {
    try {
        NumberValue &numberValue = dynamic_cast<NumberValue &>(value);
        return constants.getConstant(numberValue.value)->getAddress();
    } catch (std::bad_cast e) {
        try {
            IdentifierValue &identifierValue = dynamic_cast<IdentifierValue &>(value);
            return scopedVariables.resolveAddress(identifierValue.identifier);
        } catch (std::bad_cast ee) {
            // TODO add expression value
            throw "Value resolution error";
        }
    }
}


InstructionList &AbstractAssembler::assemble() {
    InstructionList &constantsInstructions = assembleConstants();
    getVariablesFromDeclarations();
    constantsInstructions.append(assembleCommands(program.commands));
    constantsInstructions.instructions.push_back(new Halt());
    return constantsInstructions;
}
#include "AbstractAssembler.h"

void AbstractAssembler::getVariablesFromDeclarations() {
    for (const auto &declaration : program.declarations.declarations) {
        if (auto numDecl = dynamic_cast<IdentifierDeclaration *>(declaration)) {
            NumberVariable *var = new NumberVariable(numDecl->name, *new ResolvableAddress());
            scopedVariables.pushVariableScope(var);
            std::cout << "Variable '" << numDecl->name << "' added at 0x" << std::setbase(16)
                      << var->getAddress().getAddress() << std::endl;
        } else if (auto arrDecl = dynamic_cast<ArrayDeclaration *>(declaration)) {
            NumberArrayVariable *var = new NumberArrayVariable(arrDecl->name, *new ResolvableAddress(), arrDecl->start,
                                                               arrDecl->end);
            scopedVariables.pushVariableScope(var);
            std::cout << "Array variable '" << arrDecl->name <<
                      "' added at 0x" << std::setbase(16) << var->getAddress().getAddress() << std::endl;
        }
    }
}

InstructionList &AbstractAssembler::assembleCommands(CommandList &commandList) {
    InstructionList &instructions = *new InstructionList();

    for (const auto &command : commandList.commands) { // IF TRANSLATION
        if (auto ifNode = dynamic_cast<If *>(command)) {
            InstructionList results = assembleCommands(ifNode->commands);

        }
    }

    return instructions;
}

InstructionList &AbstractAssembler::assembleCondition(Condition &condition) {
    InstructionList &instructions = *new InstructionList();

    return instructions;
}

void AbstractAssembler::getConstants() {
    for (const auto num : program.constants.constants) {
        constants.addConstant(num);
    }
}

InstructionList &AbstractAssembler::assemble() {
    getVariablesFromDeclarations();
    getConstants();
    return assembleCommands(program.commands);
}
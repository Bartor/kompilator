#include "ASTOptimizer.h"

bool ASTOptimizer::traverse(CommandList &commandList, Callback callback) {
    bool changed = false;
    for (int i = 0; i < commandList.commands.size(); i++) {
        Node *newNode = callback(commandList.commands[i]);
        if (newNode != commandList.commands[i]) {
            commandList.commands[i] = newNode;
            changed = true;
        }
        traverse(commandList.commands[i], callback);
    }
    return changed;
}

bool ASTOptimizer::traverse(Node *node, Callback callback) {
    if (auto cmdList = dynamic_cast<CommandList *>(node)) {
        return traverse(*cmdList, callback);
    } else if (auto whileNode = dynamic_cast<While *>(node)) {
        return traverse(whileNode->commands, callback);
    } else if (auto forNode = dynamic_cast<For *>(node)) {
        return traverse(forNode->commands, callback);
    } else if (auto ifNode = dynamic_cast<If *>(node)) {
        return traverse(ifNode->commands, callback);
    } else if (auto ifElse = dynamic_cast<IfElse *>(node)) {
        return traverse(ifElse->commands, callback) || traverse(ifElse->elseCommands, callback);
    }
    return false;
}

Node *ASTOptimizer::constantExpressionOptimizer(Node *node) {
    if (auto assignNode = dynamic_cast<Assignment *>(node)) {
        try {
            auto binaryExpression = dynamic_cast<BinaryExpression &>(assignNode->expression);
            try {
                auto lhsConstant = dynamic_cast<NumberValue &>(binaryExpression.lhs);
                try {
                    auto rhsConstant = dynamic_cast<NumberValue &>(binaryExpression.rhs);

                    std::cout << "- Optimizing constant binary expression - HANDLE OVERFLOWS" << std::endl;

                    long long newValue = 0;
                    switch (binaryExpression.type) {
                        case ADDITION:
                            newValue = lhsConstant.value + rhsConstant.value;
                            break;
                        case SUBTRACTION:
                            newValue = lhsConstant.value - rhsConstant.value;
                            break;
                        case MULTIPLICATION:
                            newValue = lhsConstant.value * rhsConstant.value;
                            break;
                        case DIVISION:
                            newValue = floor((double) lhsConstant.value / (double) rhsConstant.value);
                            break;
                        case MODULO:
                            newValue = lhsConstant.value - floor((double) lhsConstant.value / (double) rhsConstant.value) * rhsConstant.value;
                            break;
                    }

                    originalProgram->constants.constants.push_back(newValue);

                    return new Assignment(assignNode->identifier, *new UnaryExpression(*new NumberValue(newValue)));
                } catch (std::bad_cast _) {}
            } catch (std::bad_cast _) {}
        } catch (std::bad_cast _) {}
    }
    return node;
}

Node *ASTOptimizer::constantLoopUnroller(Node *node) {
    if (auto forNode = dynamic_cast<For *>(node)) {
        try {
            auto startConstant = dynamic_cast<NumberValue &>(forNode->startValue);
            try {
                auto endConstant = dynamic_cast<NumberValue &>(forNode->endValue);

                CommandList *cmdList = new CommandList();

                if (forNode->reversed) {
                    for (long long i = startConstant.value; i >= endConstant.value; i--) {
                        std::cout << i << std::endl;
                        Callback replacer = iteratorReplacer(forNode->variableName, i);
                        originalProgram->constants.constants.push_back(i);
                        cmdList->append(*static_cast<CommandList *>(forNode->commands.copy(replacer)));
                    }
                } else {
                    for (long long i = startConstant.value; i <= endConstant.value; i++) {
                        std::cout << i << std::endl;
                        Callback replacer = iteratorReplacer(forNode->variableName, i);
                        originalProgram->constants.constants.push_back(i);
                        cmdList->append(*static_cast<CommandList *>(forNode->commands.copy(replacer)));
                    }
                }

                return cmdList;
            } catch (std::bad_cast _) {}
        } catch (std::bad_cast _) {}
    }
    return node;
}

Callback ASTOptimizer::iteratorReplacer(std::string &variableToReplace, long long value) {
    return [this, variableToReplace, value](Node *node) -> Node * {
        if (auto idVal = dynamic_cast<IdentifierValue *>(node)) {
            try {
                VariableIdentifier &varId = dynamic_cast<VariableIdentifier &>(idVal->identifier);

                if (varId.name == variableToReplace) {
                    return new NumberValue(value);
                }
            } catch (std::bad_cast _) {
                try {
                    VariableAccessIdentifier &varAccId = dynamic_cast<VariableAccessIdentifier &>(idVal->identifier);

                    if (varAccId.accessName == variableToReplace) {
                        return new IdentifierValue(*new AccessIdentifier(varAccId.name, value));
                    }
                } catch (std::bad_cast _) {}
            }
        }
        return node;
    };
}

void ASTOptimizer::optimize() {
    while (traverse(originalProgram->commands, [this](Node *node) -> Node * { return constantLoopUnroller(node); })) {
        std::cout << "Unrolling constant loops" << std::endl;
    }
    while (traverse(originalProgram->commands, [this](Node *node) -> Node * { return constantExpressionOptimizer(node); })) {
        std::cout << "Replacing constant expressions" << std::endl;
    }
}
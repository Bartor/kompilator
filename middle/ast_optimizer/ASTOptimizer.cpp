#include "ASTOptimizer.h"

void ASTOptimizer::traverse(CommandList &commandList, Callback callback) {
    for (int i = 0; i < commandList.commands.size(); i++) {
        commandList.commands[i] = callback(commandList.commands[i]);
        traverse(commandList.commands[i], callback);
    }
}

void ASTOptimizer::traverse(Node *node, Callback callback) {
    if (auto whileNode = dynamic_cast<While *>(node)) {
        traverse(whileNode->commands, callback);
    } else if (auto forNode = dynamic_cast<For *>(node)) {
        traverse(forNode->commands, callback);
    } else if (auto ifNode = dynamic_cast<If *>(node)) {
        traverse(ifNode->commands, callback);
    } else if (auto ifElse = dynamic_cast<IfElse *>(node)) {
        traverse(ifElse->commands, callback);
        traverse(ifElse->elseCommands, callback);
    }
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

                long long iterationStart = forNode->reversed ? endConstant.value : startConstant.value;
                long long iterationEnd = forNode->reversed ? startConstant.value : endConstant.value;

                CommandList *cmdList = new CommandList();

                for (long long i = iterationStart; forNode->reversed ? i >= iterationEnd : i <= iterationEnd; forNode->reversed ? i-- : i++) {
                    Callback replacer = iteratorReplacer(forNode->variableName, i);
                    originalProgram->constants.constants.push_back(i);
                    CommandList *forCommands = static_cast<CommandList *>(forNode->commands.copy(replacer));
                    cmdList->append(*forCommands);
                }

                std::cout << "generated new commandlist" << std::endl << cmdList->toString(0) << std::endl;
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
    traverse(originalProgram->commands, [this](Node *node) -> Node * { return constantExpressionOptimizer(node); });
    traverse(originalProgram->commands, [this](Node *node) -> Node * { return constantLoopUnroller(node); });
}
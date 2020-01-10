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


            } catch (std::bad_cast _) {}
        } catch (std::bad_cast _) {}
    }
    return node;
}

Node* ASTOptimizer::iteratorReplacer(Node *node, VariableIdentifier *variableToReplace, NumberValue *valueToPlace) {
    throw "NOT IMPLEMENTED";
    return node;
}

void ASTOptimizer::optimize() {
    traverse(originalProgram->commands, [this](Node *node) -> Node * { return constantExpressionOptimizer(node); });
}
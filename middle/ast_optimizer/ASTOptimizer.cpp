#include "ASTOptimizer.h"

Callback identity = [](Node *node) -> Node * { return node; };

enum ConditionState {
    NEVER,
    SOMETIMES,
    ALWAYS
};

ConditionState checkTautology(Condition &condition) {
    try {
        NumberValue &lVal = dynamic_cast<NumberValue &>(condition.lhs);
        NumberValue &rVal = dynamic_cast<NumberValue &>(condition.rhs);

        switch (condition.type) {
            case EQUAL:
                return lVal.value == rVal.value ? ALWAYS : NEVER;
            case NOT_EQUAL:
                return lVal.value != rVal.value ? ALWAYS : NEVER;
            case LESS:
                return lVal.value < rVal.value ? ALWAYS : NEVER;
            case GREATER:
                return lVal.value > rVal.value ? ALWAYS : NEVER;
            case LESS_OR_EQUAL:
                return lVal.value <= rVal.value ? ALWAYS : NEVER;
            case GREATER_OR_EQUAL:
                return lVal.value >= rVal.value ? ALWAYS : NEVER;
        }
    } catch (std::bad_cast _) {
        try {
            IdentifierValue &lVal = dynamic_cast<IdentifierValue &>(condition.lhs);
            IdentifierValue &rVal = dynamic_cast<IdentifierValue &>(condition.rhs);

            try {
                VariableIdentifier &lValId = dynamic_cast<VariableIdentifier &>(lVal.identifier);
                VariableIdentifier &rValId = dynamic_cast<VariableIdentifier &>(rVal.identifier);

                if (lValId.name == rValId.name) {
                    switch (condition.type) {
                        case EQUAL:
                        case LESS_OR_EQUAL:
                        case GREATER_OR_EQUAL:
                            return ALWAYS;
                        case NOT_EQUAL:
                        case LESS:
                        case GREATER:
                            return NEVER;
                    }
                } else return SOMETIMES;
            } catch (std::bad_cast _) {
                try {
                    AccessIdentifier &lValAcc = dynamic_cast<AccessIdentifier &>(lVal.identifier);
                    AccessIdentifier &rValAcc = dynamic_cast<AccessIdentifier &>(rVal.identifier);

                    if (lValAcc.name == rValAcc.name && lValAcc.index == rValAcc.index) {
                        switch (condition.type) {
                            case EQUAL:
                            case LESS_OR_EQUAL:
                            case GREATER_OR_EQUAL:
                                return ALWAYS;
                            case NOT_EQUAL:
                            case LESS:
                            case GREATER:
                                return NEVER;
                        }
                    } else return SOMETIMES;
                } catch (std::bad_cast) {
                    try {
                        VariableAccessIdentifier &lValVarAcc = dynamic_cast<VariableAccessIdentifier &>(lVal.identifier);
                        VariableAccessIdentifier &rValVarAcc = dynamic_cast<VariableAccessIdentifier &>(rVal.identifier);

                        if (lValVarAcc.name == rValVarAcc.name && lValVarAcc.accessName == rValVarAcc.accessName) {
                            switch (condition.type) {
                                case EQUAL:
                                case LESS_OR_EQUAL:
                                case GREATER_OR_EQUAL:
                                    return ALWAYS;
                                case NOT_EQUAL:
                                case LESS:
                                case GREATER:
                                    return NEVER;
                            }
                        } else return SOMETIMES;
                    } catch (std::bad_cast _) {}
                }
            }
        } catch (std::bad_cast _) {}
    }
    return SOMETIMES;
}

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
                        Callback replacer = iteratorReplacer(forNode->variableName, i);
                        originalProgram->constants.constants.push_back(i);
                        cmdList->append(*static_cast<CommandList *>(forNode->commands.copy(replacer)));
                    }
                } else {
                    for (long long i = startConstant.value; i <= endConstant.value; i++) {
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

Node *ASTOptimizer::constantConditionRemover(Node *node) {
    if (auto whileNode = dynamic_cast<While *>(node)) {
        if (checkTautology(whileNode->condition) == ALWAYS) {
            std::cout << "  |[w] Infinite loop detected" << std::endl;
            return node;
        } else if (checkTautology(whileNode->condition) == NEVER) {
            return new CommandList();
        }
    } else if (auto ifNode = dynamic_cast<If *>(node)) {
        if (checkTautology(ifNode->condition) == ALWAYS) {
            return ifNode->commands.copy(identity);
        } else if (checkTautology(ifNode->condition) == NEVER) {
            return new CommandList();
        }
    } else if (auto ifElseNode = dynamic_cast<IfElse *>(node)) {
        if (checkTautology(ifElseNode->condition) == ALWAYS) {
            return ifElseNode->commands.copy(identity);
        } else if (checkTautology(ifElseNode->condition) == NEVER) {
            return ifElseNode->elseCommands.copy(identity);
        }
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
        } else if (auto varArrId = dynamic_cast<VariableAccessIdentifier *>(node)) {
            if (varArrId->accessName == variableToReplace) return new AccessIdentifier(varArrId->name, value);
        }
        return node;
    };
}

void ASTOptimizer::optimize(bool verbose) {
    while (traverse(originalProgram->commands, [this](Node *node) -> Node * { return constantConditionRemover(node); })) {
        std::cout << "   [i] flattening always true expressions" << std::endl;
    }

    while (traverse(originalProgram->commands, [this](Node *node) -> Node * { return constantLoopUnroller(node); })) {
        std::cout << "   [i] unrolling constant loops" << std::endl;
    }
    while (traverse(originalProgram->commands, [this](Node *node) -> Node * { return constantExpressionOptimizer(node); })) {
        std::cout << "   [i] Replacing constant expressions" << std::endl;
    }
}
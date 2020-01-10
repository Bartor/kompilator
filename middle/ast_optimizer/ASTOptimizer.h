#include "../../front/ast/node.h"
#include <math.h>
#include <functional>
#include <iostream>

#ifndef COMPILER_ASTOPTIMIZER_H
#define COMPILER_ASTOPTIMIZER_H

typedef std::function<Node *(Node *)> Callback;

class ASTOptimizer {
private:
    Program *originalProgram;

    void traverse(Node *node, Callback callback);

    void traverse(CommandList &commandList, Callback callback);

    Node *constantExpressionOptimizer(Node *node);

    Node *constantLoopUnroller(Node *node);

    Node *iteratorReplacer(Node *node, VariableIdentifier *variableToReplace, NumberValue *valueToPlace);
public:
    ASTOptimizer(Program *program) : originalProgram(program) {}

    void optimize();
};

#endif //COMPILER_ASTOPTIMIZER_H

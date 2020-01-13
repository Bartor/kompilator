#include "../../front/ast/node.h"
#include <math.h>
#include <functional>
#include <iostream>

#ifndef COMPILER_ASTOPTIMIZER_H
#define COMPILER_ASTOPTIMIZER_H

class ASTOptimizer {
private:
    Program *originalProgram;

    bool traverse(Node *node, Callback callback);

    bool traverse(CommandList &commandList, Callback callback);

    Node *constantExpressionOptimizer(Node *node);

    Node *constantLoopUnroller(Node *node);

    Callback iteratorReplacer(std::string &variableToReplace, long long value);
public:
    ASTOptimizer(Program *program) : originalProgram(program) {}

    void optimize();
};

#endif //COMPILER_ASTOPTIMIZER_H

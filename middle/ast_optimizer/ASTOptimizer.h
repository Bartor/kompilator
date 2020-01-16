#include "../../front/ast/node.h"
#include <math.h>
#include <functional>
#include <iostream>

#ifndef COMPILER_ASTOPTIMIZER_H
#define COMPILER_ASTOPTIMIZER_H

/**
 * High abstraction class for AST optimizations, based
 * on callback functions and recursive replacer-copying
 * mechanism (implemented in ast node definitions).
 */
class ASTOptimizer {
private:
    Program *originalProgram;

    /**
     * Traverses programs nodes, calling traverse on every command in node's
     * command list (if it has any).
     * @param node Node to be traversed.
     * @param callback Callback used with called traverse.
     * @return True if any element was changed, false otherwise.
     */
    bool traverse(Node *node, Callback callback);

    /**
     * Calls callback on each command on the list and changes the
     * command on the list if the callback returned something else
     * that the original command.
     * @param commandList Commands to be traversed.
     * @param callback Callback which is called on each command.
     * @return True if any element was changed, false otherwise.
     */
    bool traverse(CommandList &commandList, Callback callback);

    /**
     * Creates an UnaryExpression for every BinaryExpression
     * with both values being constants, basically doing compile-time
     * maths.
     * @param node Node to be checked.
     * @return Original node if it wasn't a BinaryExpression with
     * constant rhs and lhs; UnaryExpression with proper value
     * otherwise.
     */
    Node *constantExpressionOptimizer(Node *node);

    /**
     * Creates a command list node for every for loop
     * with iteration start and end is a constant value, repeating
     * commands in the loop with iterator changed to incremented
     * values.
     * @param node Node to be checked.
     * @return Original node if it wasn't a for loop with
     * constant start and end value; unrolled loop as a
     * command list otherwise.
     */
    Node *constantLoopUnroller(Node *node);

    /**
     * Searches for condtions which always hold or always fail
     * and removes them/warns about them
     * @param node Node to be checked.
     * @return Original node, if there were no constant condition in it;
     * (possibly empty) instruction list otherwise.
     */
    Node *constantConditionRemover(Node *node);

    /**
     * A copy-callback replacer which replaces each use of some
     * variable by a number value, e.g.
     * a ASSIGN i PLUS 1 ==> a ASSIGN 10 PLUS 1
     * a[i] ASSIGN 0 ==> a[10] ASSIGN 0
     * @param variableToReplace Name of the iterator value to look for.
     * @param value A numeric value to replace it to.
     * @return Original node if it doesn't use the specifier variable
     * name anywhere; node with name replaced to the numeric value
     * everywhere otherwise.
     */
    Callback iteratorReplacer(std::string &variableToReplace, long long value);

public:
    ASTOptimizer(Program *program) : originalProgram(program) {}

    void optimize(bool verbose);
};

#endif //COMPILER_ASTOPTIMIZER_H

/**
 * AST class definitions
 */

#include <vector>
#include <string>

#ifndef COMPILER_NODE_H
#define COMPILER_NODE_H

enum BinaryExpressionType {
    ADDITION,
    SUBTRACTION,
    MULTIPLICATION,
    DIVISION,
    MODULO
};

enum ConditionType {
    EQUAL,
    NOT_EQUAL,
    LESS,
    GREATER,
    LESS_OR_EQUAL,
    GREATER_OR_EQUAL
};

/**
 * Base abstract class for representing anything in AST
 */
class Node {
public:
    virtual ~Node() {}
};

/**
 * Base class for VariableIdentifier, AccessIdentifier and VariableAccessIdentifier
 */
class AbstractIdentifier : public Node {
public:
    virtual ~AbstractIdentifier() {}
};

/**
 * a
 */
class VariableIdentifier : public AbstractIdentifier {
public:
    std::string &name;

    VariableIdentifier(std::string &name) : name(name) {}
};

/**
 * a[0]
 */
class AccessIdentifier : public AbstractIdentifier {
    std::string &name;
    long long index;

    AccessIdentifier(std::string &name, long long index) : name(name), index(index) {}
};

/**
 * a[b]
 */
class VariableAccessIdentifier : public AbstractIdentifier {
    std::string &name;
    VariableIdentifier &variableIdentifier;

    VariableAccessIdentifier(std::string &name, VariableIdentifier &variableIdentifier)
            : name(name), variableIdentifier(variableIdentifier) {}
};

/**
 * Values are used in iterators, expressions and conditions
 */
class AbstractValue : public Node {
public:
    virtual ~AbstractValue() {}
};

/**
 * 5
 */
class NumberValue : public AbstractValue {
public:
    long long value;

    NumberValue(long long value) : value(value) {}
};

/**
 * a, a[0], a[b]
 */
class IdentifierValue : public AbstractValue {
    AbstractIdentifier &identifier;

    IdentifierValue(AbstractIdentifier &identifier) : identifier(identifier) {}
};

/**
 * Basically math
 */
class AbstractExpression : public Node {
public:
    virtual ~AbstractExpression() {}
};

/**
 * Basically a Value
 */
class UnaryExpression : public AbstractExpression {
public:
    AbstractValue &value;

    UnaryExpression(AbstractValue &value) : value(value) {}
};

/**
 * +, -, *, /, %
 */
class BinaryExpression : public AbstractExpression {
public:
    AbstractValue &lhs;
    AbstractValue &rhs;
    BinaryExpressionType type;

    BinaryExpression(AbstractValue &lhs, AbstractValue &rhs, BinaryExpressionType type)
            : lhs(lhs), rhs(rhs), type(type) {}
};

/**
 * >, <, ==, !==, >=, <=
 */
class Condition : public Node {
public:
    AbstractValue &lhs;
    AbstractValue &rhs;
    ConditionType type;

    Condition(AbstractValue &lhs, AbstractValue &rhs, ConditionType type)
            : lhs(lhs), rhs(rhs), type(type) {}
};

/**
 * Base abstract class for data manipulation, looping etc.
 */
class Command : public Node {
public:
    virtual ~Command() {}
};

class CommandList : public Node {
public:
    std::vector<Command *> commands;

    CommandList() {}
};

class Assignment : public Command {
public:
    AbstractIdentifier &identifier;
    AbstractExpression &expression;

    Assignment(AbstractIdentifier &identifier, AbstractExpression &expression)
            : identifier(identifier), expression(expression) {}
};

class If : public Command {
public:
    Condition &condition;
    CommandList &commands;

    If(Condition &condition, CommandList &commands) : condition(condition), commands(commands) {}
};

class IfElse : public If {
public:
    CommandList &elseCommands;

    IfElse(Condition &condition, CommandList &commands, CommandList &elseCommands)
            : If(condition, commands), elseCommands(elseCommands) {}
};

class While : public Command {
public:
    Condition &condition;
    CommandList &commands;
    bool doWhile;

    While(Condition &condition, CommandList &commands, bool doWhile = false)
            : condition(condition), commands(commands), doWhile(doWhile) {}
};

class For : public Command {
public:
    std::string variableName;
    AbstractValue &startValue;
    AbstractValue &endValue;
    CommandList &commands;
    bool reversed;

    For(std::string &variableName, AbstractValue &startValue, AbstractValue &endValue,
        CommandList &commands,
        bool reversed = false)
            : variableName(variableName), startValue(startValue), endValue(endValue), commands(commands),
              reversed(reversed) {}
};

class Read : public Command {
public:
    AbstractIdentifier &identifier;

    Read(AbstractIdentifier &identifier) : identifier(identifier) {}
};

class Write : public Command {
public:
    AbstractValue &value;

    Write(AbstractValue &value) : value(value) {}
};

class AbstractDeclaration : public Node {
public:
    virtual ~AbstractDeclaration() {}
};

class IdentifierDeclaration : public AbstractDeclaration {
public:
    std::string &name;

    IdentifierDeclaration(std::string &name) : name(name) {}
};

class ArrayDeclaration : public AbstractDeclaration {
public:
    std::string &name;
    long long start;
    long long end;

    ArrayDeclaration(std::string &name, long long start, long long end) : name(name), start(start), end(end) {}
};

class DeclarationList : public Node {
public:
    std::vector<AbstractDeclaration *> declarations;

    DeclarationList() {}
};

class Program {
public:
    DeclarationList declarations;
    CommandList commands;

    Program(DeclarationList &declarationList, CommandList &commandList)
            : declarations(declarationList), commands(commandList) {}
};

#endif //COMPILER_NODE_H

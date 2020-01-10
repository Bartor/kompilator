/**
 * AST class definitions
 */

#include <string>
#include <vector>

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
    std::string indent(int indentation);

    virtual std::string toString(int indentation) = 0;

    virtual ~Node() {}
};

/**
 * Base class for VariableIdentifier, AccessIdentifier and VariableAccessIdentifier
 */
class AbstractIdentifier : public Node {
public:
    std::string &name;

    AbstractIdentifier(std::string &name) : name(name) {}

    virtual ~AbstractIdentifier() {}
};

/**
 * a
 */
class VariableIdentifier : public AbstractIdentifier {
public:
    virtual std::string toString(int indentation);

    VariableIdentifier(std::string &name) : AbstractIdentifier(name) {}
};

/**
 * a[0]
 */
class AccessIdentifier : public AbstractIdentifier {
public:
    long long index;

    virtual std::string toString(int indentation);

    AccessIdentifier(std::string &name, long long index) : AbstractIdentifier(name), index(index) {}
};

/**
 * a[b]
 */
class VariableAccessIdentifier : public AbstractIdentifier {
public:
    std::string &accessName;

    virtual std::string toString(int indentation);

    VariableAccessIdentifier(std::string &name, std::string &accessName)
            : AbstractIdentifier(name), accessName(accessName) {}
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

    virtual std::string toString(int indentation);

    NumberValue(long long value) : value(value) {}
};

/**
 * a, a[0], a[b]
 */
class IdentifierValue : public AbstractValue {
public:
    AbstractIdentifier &identifier;

    virtual std::string toString(int indentation);

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

    virtual std::string toString(int indentation);

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

    virtual std::string toString(int indentation);

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

    virtual std::string toString(int indentation);

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
    std::vector<Node *> commands;

    virtual std::string toString(int indentation);

    CommandList() {}
};

class Assignment : public Command {
public:
    AbstractIdentifier &identifier;
    AbstractExpression &expression;

    virtual std::string toString(int indentation);

    Assignment(AbstractIdentifier &identifier, AbstractExpression &expression)
            : identifier(identifier), expression(expression) {}
};

class If : public Command {
public:
    Condition &condition;
    CommandList &commands;

    virtual std::string toString(int indentation);

    If(Condition &condition, CommandList &commands) : condition(condition), commands(commands) {}
};

class IfElse : public Command {
public:
    Condition &condition;
    CommandList &commands;
    CommandList &elseCommands;

    virtual std::string toString(int indentation);

    IfElse(Condition &condition, CommandList &commands, CommandList &elseCommands)
            : condition(condition), commands(commands), elseCommands(elseCommands) {}
};

class While : public Command {
public:
    Condition &condition;
    CommandList &commands;
    bool doWhile;

    virtual std::string toString(int indentation);

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

    virtual std::string toString(int indentation);

    For(std::string &variableName, AbstractValue &startValue, AbstractValue &endValue,
        CommandList &commands,
        bool reversed = false)
            : variableName(variableName), startValue(startValue), endValue(endValue), commands(commands),
              reversed(reversed) {}
};

class Read : public Command {
public:
    AbstractIdentifier &identifier;

    virtual std::string toString(int indentation);

    Read(AbstractIdentifier &identifier) : identifier(identifier) {}
};

class Write : public Command {
public:
    AbstractValue &value;

    virtual std::string toString(int indentation);

    Write(AbstractValue &value) : value(value) {}
};

class AbstractDeclaration : public Node {
public:
    virtual ~AbstractDeclaration() {}
};

class IdentifierDeclaration : public AbstractDeclaration {
public:
    std::string &name;

    virtual std::string toString(int indentation);

    IdentifierDeclaration(std::string &name) : name(name) {}
};

class ArrayDeclaration : public AbstractDeclaration {
public:
    std::string &name;
    long long start;
    long long end;

    virtual std::string toString(int indentation);

    ArrayDeclaration(std::string &name, long long start, long long end) : name(name), start(start), end(end) {}
};

class DeclarationList : public Node {
public:
    std::vector<AbstractDeclaration *> declarations;

    virtual std::string toString(int indentation);

    DeclarationList() {}
};

class ConstantList {
public:
    std::vector<long long> constants;
};

class Program {
public:
    DeclarationList &declarations;
    CommandList &commands;
    ConstantList &constants;

    Program(DeclarationList &declarationList, CommandList &commandList, ConstantList &constantList)
            : declarations(declarationList), commands(commandList), constants(constantList) {}

    std::string toString();
};

#endif  //COMPILER_NODE_H

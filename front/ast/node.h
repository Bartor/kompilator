/**
 * AST class definitions
 */

#include <string>
#include <vector>
#include <functional>

#ifndef COMPILER_NODE_H
#define COMPILER_NODE_H

class Node;

typedef std::function<Node *(Node *)> Callback;

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

    virtual Node *copy(Callback replacer) = 0;

    virtual ~Node() {}
};

/**
 * Base class for VariableIdentifier, AccessIdentifier and VariableAccessIdentifier
 */
class AbstractIdentifier : public Node {
public:
    std::string &name;

    AbstractIdentifier(std::string &name) : name(name) {}

    virtual Node *copy(Callback replacer) = 0;

    virtual ~AbstractIdentifier() {}
};

/**
 * a
 */
class VariableIdentifier : public AbstractIdentifier {
public:
    virtual std::string toString(int indentation);

    virtual Node *copy(Callback replacer) {
        return replacer(new VariableIdentifier(name));
    }

    VariableIdentifier(std::string &name) : AbstractIdentifier(name) {}
};

/**
 * a[0]
 */
class AccessIdentifier : public AbstractIdentifier {
public:
    long long index;

    virtual std::string toString(int indentation);

    virtual Node *copy(Callback replacer) {
        return replacer(new AccessIdentifier(name, index));
    }

    AccessIdentifier(std::string &name, long long index) : AbstractIdentifier(name), index(index) {}
};

/**
 * a[b]
 */
class VariableAccessIdentifier : public AbstractIdentifier {
public:
    std::string &accessName;

    virtual std::string toString(int indentation);

    virtual Node *copy(Callback replacer) {
        return replacer(new VariableAccessIdentifier(name, accessName));
    }

    VariableAccessIdentifier(std::string &name, std::string &accessName)
            : AbstractIdentifier(name), accessName(accessName) {}
};

/**
 * Values are used in iterators, expressions and conditions
 */
class AbstractValue : public Node {
public:
    virtual ~AbstractValue() {}

    virtual Node *copy(Callback replacer) = 0;
};

/**
 * 5
 */
class NumberValue : public AbstractValue {
public:
    long long value;

    virtual std::string toString(int indentation);

    virtual Node *copy(Callback replacer) {
        return replacer(new NumberValue(value));
    }

    NumberValue(long long value) : value(value) {}
};

/**
 * a, a[0], a[b]
 */
class IdentifierValue : public AbstractValue {
public:
    AbstractIdentifier &identifier;

    virtual std::string toString(int indentation);

    virtual Node *copy(Callback replacer) {
        return replacer(new IdentifierValue(*static_cast<AbstractIdentifier *>(identifier.copy(replacer))));
    }

    IdentifierValue(AbstractIdentifier &identifier) : identifier(identifier) {}
};

/**
 * Basically math
 */
class AbstractExpression : public Node {
public:
    virtual ~AbstractExpression() {}

    virtual Node *copy(Callback replacer) = 0;
};

/**
 * Basically a Value
 */
class UnaryExpression : public AbstractExpression {
public:
    AbstractValue &value;

    virtual std::string toString(int indentation);

    virtual Node *copy(Callback replacer) {
        return replacer(new UnaryExpression(*static_cast<AbstractValue *>(value.copy(replacer))));
    }

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

    virtual Node *copy(Callback replacer) {
        return replacer(new BinaryExpression(*static_cast<AbstractValue *>(lhs.copy(replacer)), *static_cast<AbstractValue *>(rhs.copy(replacer)), type));
    }

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

    virtual Node *copy(Callback replacer) {
        return replacer(new Condition(*static_cast<AbstractValue *>(lhs.copy(replacer)), *static_cast<AbstractValue *>(rhs.copy(replacer)), type));
    }

    Condition(AbstractValue &lhs, AbstractValue &rhs, ConditionType type)
            : lhs(lhs), rhs(rhs), type(type) {}
};

/**
 * Base abstract class for data manipulation, looping etc.
 */
class Command : public Node {
public:
    virtual ~Command() {}

    virtual Node *copy(Callback replacer) = 0;
};

class CommandList : public Node {
public:
    std::vector<Node *> commands;

    virtual std::string toString(int indentation);

    virtual Node *copy(Callback replacer) {
        CommandList *cmdList = new CommandList();

        for (const auto &cmd : commands) {
            cmdList->commands.push_back(cmd->copy(replacer));
        }
        return cmdList;
    }

    void append(CommandList &commandList) {
        commands.insert(commands.end(), commandList.commands.begin(), commandList.commands.end());
    }

    CommandList() {}
};

class Assignment : public Command {
public:
    AbstractIdentifier &identifier;
    AbstractExpression &expression;

    virtual std::string toString(int indentation);

    virtual Node *copy(Callback replacer) {
        return replacer(new Assignment(*static_cast<AbstractIdentifier *>(identifier.copy(replacer)), *static_cast<AbstractExpression *>(expression.copy(replacer))));
    }

    Assignment(AbstractIdentifier &identifier, AbstractExpression &expression)
            : identifier(identifier), expression(expression) {}
};

class If : public Command {
public:
    Condition &condition;
    CommandList &commands;

    virtual std::string toString(int indentation);

    virtual Node *copy(Callback replacer) {
        return replacer(new If(*static_cast<Condition *>(condition.copy(replacer)), *static_cast<CommandList * >(commands.copy(replacer))));
    }

    If(Condition &condition, CommandList &commands) : condition(condition), commands(commands) {}
};

class IfElse : public Command {
public:
    Condition &condition;
    CommandList &commands;
    CommandList &elseCommands;

    virtual std::string toString(int indentation);

    virtual Node *copy(Callback replacer) {
        return replacer(
                new IfElse(*static_cast<Condition *>(condition.copy(replacer)), *static_cast<CommandList *>(commands.copy(replacer)), *static_cast<CommandList *>(elseCommands.copy(replacer))));
    }

    IfElse(Condition &condition, CommandList &commands, CommandList &elseCommands)
            : condition(condition), commands(commands), elseCommands(elseCommands) {}
};

class While : public Command {
public:
    Condition &condition;
    CommandList &commands;
    bool doWhile;

    virtual std::string toString(int indentation);

    virtual Node *copy(Callback replacer) {
        return replacer(new While(*static_cast<Condition *>(condition.copy(replacer)), *static_cast<CommandList *>(commands.copy(replacer)), doWhile));
    }

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

    virtual Node *copy(Callback replacer) {
        return replacer(new For(variableName, *static_cast<AbstractValue *>(startValue.copy(replacer)), *static_cast<AbstractValue *>(endValue.copy(replacer)),
                                *static_cast<CommandList *>(commands.copy(replacer)), reversed));
    }

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

    virtual Node *copy(Callback replacer) {
        return replacer(new Read(*static_cast<AbstractIdentifier *>(identifier.copy(replacer))));
    }

    Read(AbstractIdentifier &identifier) : identifier(identifier) {}
};

class Write : public Command {
public:
    AbstractValue &value;

    virtual std::string toString(int indentation);

    virtual Node *copy(Callback replacer) {
        return replacer(new Write(*static_cast<AbstractValue *>(value.copy(replacer))));
    }

    Write(AbstractValue &value) : value(value) {}
};

class AbstractDeclaration : public Node {
public:
    virtual ~AbstractDeclaration() {}

    virtual Node *copy(Callback replacer) = 0;
};

class IdentifierDeclaration : public AbstractDeclaration {
public:
    std::string &name;

    virtual std::string toString(int indentation);

    virtual Node *copy(Callback replacer) {
        return replacer(new IdentifierDeclaration(name));
    }

    IdentifierDeclaration(std::string &name) : name(name) {}
};

class ArrayDeclaration : public AbstractDeclaration {
public:
    std::string &name;
    long long start;
    long long end;

    virtual std::string toString(int indentation);

    virtual Node *copy(Callback replacer) {
        return replacer(new ArrayDeclaration(name, start, end));
    }

    ArrayDeclaration(std::string &name, long long start, long long end) : name(name), start(start), end(end) {}
};

class DeclarationList : public Node {
public:
    std::vector<AbstractDeclaration *> declarations;

    virtual std::string toString(int indentation);

    virtual Node *copy(Callback replacer) {
        DeclarationList *dclList = new DeclarationList();

        for (auto const &dcl : declarations) {
            dclList->declarations.push_back(static_cast<AbstractDeclaration *>(dcl->copy(replacer)));
        }

        return dclList;
    }

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

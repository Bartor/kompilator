//
// Created by Bartor on 25.12.2019.
//

#include <vector>
#include <string>

#ifndef COMPILER_NODE_H
#define COMPILER_NODE_H

class Declaration;

class Command;

typedef std::vector<Command *> CommandList;
typedef std::vector<Declaration *> DeclarationList;

class Program {
public:
    DeclarationList declarations;
    CommandList commands;

    Program(DeclarationList &declarationList, CommandList &commandList) : declarations(declarationList),
                                                                          commands(commandList) {}
};

class Node {
public:
    virtual ~Node() {}
};

class Identifier : public Node {
public:
    std::string name;

    Identifier(const std::string &name) : name(name) {}
};

class Value : public Node {
public:
    long long value;

    Value(long long value) : value(value) {}
};

class Expression : public Node {
};

class ValueExpression : public Expression {
public:
    Value &value;

    ValueExpression(Value &value) : value(value) {}
};

class Addition : public Expression {
public:
    Value &lhs;
    Value &rhs;

    Addition(Value &lhs, Value &rhs) : lhs(lhs), rhs(rhs) {}
};

class Command : public Node {
};

class Assignment : public Command {
public:
    Identifier &identifier;
    Expression &expression;

    Assignment(Identifier &identifier, Expression &expression) : identifier(identifier), expression(expression) {}
};

#endif //COMPILER_NODE_H

#include "node.hpp"
#include <string>
#include <iostream>
#include <typeinfo>

/* ==== toString ==== */

std::string Node::indent(int indentation) {
    return std::string(indentation, ' ');
}

std::string VariableIdentifier::toString(int indentation) {
    std::cout << "var id: " << name << std::endl;
    return indent(indentation) + "Variable<" + name + ">\n";
}

std::string AccessIdentifier::toString(int indentation) {
    return indent(indentation) + "ArrayAccess<" + name + ">[" + std::to_string(index) + "]\n";
}

std::string VariableAccessIdentifier::toString(int indentation) {
    return indent(indentation) + "ArrayAccess<" + name + ">[\n" + variableIdentifier.toString(indentation + 1) +
           indent(indentation) + "\n";
}

std::string NumberValue::toString(int indentation) {
    return indent(indentation) + ("NumberValue<" + std::to_string(value) + ">\n");
}

std::string IdentifierValue::toString(int indentation) {
    return indent(indentation) + "IdentifierValue<\n" + identifier.toString(indentation + 1) + indent(indentation) +
           ">\n";
}

std::string UnaryExpression::toString(int indentation) {
    return indent(indentation) + "UnaryExpression<\n" + value.toString(indentation + 1) + indent(indentation) + ">\n";
}

std::string BinaryExpression::toString(int indentation) {
    return indent(indentation) + "BinaryExpression<\n" + lhs.toString(indentation + 1) + "," +
           rhs.toString(indentation + 1) + indent(indentation) + ">\n";
}

std::string Condition::toString(int indentation) {
    return indent(indentation) + "Condition<\n" + lhs.toString(indentation + 1) + "," + rhs.toString(indentation + 1) +
           indent(indentation) + ">\n";
}

std::string CommandList::toString(int indentation) {
    std::string s = indent(indentation) + ("CommandList<\n");
    for (int i = 0; i < commands.size(); i++) {
        s += commands[i]->toString(indentation + 1);
    }
    s += indent(indentation) + ">\n";
    return s;
}

std::string Assignment::toString(int indentation) {
    std::cout << "[{" << std::endl;
    std::cout << &identifier << std::endl;
    std::cout << "Class: " << typeid(identifier).name() << std::endl;
    std::cout << identifier.toString(0);
    std::cout << "}]" << std::endl;

    return indent(indentation) + "Assignment<\n" + identifier.toString(indentation + 1) +
           expression.toString(indentation + 1) + indent(indentation) + ">\n";
}

std::string If::toString(int indentation) {
    return indent(indentation) + "If<\n" + condition.toString(indentation + 1) + "," +
           commands.toString(indentation + 1) + indent(indentation) + ">\n";
}

std::string IfElse::toString(int indentation) {
    return indent(indentation) + "If<\n" + condition.toString(indentation + 1) + "," +
           commands.toString(indentation + 1) + "," + elseCommands.toString(indentation + 1) + indent(indentation) +
           ">\n";
}

std::string While::toString(int indentation) {
    return indent(indentation) + "While<\n" + condition.toString(indentation + 1) + "," +
           commands.toString(indentation + 1) + ">\n";
}

std::string For::toString(int indentation) {
    return indent(indentation) + "For<" + variableName + "," + startValue.toString(indentation + 1) +
           endValue.toString(indentation + 1) + commands.toString(indentation + 1) + indent(indentation) + ">\n";
}

std::string Read::toString(int indentation) {
    return indent(indentation + 1) + "Read<\n" + identifier.toString(indentation + 1) + indent(indentation) + ">\n";
}

std::string Write::toString(int indentation) {
    return indent(indentation + 1) + "Write<\n" + value.toString(indentation + 1) + indent(indentation) + ">\n";
}

std::string DeclarationList::toString(int indentation) {
    std::string s = indent(indentation) + ("DeclarationList<\n");
    for (const auto &declaration : declarations) s += declaration->toString(indentation + 1);
    s += indent(indentation) + ">\n";
    return s;
}

std::string IdentifierDeclaration::toString(int indentation) {
    return indent(indentation) + "IdentifierDeclaration<" + name + ">\n";
}

std::string ArrayDeclaration::toString(int indentation) {
    return indent(indentation) + "ArrayDeclaration<" + name + "," + std::to_string(start) + "," + std::to_string(end) +
           ">\n";
}

std::string Program::toString() {
    return "Program<\n" + declarations.toString(0) + "\n" + commands.toString(0) + ">";
}

/* ==== end of toString ==== */
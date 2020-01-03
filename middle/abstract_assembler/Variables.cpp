#include "Variables.h"

ResolvableAddress &Variable::getAddress() {
    return address;
}

std::string NumberVariable::toString() {
    return "NumberVariable<" + name + ">";
}

std::string NumberArrayVariable::toString() {
    return "NumberArrayVariable<" + name + ">[" + std::to_string(start) + ", " + std::to_string(end) + "]";
}
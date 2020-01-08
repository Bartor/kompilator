#include "Variables.h"

ResolvableAddress &Variable::getAddress() {
    return address;
}

std::string NumberVariable::toString() {
    return "NumberVariable<" + name + ">->" + std::to_string(getAddress().getAddress());
}

std::string NumberArrayVariable::toString() {
    return "NumberArrayVariable<" + name + ">[" + std::to_string(start) + ", " + std::to_string(end) + "]->" + std::to_string(getAddress().getAddress());
}

std::string TemporaryVariable::toString() {
    return "TemporaryVariable<" + name + ">->" + std::to_string(getAddress().getAddress());
}
#include "asm.h"

std::string Get::toAssemblyCode(bool pretty) {
    return "GET";
}

std::string Put::toAssemblyCode(bool pretty) {
    return "PUT";
}

std::string Halt::toAssemblyCode(bool pretty) {
    return "HALT";
}

std::string Inc::toAssemblyCode(bool pretty) {
    return "INC";
}

std::string Dec::toAssemblyCode(bool pretty) {
    return "DEC";
}

std::string Load::toAssemblyCode(bool pretty) {
    return "LOAD" + std::string(pretty ? " " : "") + std::to_string(address.getAddress());
}

std::string Store::toAssemblyCode(bool pretty) {
    return "STORE" + std::string(pretty ? " " : "") + std::to_string(address.getAddress());
}

std::string Loadi::toAssemblyCode(bool pretty) {
    return "LOADI" + std::string(pretty ? " " : "") + std::to_string(address.getAddress());
}

std::string Storei::toAssemblyCode(bool pretty) {
    return "STOREI" + std::string(pretty ? " " : "") + std::to_string(address.getAddress());
}

std::string Add::toAssemblyCode(bool pretty) {
    return "ADD" + std::string(pretty ? " " : "") + std::to_string(address.getAddress());
}

std::string Sub::toAssemblyCode(bool pretty) {
    return "SUB" + std::string(pretty ? " " : "") + std::to_string(address.getAddress());
}

std::string Shift::toAssemblyCode(bool pretty) {
    return "SHIFT" + std::string(pretty ? " " : "") + std::to_string(address.getAddress());
}

std::string Jump::toAssemblyCode(bool pretty) {
    return "JUMP" + std::string(pretty ? " " : "") + std::to_string(address.getAddress());
}

std::string Jpos::toAssemblyCode(bool pretty) {
    return "JPOS" + std::string(pretty ? " " : "") + std::to_string(address.getAddress());
}

std::string Jzero::toAssemblyCode(bool pretty) {
    return "JZERO" + std::string(pretty ? " " : "") + std::to_string(address.getAddress());
}

std::string Jneg::toAssemblyCode(bool pretty) {
    return "JNEG" + std::string(pretty ? " " : "") + std::to_string(address.getAddress());
}
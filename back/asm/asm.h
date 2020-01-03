#include "../../middle/abstract_assembler/Variables.h"
#include <string>
#include <vector>

#ifndef COMPILER_ASM_H
#define COMPILER_ASM_H

class Instruction {
public:
    virtual std::string toAssemblyCode(bool pretty = false) = 0;

    virtual ~Instruction() {}
};

class InstructionList {
public:
    std::vector<Instruction *> instructions;

    InstructionList &append(InstructionList &list);
};

class Get : public Instruction {
public:
    virtual std::string toAssemblyCode(bool pretty = false);
};

class Put : public Instruction {
public:
    virtual std::string toAssemblyCode(bool pretty = false);
};

class Halt : public Instruction {
public:
    virtual std::string toAssemblyCode(bool pretty = false);
};

class Inc : public Instruction {
public:
    virtual std::string toAssemblyCode(bool pretty = false);
};

class Dec : public Instruction {
public:
    virtual std::string toAssemblyCode(bool pretty = false);
};

class Load : public Instruction {
public:
    ResolvableAddress &address;

    virtual std::string toAssemblyCode(bool pretty = false);

    Load(ResolvableAddress &address) : address(address) {}
};

class Store : public Instruction {
public:
    ResolvableAddress &address;

    virtual std::string toAssemblyCode(bool pretty = false);

    Store(ResolvableAddress &address) : address(address) {}
};

class Loadi : public Instruction {
public:
    ResolvableAddress &address;

    virtual std::string toAssemblyCode(bool pretty = false);

    Loadi(ResolvableAddress &address) : address(address) {}
};

class Storei : public Instruction {
public:
    ResolvableAddress &address;

    virtual std::string toAssemblyCode(bool pretty = false);

    Storei(ResolvableAddress &address) : address(address) {}
};

class Add : public Instruction {
public:
    ResolvableAddress &address;

    virtual std::string toAssemblyCode(bool pretty = false);

    Add(ResolvableAddress &address) : address(address) {}
};

class Sub : public Instruction {
public:
    ResolvableAddress &address;

    virtual std::string toAssemblyCode(bool pretty = false);

    Sub(ResolvableAddress &address) : address(address) {}
};

class Shift : public Instruction {
public:
    ResolvableAddress &address;

    virtual std::string toAssemblyCode(bool pretty = false);

    Shift(ResolvableAddress &address) : address(address) {}
};

class Jump : public Instruction {
public:
    ResolvableAddress &address;

    virtual std::string toAssemblyCode(bool pretty = false);

    Jump(ResolvableAddress &address) : address(address) {}
};

class Jpos : public Instruction {
public:
    ResolvableAddress &address;

    virtual std::string toAssemblyCode(bool pretty = false);

    Jpos(ResolvableAddress &address) : address(address) {}
};

class Jzero : public Instruction {
public:
    ResolvableAddress &address;

    virtual std::string toAssemblyCode(bool pretty = false);

    Jzero(ResolvableAddress &address) : address(address) {}
};

class Jneg : public Instruction {
public:
    ResolvableAddress &address;

    virtual std::string toAssemblyCode(bool pretty = false);

    Jneg(ResolvableAddress &address) : address(address) {}
};

#endif //COMPILER_ASM_H

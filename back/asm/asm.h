#include "../../middle/abstract_assembler/Variables.h"
#include <string>
#include <vector>

#ifndef COMPILER_ASM_H
#define COMPILER_ASM_H

/**
 * An abstract class representing a single assembly instruction.
 * It has a protected address field which is set and get using
 * setters and getters; this address field indicated instruction's
 * place in the generated assmebly code, for example in
 * LOAD 1
 * ADD 2
 * STORE 1
 * LOAD has address of 0, ADD - 2 and STORE - 3.
 */
class Instruction {
protected:
    long long address = -1;
public:
    bool stub = false;

    virtual void setAddress(long long newAddress);

    virtual long long getAddress();

    virtual std::string toAssemblyCode(bool pretty = false) = 0;

    virtual ~Instruction() {}
};

/**
 * This is a special instruction which is basically a NOP. Its
 * address field should ALWAYS be set to the same address as the
 * next instruction in file. It should not be assembled, as it is
 * not a part of assembly language. For example
 * JUMP 3
 * LOAD 2
 * STORE 3
 * STUB
 * PUT
 * the STUB and PUT instructions has an compile-time address of 3
 * and should therefore be compiled down to
 * JUMP 3
 * LOAD 2
 * STORE 3
 * PUT
 */
class Stub : public Instruction {
public:
    virtual std::string toAssemblyCode(bool pretty = false);

    void setAddress(long long newAddress);

    long long getAddress();

    Stub() {
        stub = true;
    }
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

class InstructionUsingAddress : public Instruction {
public:
    ResolvableAddress &address;

    virtual ~InstructionUsingAddress() {}
    virtual std::string toAssemblyCode(bool pretty = false) = 0;

    InstructionUsingAddress(ResolvableAddress &address) : address(address) {}
};

class Load : public InstructionUsingAddress {
public:
    virtual std::string toAssemblyCode(bool pretty = false);

    Load(ResolvableAddress &address) : InstructionUsingAddress(address) {}
};

class Store : public InstructionUsingAddress {
public:
    virtual std::string toAssemblyCode(bool pretty = false);

    Store(ResolvableAddress &address) : InstructionUsingAddress(address) {}
};

class Loadi : public InstructionUsingAddress {
public:
    virtual std::string toAssemblyCode(bool pretty = false);

    Loadi(ResolvableAddress &address) : InstructionUsingAddress(address) {}
};

class Storei : public InstructionUsingAddress {
public:
    virtual std::string toAssemblyCode(bool pretty = false);

    Storei(ResolvableAddress &address) : InstructionUsingAddress(address) {}
};

class Add : public InstructionUsingAddress {
public:
    virtual std::string toAssemblyCode(bool pretty = false);

    Add(ResolvableAddress &address) : InstructionUsingAddress(address) {}
};

class Sub : public InstructionUsingAddress {
public:
    virtual std::string toAssemblyCode(bool pretty = false);

    Sub(ResolvableAddress &address) : InstructionUsingAddress(address) {}
};

class Shift : public InstructionUsingAddress {
public:
    virtual std::string toAssemblyCode(bool pretty = false);

    Shift(ResolvableAddress &address) : InstructionUsingAddress(address) {}
};

/**
 * A basic JUMP instruction with a Instruction* target field.
 * When assembled, it uses target's getAddress to fetch a jump
 * address and only then prints it as a number value. For every
 * other circumstance the target is just an abstract instruction
 * pointer and thus allows to freely add and remove instructions
 * before and after without needing to be offset.
 */
class Jump : public Instruction {
public:
    Instruction *target;

    virtual std::string toAssemblyCode(bool pretty = false);

    Jump(Instruction *target) : target(target) {}
};

class Jpos : public Jump {
public:
    virtual std::string toAssemblyCode(bool pretty = false);

    Jpos(Instruction *target) : Jump(target) {}
};

class Jzero : public Jump {
public:
    virtual std::string toAssemblyCode(bool pretty = false);

    Jzero(Instruction *target) : Jump(target) {}
};

class Jneg : public Jump {
public:
    virtual std::string toAssemblyCode(bool pretty = false);

    Jneg(Instruction *target) : Jump(target) {}
};

#endif //COMPILER_ASM_H

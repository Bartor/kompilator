#include "../../middle/abstract_assembler/Variables.h"
#include <string>
#include <vector>

#ifndef COMPILER_ASM_H
#define COMPILER_ASM_H

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

class Stub : public Instruction {
private:
    Stub *target;
    std::vector<Stub *> redirections;
public:
    virtual std::string toAssemblyCode(bool pretty = false);

    void redirect(Stub *newTarget);

    void setAddress(long long newAddress);

    long long getAddress();

    void registerRedirection(Stub *newRedirection);

    Stub() {
        target = this;
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

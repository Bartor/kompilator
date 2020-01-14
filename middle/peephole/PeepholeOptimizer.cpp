#include "PeepholeOptimizer.h"

void PeepholeOptimizer::optimize() {
    while (removeUselessStoreLoads()) {
        std::cout << "Removed useless STORE LOAD" << std::endl;
    }
    while (removeUselessStoreLoadis()) {
        std::cout << "Removed useless STORE LOADI" << std::endl;
    }
}

bool PeepholeOptimizer::removeUselessStoreLoads() {
    for (int i = 0; i < instructions.getInstructions().size(); i++) {
        if (auto storeInstruction = dynamic_cast<Store *>(instructions.getInstructions()[i])) {

            int offsetToNonStub = 0;
            while (dynamic_cast<Stub *>(instructions.getInstructions()[++offsetToNonStub + i])) {
                if (i + offsetToNonStub == instructions.getInstructions().size()) return false;
            }

            if (auto loadInstruction = dynamic_cast<Load *>(instructions.getInstructions()[i + offsetToNonStub])) {
                if (storeInstruction->address.getAddress() == loadInstruction->address.getAddress()) {

                    bool canRemove = true;
                    for (int j = 0; j < instructions.getInstructions().size(); j++) {
                        if (auto jumpInstruction = dynamic_cast<Jump *>(instructions.getInstructions()[j])) {
                            if (jumpInstruction->target == loadInstruction || jumpInstruction->target == storeInstruction) {
                                canRemove = false;
                                break;
                            }
                        }
                    }

                    if (canRemove) {
                        instructions.getInstructions().erase(instructions.getInstructions().begin() + i + offsetToNonStub);

                        return true;
                    }
                }
            }
        }
    }

    return false;
}

bool PeepholeOptimizer::removeUselessStoreLoadis() {
    for (int i = 0; i < instructions.getInstructions().size(); i++) {
        if (auto storeInstruction = dynamic_cast<Store *>(instructions.getInstructions()[i])) {

            int offsetToNonStub = 0;
            while (dynamic_cast<Stub *>(instructions.getInstructions()[++offsetToNonStub + i])) {
                if (i + offsetToNonStub == instructions.getInstructions().size()) return false;
            }

            if (auto loadiInstruction = dynamic_cast<Loadi *>(instructions.getInstructions()[i + offsetToNonStub])) {

                if (storeInstruction->address.getAddress() == loadiInstruction->address.getAddress()) {

                    bool canRemove = true;
                    for (int j = 0; j < instructions.getInstructions().size(); j++) {
                        if (auto jumpInstruction = dynamic_cast<Jump *>(instructions.getInstructions()[j])) {
                            if (jumpInstruction->target == loadiInstruction || jumpInstruction->target == storeInstruction) {
                                canRemove = false;
                                break;
                            }
                        }
                    }

                    if (canRemove) {
                        instructions.getInstructions().erase(instructions.getInstructions().begin() + i);
                        instructions.getInstructions()[i + offsetToNonStub - 1] = new Loadi(*new ResolvableAddress(0));

                        return true;
                    }
                }
            }
        }

    }

    return false;
}
#include "Constant.h"

std::string lltoa(unsigned long long val, int base) {
    static char buf[64] = {0};

    int i = 62;

    if (val == 0) return std::string("0");

    for (; val && i; --i, val /= base) {
        buf[i] = "0123456789abcdef"[val % base];
    }

    return std::string(&buf[i + 1]);
}

long long calculatePrefix(Constant *a, Constant *b) {
    long long prefixLength = 0;
    while (a->binaryString[prefixLength] && b->binaryString[prefixLength] && a->binaryString[prefixLength] == b->binaryString[prefixLength]) {
        prefixLength++;
    }
    return prefixLength;
}

Constant::Constant(long long value, ResolvableAddress &address) : value(value), address(address) {
    long long valCopy = llabs(value);
    binaryString = std::string(lltoa(valCopy, 2));

    generationCost = 0;
    for (long long i = 0; i < binaryString.size(); i++) {
        if (binaryString[i] == '1') generationCost++;
        if (i != binaryString.size() - 1) generationCost += 5;
    }
}

InstructionList &Constant::generateConstant(Constants *constantsData, ResolvableAddress &primaryAccumulator, ResolvableAddress &secondaryAccumulator) {
    InstructionList &instructions = *new InstructionList();

    if (value == 0) {
        instructions.append(new Sub(primaryAccumulator))
                .append(new Store(address));

        return instructions;
    } // 1 and -1 are covered separately

    long long bestPrefix = 0;
    Constant *bestFit = nullptr;

    Constant *closest = nullptr;
    for (auto const &constant : constantsData->getConstants()) {
        if (constant == this) break;
        if (llabs(constant->value) > llabs(value)) break;

        if ((value > 0 && constant->value > 0) || (value < 0 && constant->value < 0)) {
            if (llabs(value) > llabs(constant->value)) {
                if (closest) {
                    if (llabs(value - constant->value) < llabs(value - closest->value)) {
                        closest = constant;
                    }
                } else {
                    closest = constant;
                }
            }
        }

        long long prefix = calculatePrefix(this, constant);
        if (prefix > bestPrefix || (prefix == bestPrefix && constant->value > 0)) {
            bestPrefix = prefix;
            bestFit = constant;
        }
    }

    if (closest && llabs(value - closest->value) < 10) {
        instructions.append(new Load(closest->getAddress()));

        for (long long i = 0; i < llabs(value - closest->value); i++) {
            if (value < 0) {
                instructions.append(new Dec());
            } else {
                instructions.append(new Inc());
            }
        }

        instructions.append(new Store(address));

        return instructions;
    }

    bool byBestFit = false;
    if (bestFit) {
        if (bestPrefix == binaryString.size()) { // the number is actually the same, just with different sign
            byBestFit = true;

            instructions.append(new Sub(primaryAccumulator))
                    .append(new Sub(bestFit->address))
                    .append(new Store(address));
        } else { // numbers differ by last (binaryString.length - bestPrefix) bits
            long long backShifts = bestFit->binaryString.size() - bestPrefix;
            long long cost = (backShifts - 1) * 5 + bestFit->value < 0 ? 20 : 10; // load + shift [-1]s cost
            for (long long i = bestPrefix; i < binaryString.size(); i++) {
                if (binaryString[i] == '1') cost++;
                if (i != binaryString.size() - 1) cost += 5;
            }

            if (cost < generationCost) {
                byBestFit = true;

                if (bestFit->value > 0) {
                    instructions.append(new Load(bestFit->address));
                } else {
                    bestFit->value < 0;
                    instructions.append(new Sub(primaryAccumulator))
                            .append(new Sub(bestFit->getAddress()));
                }

                for (long long i = 0; i < backShifts; i++) instructions.append(new Shift(constantsData->getConstant(-1)->getAddress()));
                instructions.append(new Shift(constantsData->getConstant(1)->getAddress()));
                for (long long i = bestPrefix; i < binaryString.size(); i++) {
                    if (binaryString[i] == '1') instructions.append(new Inc());
                    if (i != binaryString.size() - 1) instructions.append(new Shift(constantsData->getConstant(1)->getAddress()));
                }
                if (value < 0) {
                    instructions.append(new Store(secondaryAccumulator))
                            .append(new Sub(secondaryAccumulator))
                            .append(new Sub(secondaryAccumulator));
                }
                instructions.append(new Store(address));
            } else {
                byBestFit = false;
            }
        }
    }


    if (!byBestFit) {
        instructions.append(new Sub(primaryAccumulator));
        for (long long i = 0; i < binaryString.size(); i++) {
            if (binaryString[i] == '1')instructions.append(new Inc());
            if (i != binaryString.size() - 1) instructions.append(new Shift(constantsData->getConstant(1)->getAddress()));
        }
        if (value < 0)
            instructions.append(new Store(secondaryAccumulator))
                    .append(new Sub(secondaryAccumulator))
                    .append(new Sub(secondaryAccumulator));
        instructions.append(new Store(address));
    }

    return instructions;
}

ResolvableAddress &Constant::getAddress() {
    return address;
}

std::string Constant::toString() {
    return "Constant<" + std::to_string(value) + ">";
}
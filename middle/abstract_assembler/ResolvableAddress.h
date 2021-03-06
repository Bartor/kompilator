#ifndef COMPILER_RESOLVABLEADDRESS_H
#define COMPILER_RESOLVABLEADDRESS_H

/**
 * Represents an address in VM's memory; allows for multiple
 * instruction to reference a single ResolvableAddress when then
 * can be moved or modified without needing to modify those
 * instructions.
 */
class ResolvableAddress {
private:
    long long offset;
    long long address;
public:
    ResolvableAddress(long long startingAddress = 0) : address(startingAddress), offset(0) {}

    long long getAddress();

    void setAddress(long long newAddress);

    void setOffset(long long newOffset);
};


#endif //COMPILER_RESOLVABLEADDRESS_H

#ifndef COMPILER_RESOLVABLEADDRESS_H
#define COMPILER_RESOLVABLEADDRESS_H

class ResolvableAddress {
private:
    long long selfOffset;
    long long address;
    bool offset;
public:
    ResolvableAddress(long long startingAddress = 0) : address(startingAddress), offset(false) {}

    long long getAddress();

    void setAddress(long long newAddress);

    void setOffset(long long offset);
};


#endif //COMPILER_RESOLVABLEADDRESS_H

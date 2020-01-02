#include "ResolvableAddress.h"

void ResolvableAddress::setAddress(long long newAddress) {
    address = newAddress;
}

void ResolvableAddress::setOffset(long long offset) {
    selfOffset = offset;
}

long long ResolvableAddress::getAddress() {
    return offset ? selfOffset : address;
}

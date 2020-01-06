#include "ResolvableAddress.h"

void ResolvableAddress::setAddress(long long newAddress) {
    address = newAddress;
}

void ResolvableAddress::setOffset(long long newOffset) {
    offset = newOffset;
}

long long ResolvableAddress::getAddress() {
    return address + offset;
}

#include "HDD.h"
#include <iostream>
#include <bitset>

HDD::HDD(long _size) {
    data = new char[_size];
    size = _size;
    index = 0;
    debug = false;
}

HDD::~HDD() {
	delete data;
}

void HDD::write(char b) {
    if (debug) std::cout << "writing " << (int)b << " aka " << std::bitset<8>(b) << " to " << index << std::endl;
    data[index++] = b;
}

char HDD::read() {
    if (debug) std::cout << "reading " << (int)data[index] << " aka " << std::bitset<8>(data[index]) << " from " << index << std::endl;
    return data[index++];
}

long HDD::remaining() {
    return size - index;
}

void HDD::reset() {
    index = 0;
}

void HDD::seek(long _index) {
    index = _index;
}

long HDD::current() {
    return index;
}

long HDD::capacity() {
    return size;
}

void HDD::setDebug(bool _debug) {
	debug = _debug;
}

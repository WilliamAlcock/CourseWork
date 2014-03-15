#pragma once
#include <stdlib.h>

typedef unsigned int uint;
typedef unsigned long ulong;

class HDD {
private:
    char* data;
    long size;
    long index;
    bool debug;
public:
    HDD(long _size);
    ~HDD();

    void write(char b);
    char read();
    long remaining();
    void reset();
    void seek(long index);
    long current();
    long capacity();
    void setDebug(bool debug);
};

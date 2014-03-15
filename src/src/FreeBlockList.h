#pragma once
#include "HDD.h"

const int freeBlockHeaderSize = 8;

class FreeBlockList {
public:
    FreeBlockList(HDD* _hdd, int _numberOfBlocks, int _blockSize, int _startingPosition);
    ~FreeBlockList(){}
    void read();						// Tested
    void write();						// Tested
    int getBlock();						// Tested
    void freeBlock(int blockNumber);	// Tested
    // getters
    int getNumberOfFreeBlocks();		// Tested
private:
	// from input
	HDD* hdd;
    int numberOfBlocksOnDisk;
    int blockSize;
    int startingPosition;
    // counters
    int firstFreeBlock;		// saved in header
    int freeBlockCounter;	// saved in header
    bool initialised;
    void writeFirstFreeBlock();						// Tested
    void writeFreeBlockCounter();					// Tested
};

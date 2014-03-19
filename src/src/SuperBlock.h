#pragma once
#include "HDD.h"
#include <stdlib.h>

const int superBlockSize = 24;
struct SuperBlockStruct {
	unsigned int numberOfBlocks;		// Number of blocks
	unsigned int blockSize;				// Block size
	unsigned int freeBlockListSizeBytes;
	unsigned int systemBlocks;
	unsigned int iNodeListBytes;
	unsigned int numberOfINodes;
};
union SuperBlockData {
	SuperBlockStruct data;
	char c[superBlockSize];
};

class SuperBlock {
public:
	SuperBlock(HDD* _hdd);
	~SuperBlock() {}
	void write(int blockSize);					// Tested
	void read();								// Tested
	// getters
	int getNumberOfBlocks() const;				// Tested
	int getBlockSize() const;					// Tested
	int getFreeBlockStart() const;				// Tested
	int getINodeStart() const;					// Tested
	int getNumberOfINodes() const;				// Tested
	int getFirstBlockStart() const;				// Tested
	int getStartOfBlock(int blockNumber);		// Tested
private:
	// from input
	HDD* hdd;
	// calculated
	SuperBlockData data;	// saved in header
	// counters
	bool initialised;
};

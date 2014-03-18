#pragma once
#include "HDD.h"
#include "SuperBlock.h"
#include <stdlib.h>
#include <string>

using namespace std;

class UnitTest {
private:
	int readInt(HDD* hdd);
	int readInt(HDD* hdd, int index);
	void testSuperBlockGetters(SuperBlock* superBlock, int numberOfBlocks, int systemBlocks, int blockSize, int freeBlockListSizeBytes, int iNodeListBytes, int finalINodeListBytes, int numberOfINodes);
public:
	UnitTest() {};
	~UnitTest() {};
	void runFreeBlockListTest();
	void runSuperBlockTest();
	void runINodeListTest();
	void runDirectoryTest();
	string createRandomFile();
};

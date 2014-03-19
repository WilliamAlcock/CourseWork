#pragma once
#include "FreeBlockList.h"
#include "SuperBlock.h"
#include "HDD.h"
#include <set>

using namespace std;

const int iNodeSize = 85;
const int iNodeHeaderSize = 12;
const int iNodeFooterSize = 4;	// unused  - will be used for jump address
const int iNodeReferenceBlockHeaderSize = 4;
const int iNodeReferenceBlockEntrySize = 4;
struct iNodeStruct {
	long size;
	long creationTime;			// unused
	long modificationTime;		// unused
	long accessTime;			// unused
	int count;
	int block[11];
	int numberOfBlocks;
	unsigned int used : 1;
	unsigned int directory : 1;
	unsigned int spare3 : 1;	// unused
	unsigned int spare4 : 1;	// unused
	unsigned int spare5 : 1;	// unused
	unsigned int spare6 : 1;	// unused
	unsigned int spare7 : 1;	// unused
	unsigned int spare8 : 1;	// unused
};
union iNodeData {
	iNodeStruct data;
	char c[iNodeSize];
};

class INodeList {
public:
    INodeList(HDD* _hdd, SuperBlock* superBlock, FreeBlockList* freeBlockList);
    ~INodeList(){};
    void read();
    void write(int rootBlock);
    int getNumberOfFreeINodes();
    int writeNewINode(bool directory, long size, int firstBlock);			// Tested
    void removeLinkFromINode(int iNodeNumber);								// Tested
    void addLinkToINode(int iNodeNumber);									// Tested
    // INode setters
    void removeBlockFromINode(int iNodeNumber);								// Tested
    void addBlockToINode(int iNodeNumber, int block);						// Tested
    void setINodeSize(int iNodeNumber, long newSize);						// Tested
    void lockINode(int iNodeNumber);										// Tested
    void unLockINode(int iNodeNumber);										// Tested
    // INode getters
    bool isINodeDirectory(int iNodeNumber);									// Tested
    bool isINodeLocked(int iNodeNumber);									// Tested
    long getINodeSize(int iNodeNumber);										// Tested
    long getINodeCreationTime(int iNodeNumber);								// Not Implemented
    long getINodeModificationTime(int iNodeNumber);							// Not Implemented
    long getINodeAccessTime(int iNodeNumber);								// Not Implemented
    int getNumberOfBlocks(int iNodeNumber);									// Tested
    int getBlockNumber(int iNodeNumber, int blockNumber);					// Tested
private:
	// from input
	HDD* hdd;
	FreeBlockList* freeBlockList;
    int startingPosition;
    int numberOfINodes;
    SuperBlock* superBlock;

    // iNodeList
    set <int> lockedNodes;
    // counters
    int firstFreeINode;			// saved in header
    int freeINodeCounter;		// saved in header
    int lastUsedINode;			// saved in header
    bool initialised;
    void writeFirstFreeINode();												// Tested
    void writeFreeINodeCounter();											// Tested
    void writeLastUsedINode();												// Tested
    void writeINode(int iNodeNumber, iNodeData);							// Tested
    iNodeData readINode(int iNodeNumber);									// Tested
    bool isValid(int iNodeNumber);											// Tested
    // INode reference blocks
    int readReferenceBlockHeader(int blockNumber);										// Tested
    void writeReferenceBlockHeader(int blockNumber, int header);						// Tested
    int readReferenceBlockEntry(int blockNumber, int referenceNumber);					// Tested
    void writeReferenceBlockEntry(int blockNumber, int referenceNumber, int entry);		// Tested
};

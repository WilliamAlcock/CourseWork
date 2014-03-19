#pragma once
#include "SuperBlock.h"
#include "FreeBlockList.h"
#include "INodeList.h"
#include <iostream>
#include <map>

using namespace std;

const int directoryEntrySize = 8;
const int directoryHeaderSize = 8;
struct DirectoryHeaderStruct {
	int endOfLastName;
	int startOfLastNode;
};
union DirectoryHeaderData {
	DirectoryHeaderStruct data;
	char c[directoryHeaderSize];
};
struct DirectoryEntryStruct {
	int iNodeRef;
	int nameOffset;
};
union DirectoryEntryData {
	DirectoryEntryStruct data;
	char c[directoryEntrySize];
};

class CachedDirectory {
public:
	CachedDirectory(int iNodeNumber, HDD* hdd, SuperBlock* superBlock, FreeBlockList* freeBlockList, INodeList* iNodeList);
	~CachedDirectory();
	map <std::string, int> cache;
	// write operations
	void writeCachedDirectory();																	// Tested
	void writeRootNode();																			// Tested
	void readCacheDirectory();																		// Tested
	int getINodeNumber();
	//
	void writeDirectoryHeader(int blockNumber, DirectoryHeaderData directoryHeader);				// Tested
	//
	int count();
private:
	// from input
	int iNodeNumber;
	HDD* hdd;
	SuperBlock* superBlock;
	FreeBlockList* freeBlockList;
	INodeList* iNodeList;
	// cached directory
	DirectoryHeaderData cachedDirectoryLastBlockHeader;
	// write operations
	void writeDirectoryEntryData(DirectoryEntryData entry, int blockNumber, int lastNodePointer);	// Tested
	void writeDirectoryEntryName(string name, int blockNumber, int startPoiner);					// Tested
	// cache operations
	void cacheDirectoryBlock(int blockNumber);														// Tested
	DirectoryHeaderData readDirectoryHeader(int blockNumber);										// Tested
	DirectoryEntryData readDirectoryEntry(int blockNumber, int entryNumber);						// Tested
	string readDirectoryEntryNames(int blockNumber, int numberOfEntries, int endOfLastName);		// Tested
};

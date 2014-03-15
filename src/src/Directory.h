#pragma once
#include "SuperBlock.h"
#include "FreeBlockList.h"
#include "INodeList.h"
#include "DirectoryNode.h"
#include "DirectoryTree.h"
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
struct TreeQueuePair {
	int iNodeRef;
	string path;
};

class Directory {
public:
	Directory(HDD* _hdd, const SuperBlock* _superBlock, FreeBlockList* _freeBlockList, INodeList* _iNodeList, int _rootINode);
	~Directory();
    void read();
    void write();
    // file operations
    void createFile();			// Needs Implementing
    void deleteFile();			// Needs Implementing
    void renameFile();			// Needs Implementing
    void moveFile();			// Needs Implementing
    void copyFile();			// Needs Implementing
    void makeHardLinkToFile();	// Needs Implementing
    //
    void readFile();			// Needs Implementing
    void openFile();			// Needs Implementing
    void closeFile();			// Needs Implementing
    // directory operations
    bool deleteDir(string path, string name);												// Tested
    bool createDir(string path, string name);												// Tested
    bool moveDir(string fromPath, string fromName, string toPath, string toName);			// Tested
    bool makeHardLinkDir(string fromPath, string fromName, string toPath, string toName);	// Tested
    bool copyDir();				// Needs Implementing
private:
	// from input
	HDD* hdd;
	const SuperBlock* superBlock;
	FreeBlockList* freeBlockList;
	INodeList* iNodeList;
	int rootINode;
	// counters
	bool initialised;
	bool busy;
	// cached directory
	DirectoryHeaderData cachedDirectoryLastBlockHeader;
	int cachedDirectoryNode = -1;
    map <std::string, int> cachedDirectory;
    // cached directory tree
    DirectoryTree* directoryTree;
    // write operations
    void writeCachedDirectory();																	// Tested
    void writeDirectoryEntryData(DirectoryEntryData entry, int blockNumber, int lastNodePointer);	// Tested
    void writeDirectoryEntryName(string name, int blockNumber, int startPoiner);					// Tested
    void writeDirectoryHeader(int blockNumber, DirectoryHeaderData directoryHeader);				// Tested
    // cache operations
	void resetCache();																				// Tested
	void cacheDirectory(int iNodeNumber);															// Tested
	void cacheDirectoryBlock(int blockNumber);														// Tested
    void cacheDirectoryTree();
	// read operations
	DirectoryHeaderData readDirectoryHeader(int blockNumber);										// Tested
	DirectoryEntryData readDirectoryEntry(int blockNumber, int entryNumber);						// Tested
	string readDirectoryEntryNames(int blockNumber, int numberOfEntries, int endOfLastName);		// Tested
	// getters
	int getStartOfBlock(int blockNumber);															// Tested
	// string operations
	bool isNameLegal(string name);																	// Tested
	bool isPathNameLegal(string name);																// Tested
};

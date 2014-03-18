#pragma once
#include "SuperBlock.h"
#include "FreeBlockList.h"
#include "INodeList.h"
#include "DirectoryNode.h"
#include "DirectoryTree.h"
#include <iostream>
#include <map>

using namespace std;

const int fileHeaderSize = 4;
union FileHeaderData {
	int lengthOfBlock;
	char c[4];
};

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

class Directory {
public:
	Directory(HDD* _hdd, const SuperBlock* _superBlock, FreeBlockList* _freeBlockList, INodeList* _iNodeList, int _rootINode);
	~Directory();
    void read();
    void write();
    // file operations
    bool createFile(string path, string name, string &data);									// Tested
    bool rewriteFile(string path, string name, string &data);									// Tested
    bool deleteFile(string path, string name);													// Tested
    bool renameFile(string fromPath, string fromName, string toName);							// Tested
    //
    bool moveFile(string fromPath, string fromName, string toPath, string toName);				// Tested
    bool copyFile(string fromPath, string fromName, string toPath, string toName);				// Tested
    bool makeHardLinkToFile(string fromPath, string fromName, string toPath, string toName);	// Tested
    //
    bool readFile(string path, string name, string &data);										// Tested
    bool openFile(string path, string name, string &data);										// Needs Testing
    bool closeFile(string path, string name);													// Needs Testing
    // directory operations
    string listDirContents(string path);														// Tested
    bool deleteDir(string path, string name);													// Tested	add force flag
    bool createDir(string path, string name);													// Tested
    bool moveDir(string fromPath, string fromName, string toPath, string toName);				// Tested
    bool makeHardLinkDir(string fromPath, string fromName, string toPath, string toName);		// Tested
    bool copyDir();																				// Needs Implementing

private:
	// from input
	HDD* hdd;
	const SuperBlock* superBlock;
	FreeBlockList* freeBlockList;
	INodeList* iNodeList;
	int rootINode;
	// counters
	bool initialised;
	// cached directory
	DirectoryHeaderData cachedDirectoryLastBlockHeader;
	int cachedDirectoryNode = -1;
    map <std::string, int> cachedDirectory;
    // cached directory tree
    DirectoryTree* directoryTree;
    // FILE
    // write operations
    bool writeFile(int iNodeNumber, string &data);													// Tested
    void writeFileBlock(int blockNumber, string &data, int startPointer, int endPointer);			// Tested
    void writeFileBlockHeader(int blockNumber, FileHeaderData fileHeaderData);						// Tested
    // read operations
    void readFileBlock(int blockNumber, string &data);												// Tested
    FileHeaderData readFileBlockHeader(int blockNumber);											// Tested
    unsigned int getDataInBlock();																	// Tested
    // DIRECTORY
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
	bool isNameLegal(string &name);																	// Tested
	bool isPathNameLegal(string &name);																// Tested
};

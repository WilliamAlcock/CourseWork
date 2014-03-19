#pragma once
#include "SuperBlock.h"
#include "FreeBlockList.h"
#include "INodeList.h"
#include "DirectoryNode.h"
#include "DirectoryTree.h"
#include "CachedDirectory.h"
#include "bitFunctions.cpp"
#include <iostream>
#include <map>

using namespace std;

const int numberOfCachedDirectorys = 8;
const int fileHeaderSize = 4;
union FileHeaderData {
	int lengthOfBlock;
	char c[4];
};

class Directory {
public:
	Directory(HDD* _hdd, SuperBlock* _superBlock, FreeBlockList* _freeBlockList, INodeList* _iNodeList, int _rootINode);
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
    bool makeHardLinkToFile(string fromPath, string fromName, string toPath, string toName);	// Tested
    //
    bool readFile(string path, string name, string &data);										// Tested
    bool openFile(string path, string name, string &data);										// Tested
    bool closeFile(string path, string name);													// Tested
    // directory operations
    string listDirContents(string path);														// Tested
    bool deleteDir(string path, string name);													// Tested	needs force flag
    bool createDir(string path, string name);													// Tested
    bool moveDir(string fromPath, string fromName, string toPath, string toName);				// Tested
    bool makeHardLinkDir(string fromPath, string fromName, string toPath, string toName);		// Tested
    bool copyDir();																				// Needs Implementing

private:
	// from input
	HDD* hdd;
	SuperBlock* superBlock;
	FreeBlockList* freeBlockList;
	INodeList* iNodeList;
	int rootINode;
	// counters
	bool initialised;
	// cached directories
	int freeCaches;
	int lastUsed;
	int oldest;
	bool cachedDirsUsed[numberOfCachedDirectorys];
	CachedDirectory* cachedDirectory[numberOfCachedDirectorys];
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
    int getCachedDirectory(int iNodeNumber);
    void freeCachedDirectory(int i);
    void cacheDirectoryTree();																		// Tested
	// string operations
	bool isNameLegal(string &name);																	// Tested
	bool isPathNameLegal(string &name);																// Tested
};

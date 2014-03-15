#include "Directory.h"
#include "DirectoryNode.h"
#include "DirectoryTree.h"
#include "SuperBlock.h"
#include "FreeBlockList.h"
#include "INodeList.h"
#include "bitFunctions.cpp"

#include <iostream>
#include <stdexcept>
#include <sstream>
#include <queue>

Directory::Directory(HDD* _hdd, const SuperBlock* _superBlock, FreeBlockList* _freeBlockList, INodeList* _iNodeList, int _rootINode) {
	// from input
	hdd = _hdd;
	superBlock = _superBlock;
	freeBlockList = _freeBlockList;
	iNodeList = _iNodeList;
	rootINode = _rootINode;
	directoryTree = new DirectoryTree(rootINode);
	// cached directory flags
	cachedDirectoryNode = 0;
	initialised = false;
	busy = false;
}

Directory::~Directory() {
	delete directoryTree;
}

/*
 * Reads and caches the root block
 */
void Directory::read() {
	cacheDirectoryTree();
	initialised = true;
}

/*
 * Writes a blank root block
 */
void Directory::write() {
	// writes a new header for the root node
	DirectoryHeaderData directoryHeader;
	directoryHeader.data.endOfLastName = directoryHeaderSize;
	directoryHeader.data.startOfLastNode = superBlock->getBlockSize();
	writeDirectoryHeader(iNodeList->getBlockNumber(rootINode,0), directoryHeader);
	initialised = true;
}

void Directory::createFile() {
	if (!initialised) throw std::runtime_error("Directory not Initialised");
	while (busy) {}				// Wait until other operations have finished
	busy = true;

	// code goes here

	busy = false;
}

void Directory::deleteFile() {
	if (!initialised) throw std::runtime_error("Directory not Initialised");
	while (busy) {}				// Wait until other operations have finished
	busy = true;

	// code goes here

	busy = false;
}

void Directory::renameFile() {
	if (!initialised) throw std::runtime_error("Directory not Initialised");
	while (busy) {}				// Wait until other operations have finished
	busy = true;

	// code goes here

	busy = false;
}

void Directory::moveFile() {
	if (!initialised) throw std::runtime_error("Directory not Initialised");
	while (busy) {}				// Wait until other operations have finished
	busy = true;

	// code goes here

	busy = false;
}

void Directory::readFile() {
	if (!initialised) throw std::runtime_error("Directory not Initialised");
	while (busy) {}				// Wait until other operations have finished
	busy = true;

	// code goes here

	busy = false;
}

void Directory::openFile() {
	if (!initialised) throw std::runtime_error("Directory not Initialised");
	while (busy) {}				// Wait until other operations have finished
	busy = true;

	// code goes here

	busy = false;
}

void Directory::closeFile() {
	if (!initialised) throw std::runtime_error("Directory not Initialised");
	while (busy) {}				// Wait until other operations have finished
	busy = true;

	// code goes here

	busy = false;
}

// ********************** Directory operations **********************

bool Directory::deleteDir(string path, string name) {
	if (!initialised) throw std::runtime_error("Directory not Initialised");
	if (isPathNameLegal(path) && isNameLegal(name)) {
		while (busy) {}				// Wait until other operations have finished
		busy = true;

		// check the directory (path + name) exists (directoryTree)
		int directoryNode = directoryTree->getPathNode(path + name + "/");
		if (directoryNode < 0) {
			cout << "path is invalid" << endl;
			busy = false;
			return false;
		}
		// cache the directory if it is not already cached
		if (directoryNode != cachedDirectoryNode) cacheDirectory(directoryNode);
		// check the directory is empty
		if (cachedDirectory.size()>0) {
			cout << "directory is not empty" << endl;
			busy = false;
			return false;
		}
		int parentDirectoryNode = directoryTree->getPathNode(path);
		cacheDirectory(parentDirectoryNode);
		// remove the directory from the cache
		if (cachedDirectory.count(name) == 0) throw runtime_error("cached directory and directory tree out of sync");
		cachedDirectory.erase(name);
		// write the cache
		writeCachedDirectory();
		// remove the link from the directory's iNodeList
		iNodeList->removeLinkFromINode(directoryNode);
		// remove the directory from the directory Tree
		if (!directoryTree->deleteDirectory(path,name)) {
			throw std::runtime_error("Error deleting directory from tree");
		}

		busy = false;

		return true;
	} else {
		cout << "Path or filename not legal" << endl;
		return false;
	}
}

bool Directory::createDir(string path, string name) {
	if (!initialised) throw std::runtime_error("Directory not Initialised");
	if (isPathNameLegal(path) && isNameLegal(name)) {
		while (busy) {}				// Wait until other operations have finished
		busy = true;

		int parentDirectoryNode = directoryTree->getPathNode(path);
		// check the path is valid
		if (parentDirectoryNode < 0) {
			cout << "path is invalid" << endl;
			busy = false;
			return false;
		}
		// cache the parent directory (path) if it is not already cached
		if (parentDirectoryNode != cachedDirectoryNode) cacheDirectory(parentDirectoryNode);
		// check the name is not already in use
		if (cachedDirectory.count(name) > 0) {
			cout << "file/Directory name already used" << endl;
			busy = false;
			return false;
		}
		// write a new INode if this fails, busy = false, return false
		int directoryBlock = freeBlockList->getBlock();

		int directoryNode = iNodeList->writeNewINode(true,0,directoryBlock);

		// write the header for the directory block
		DirectoryHeaderData directoryHeader;
		directoryHeader.data.endOfLastName = directoryHeaderSize;
		directoryHeader.data.startOfLastNode = superBlock->getBlockSize();
		writeDirectoryHeader(directoryBlock, directoryHeader);
		// add the directory to the cache
		cachedDirectory.insert({name,directoryNode});
		// write the cache
		writeCachedDirectory();
		// add the directory to the directoryTree
		directoryTree->createDirectory(path,name,directoryNode);

		busy = false;

		return true;
	} else {
		cout << "Path or filename not legal" << endl;
		return false;
	}
}

bool Directory::moveDir(string fromPath, string fromName, string toPath, string toName) {
	if (!initialised) throw std::runtime_error("Directory not Initialised");
	if (isPathNameLegal(fromPath) && isNameLegal(fromName) && isPathNameLegal(toPath) && isNameLegal(toName)) {
		while (busy) {}				// Wait until other operations have finished
		busy = true;

		int toDir = directoryTree->getPathNode(toPath);
		int fromDir = directoryTree->getPathNode(fromPath);
		int toMoveDir = directoryTree->getPathNode(fromPath+fromName+"/");
		// check the fromPath and fromName exist
		if (toMoveDir < 0) {
			cout << fromPath << fromName << "/" << " does not exist" << endl;
			busy = false;
			return false;
		}
		// check the toPath exists
		if (toDir < 0) {
			cout << toDir << " does not exist" << endl;
			busy = false;
			return false;
		}

		// cache the to directory (path) if it is not already cached
		if (toDir != cachedDirectoryNode) cacheDirectory(toDir);
		// check the toName does not already exist
		if (cachedDirectory.count(toName) > 0) {
			cout << toPath << "/" << toDir << " already exists" << endl;
			busy = false;
			return false;
		}
		// add the directory to the cached directory
		cachedDirectory.insert({toName,toMoveDir});
		// write the directory cache
		writeCachedDirectory();
		// cache the from path
		cacheDirectory(fromDir);
		// remove the directory from the cache
		cachedDirectory.erase(fromName);
		// write the cached Directory
		writeCachedDirectory();
		// update the directory tree
		directoryTree->moveDirectory(fromPath,fromName,toPath,toName);

		busy = false;

		return true;
	} else {
		cout << "Path or filename not legal" << endl;
		return false;
	}
}

bool Directory::makeHardLinkDir(string fromPath, string fromName, string toPath, string toName) {
	if (!initialised) throw std::runtime_error("Directory not Initialised");
	if (isPathNameLegal(fromPath) && isNameLegal(fromName) && isPathNameLegal(toPath) && isNameLegal(toName)) {
		while (busy) {}				// Wait until other operations have finished
		busy = true;

		int toDir = directoryTree->getPathNode(toPath);
		int toCopyDir = directoryTree->getPathNode(fromPath+fromName+"/");
		// check the fromPath and fromName exist
		if (toCopyDir < 0) {
			cout << fromPath << fromName << "/" << " does not exist" << endl;
			busy = false;
			return false;
		}
		// check the toPath exists
		if (toDir < 0) {
			cout << toDir << " does not exist" << endl;
			busy = false;
			return false;
		}

		// cache the to directory (path) if it is not already cached
		if (toDir != cachedDirectoryNode) cacheDirectory(toDir);
		// check the toName does not already exist
		if (cachedDirectory.count(toName) > 0) {
			cout << toPath << "/" << toDir << " already exists" << endl;
			busy = false;
			return false;
		}
		// add the directory to the cached directory
		cachedDirectory.insert({toName,toCopyDir});
		// write the directory cache
		writeCachedDirectory();
		// update the directory tree
		directoryTree->copyDirectory(fromPath,fromName,toPath,toName);
		// add a link to the directorys iNode
		iNodeList->addLinkToINode(toCopyDir);

		busy = false;

		return true;
	} else {
		cout << "Path or filename not legal" << endl;
		return false;
	}
}

void Directory::writeCachedDirectory() {
	if (!initialised) throw std::runtime_error("Directory not Initialised");

	// get the number of nodes the directory currently has allocated (must be at least 1)
	int currentNumberOfBlocks = iNodeList->getNumberOfBlocks(cachedDirectoryNode);
	if (currentNumberOfBlocks<1) throw runtime_error("cached Directory Node has 0 blocks !");
	// point to the first block number
	int currentNodeBlockNumber = 0;
	int currentDiskBlockNumber = iNodeList->getBlockNumber(cachedDirectoryNode,currentNodeBlockNumber);
	// the amount of space left in the current block
	int blockSizeRemaining = superBlock->getBlockSize() - directoryHeaderSize;
	// current entry size
	int entrySize = 0;
	// current block header
	DirectoryHeaderData directoryHeaderData;
	directoryHeaderData.data.endOfLastName = directoryHeaderSize;
	directoryHeaderData.data.startOfLastNode = superBlock->getBlockSize();

	for (const auto &pair : cachedDirectory) {
		entrySize = (pair.first.length() + 1 + directoryEntrySize);
		if (blockSizeRemaining>entrySize) {
//			cout << "adding entry to existing block" << endl;
			blockSizeRemaining -= entrySize;
		} else {
//			cout << "adding entry to next block" << endl;
			// write the header
			writeDirectoryHeader(currentDiskBlockNumber,directoryHeaderData);
			// reset the counters
			blockSizeRemaining = superBlock->getBlockSize() - directoryHeaderSize;
			directoryHeaderData.data.endOfLastName = directoryHeaderSize;
			directoryHeaderData.data.startOfLastNode = superBlock->getBlockSize();
			currentNodeBlockNumber++;
			// allocate a new block to the iNode if needed
			if (currentNodeBlockNumber == currentNumberOfBlocks) {
//				cout << "adding new block" << endl;
				currentNumberOfBlocks++;
				currentDiskBlockNumber = freeBlockList->getBlock();
				iNodeList->addBlockToINode(cachedDirectoryNode, currentDiskBlockNumber);
			} else {
//				cout << "using existing block" << endl;
				currentDiskBlockNumber = iNodeList->getBlockNumber(cachedDirectoryNode,currentNodeBlockNumber);
			}
		}

		// write the entry and the name
		DirectoryEntryData directoryEntryData;
		directoryEntryData.data.iNodeRef = pair.second;
		directoryEntryData.data.nameOffset = directoryHeaderData.data.endOfLastName;

		writeDirectoryEntryData(directoryEntryData, currentDiskBlockNumber, directoryHeaderData.data.startOfLastNode);
//		cout << "name " << pair.first << endl;
//		cout << " currentDiskBlockNumber " <<  currentDiskBlockNumber << " endOfLastName " << directoryHeaderData.data.endOfLastName << endl;
		writeDirectoryEntryName(pair.first, currentDiskBlockNumber, directoryHeaderData.data.endOfLastName);

		directoryHeaderData.data.endOfLastName += pair.first.length()+1;
		directoryHeaderData.data.startOfLastNode -= directoryEntrySize;
	}
	// write the header
	writeDirectoryHeader(currentDiskBlockNumber,directoryHeaderData);
}

/*
 * Writes a directory entry to the bottom of a specified block
 */
void Directory::writeDirectoryEntryData(DirectoryEntryData entry, int blockNumber, int lastNodePointer) {
	hdd->seek(getStartOfBlock(blockNumber) + lastNodePointer - directoryEntrySize);
	for (int i=0;i<directoryEntrySize;i++) {
		hdd->write(entry.c[i]);
	}
}

/*
 * Writes a directory entry name to the top of a specified block
 */
void Directory::writeDirectoryEntryName(string name, int blockNumber, int startPointer) {
	hdd->seek(getStartOfBlock(blockNumber) + startPointer);
	for (unsigned int i=0;i<name.length();i++) {
		hdd->write(name.at(i));
	}
	hdd->write('\0');
}

/*
 * Writes a directory header to the top of a specified block
 */
void Directory::writeDirectoryHeader(int blockNumber, DirectoryHeaderData directoryHeader) {
	hdd->seek(getStartOfBlock(blockNumber));
	for (int i=0;i<directoryHeaderSize;i++) {
		hdd->write(directoryHeader.c[i]);
	}
}

/*
 * Clears all the entries from the cache
 */
void Directory::resetCache() {
	for (int i=0;i<8;i++) {
		cachedDirectoryLastBlockHeader.c[i] = '\0';
	}
	cachedDirectoryNode = -1;
	cachedDirectory.clear();
}

/*
 * Caches a directory represented by an iNode
 */
void Directory::cacheDirectory(int iNodeNumber) {
//	cout << "resetting cached directory" << endl;
	resetCache();
//	cout << "finished resetting cached directory" << endl;
	cachedDirectoryNode = iNodeNumber;
	int numberOfBlocks = iNodeList->getNumberOfBlocks(cachedDirectoryNode);
	if (numberOfBlocks<1) throw std::runtime_error("Directory has 0 blocks ERROR !!");

	for (int i=0;i<numberOfBlocks;i++) {
//		cout << "caching directory node" << cachedDirectoryNode << endl;
//		cout << "caching directory block " << iNodeList->getBlockNumber(cachedDirectoryNode,i) << endl;
		cacheDirectoryBlock(iNodeList->getBlockNumber(cachedDirectoryNode,i));
	}
}

/*
 * Caches a directory block
 */
void Directory::cacheDirectoryBlock(int blockNumber) {
	// read the directory header
	cachedDirectoryLastBlockHeader = readDirectoryHeader(blockNumber);
//	cout << "directory header, endOfLastName = " << cachedDirectoryLastBlockHeader.data.endOfLastName << " startOfLastNode = " << cachedDirectoryLastBlockHeader.data.startOfLastNode << endl;
	// calculate number of entries
	int numberOfEntries = (superBlock->getBlockSize() - cachedDirectoryLastBlockHeader.data.startOfLastNode) / directoryEntrySize;
//	cout << "numberOfEntries " << numberOfEntries << endl;
	// read the directory names
	string names = readDirectoryEntryNames(blockNumber, numberOfEntries, cachedDirectoryLastBlockHeader.data.endOfLastName);
//	cout << "names " << names << endl;
	string currentName = "";// cache the root directory
	unsigned int namesPointer = 0;
	// read entries
//	cout << "splitting names" << endl;
	for (int i=numberOfEntries-1;i>-1;i--) {
		while (namesPointer < names.length() && names.at(namesPointer)!='\0') {
//			cout << "char at pointer " << namesPointer << " = " << names.at(namesPointer) << endl;
			currentName += names.at(namesPointer);
			namesPointer++;
		}
//		cout << "Current name" << currentName << endl;
		cachedDirectory.insert({currentName,readDirectoryEntry(blockNumber, i).data.iNodeRef});
		currentName.clear();
		namesPointer++;
	}
}

/*
 * Caches a tree of the disks directory structure
 */
void Directory::cacheDirectoryTree() {
	// setup a queue (for breadth first search of the directory tree)
	queue<TreeQueuePair> myQueue;
	myQueue.push(TreeQueuePair{rootINode,""});

	while (myQueue.size()>0) {
		TreeQueuePair currentPair = myQueue.front();
		cacheDirectory(rootINode);
		for (const auto &pair : cachedDirectory) {
			if (iNodeList->isINodeDirectory(pair.second)) {
				directoryTree->createDirectory(currentPair.path, pair.first, pair.second);
				myQueue.push(TreeQueuePair{pair.second,currentPair.path + "/" + pair.first});
			}
		}
		myQueue.pop();
	}
}

/*
 * Returns the header from a directory block
 */
DirectoryHeaderData Directory::readDirectoryHeader(int blockNumber) {
	hdd->seek(getStartOfBlock(blockNumber));
	DirectoryHeaderData directoryHeader;
	for (int i=0;i<directoryHeaderSize;i++) {
		directoryHeader.c[i] = hdd->read();
	}
	return directoryHeader;
}

/*
 * Returns all the names from the beginning of a directory block
 */
string Directory::readDirectoryEntryNames(int blockNumber, int numberOfEntries, int endOfLastName) {
	hdd->seek(getStartOfBlock(blockNumber) + directoryHeaderSize);
	int nameLength = endOfLastName - directoryHeaderSize;
//	cout << "nameLength " << nameLength << endl;
	stringstream names;
	for (int i=0;i<nameLength;i++) {
		names << hdd->read();
	}
	return names.str();
}

/*
 * Returns a directory entry from an offset at the end of a directory block
 */
DirectoryEntryData Directory::readDirectoryEntry(int blockNumber, int entryNumber) {
	hdd->seek(getStartOfBlock(blockNumber+1) - (entryNumber * directoryEntrySize));
	DirectoryEntryData directoryEntry;
	for (int i=0;i<directoryEntrySize;i++) {
		directoryEntry.c[i] = hdd->read();
	}
	return directoryEntry;
}

/*
 * Returns the starting position of a block
 */
int Directory::getStartOfBlock(int blockNumber) {
	return (superBlock->getFirstBlockStart() + (blockNumber * superBlock->getBlockSize()));
}

/*
 * Returns true if the filename is legal, false otherwise
 */
bool Directory::isNameLegal(string name) {
	unsigned int pointer = 0;
	int currentCharValue;
	while (pointer<name.length()) {
		currentCharValue = name.at(pointer);
		if (currentCharValue < 48) return false;
		if (currentCharValue > 57 && currentCharValue < 65) return false;
		if (currentCharValue > 90 && currentCharValue < 97) return false;
		if (currentCharValue > 122) return false;
		pointer++;
	}
	return true;
}

/*
 * Returns true if the directory path is legal, false otherwise
 */
bool Directory::isPathNameLegal(string name) {
	unsigned int pointer = 0;
	int currentCharValue;
	while (pointer<name.length()) {
		currentCharValue = name.at(pointer);
		if (currentCharValue < 47) return false;
		if (currentCharValue > 57 && currentCharValue < 65) return false;
		if (currentCharValue > 90 && currentCharValue < 97) return false;
		if (currentCharValue > 122) return false;
		pointer++;
	}
	return true;
}

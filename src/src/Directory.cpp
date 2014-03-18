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
#include <assert.h>

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

/*
 * Create a file on the disk
 */
bool Directory::createFile(string path, string name, string &data) {
	if (!initialised) throw std::runtime_error("Directory not Initialised");
	if (isPathNameLegal(path) && isNameLegal(name)) {
		// check the path is valid
		int directoryNode = directoryTree->getPathNode(path);
		if (directoryNode < 0) {
			cout << "path is invalid" << endl;
			return false;
		}
		// cache the directory (path) if it is not already cached
		if (directoryNode != cachedDirectoryNode) cacheDirectory(directoryNode);
		// check the filename does not already exist
		if (cachedDirectory.count(name) > 0) {
			cout << "file/Directory name already used" << endl;
			return false;
		}
		// create an INode
		int fileBlock = freeBlockList->getBlock();
		int fileNode = iNodeList->writeNewINode(false,data.length(),fileBlock);
		// write the file
		if (!writeFile(fileNode,data)) {
			return false;
		}
		// add the file to the directory
		cout << "Inserting " << name << " - " << fileNode << endl;
		cachedDirectory.insert({name,fileNode});
		// write the directory
		writeCachedDirectory();
		cout << listDirContents(path) << endl;

		return true;
	} else {
		cout << "Path or filename not legal" << endl;
		return false;
	}
}

/*
 * Rewrite an existing file
 */
bool Directory::rewriteFile(string path, string name, string &data) {
	if (!initialised) throw std::runtime_error("Directory not Initialised");
	if (isPathNameLegal(path) && isNameLegal(name)) {
		// check the path is valid
		int directoryNode = directoryTree->getPathNode(path);
		if (directoryNode < 0) {
			cout << "path is invalid" << endl;
			return false;
		}
		// cache the directory (path) if it is not already cached
		if (directoryNode != cachedDirectoryNode) cacheDirectory(directoryNode);
		cout << listDirContents(path) << endl;

		// check the filename exists
		if (cachedDirectory.count(name) < 1) {
			cout << "file does not exist" << endl;
			return false;
		} else if (iNodeList->isINodeDirectory(cachedDirectory[name])) {
			cout << "filename refers to directory - unable to delete" << endl;
			return false;
		}
		// write the file
		if (!writeFile(cachedDirectory[name],data)) {
			return false;
		}
		return true;
	} else {
		cout << "Path or filename not legal" << endl;
		return false;
	}
}

/*
 * Deletes a file (or hard link) from the disk
 */
bool Directory::deleteFile(string path, string name) {
	if (!initialised) throw std::runtime_error("Directory not Initialised");
	if (isPathNameLegal(path) && isNameLegal(name)) {
		// check the path is valid
		int directoryNode = directoryTree->getPathNode(path);
		if (directoryNode < 0) {
			cout << "path is invalid" << endl;
			return false;
		}
		// cache the directory (path) if it is not already cached
		if (directoryNode != cachedDirectoryNode) cacheDirectory(directoryNode);
		// check the filename exists and is not a directory
		if (cachedDirectory.count(name) < 1) {
			cout << "file does not exist" << endl;
			return false;
		} else if (iNodeList->isINodeDirectory(cachedDirectory[name])) {
			cout << "filename refers to directory - unable to delete" << endl;
			return false;
		}
		// remove a link from the files iNode
		iNodeList->removeLinkFromINode(cachedDirectory[name]);
		// delete the file from the directory
		cachedDirectory.erase(name);
		// write the directory
		writeCachedDirectory();
		return true;
	} else {
		cout << "Path or filename not legal" << endl;
		return false;
	}
}

/*
 * Rename a file
 */
bool Directory::renameFile(string fromPath, string fromName, string toName) {
	if (!initialised) throw std::runtime_error("Directory not Initialised");
	if (isPathNameLegal(fromPath) && isNameLegal(fromName) && isNameLegal(toName)) {
		// check the path is valid
		int directoryNode = directoryTree->getPathNode(fromPath);
		if (directoryNode < 0) {
			cout << "path is invalid" << endl;
			return false;
		}
		// cache the directory (path) if it is not already cached
		if (directoryNode != cachedDirectoryNode) cacheDirectory(directoryNode);
		// check the from filename exists and is not a directory
		if (cachedDirectory.count(fromName) < 1) {
			cout << "file does not exist" << endl;
			return false;
		} else if (iNodeList->isINodeDirectory(cachedDirectory[fromName])) {
			cout << "filename " << fromName << " refers to directory - unable to delete" << endl;
			return false;
		}
		// check the toName does not exist
		if (cachedDirectory.count(toName) > 0) {
			cout << "file " << toName << " already exists" << endl;
			return false;
		}
		// change the name in the directory
		cachedDirectory[toName] = cachedDirectory[fromName];
		cachedDirectory.erase(fromName);
		// write the directory
		writeCachedDirectory();
		return true;
	} else {
		cout << "Path or filename not legal" << endl;
		return false;
	}
}

/*
 * Move a file
 */
bool Directory::moveFile(string fromPath, string fromName, string toPath, string toName) {
	if (!initialised) throw std::runtime_error("Directory not Initialised");
	if (isPathNameLegal(fromPath) && isNameLegal(fromName) && isPathNameLegal(toPath) && isNameLegal(toName)) {
		// if the fromPath and toPath are the same just rename the file
		if (fromPath == toPath) {
			return renameFile(fromPath,fromName,toName);
		}
		// check the paths are valid
		int fromDirectoryNode = directoryTree->getPathNode(fromPath);
		int toDirectoryNode = directoryTree->getPathNode(toPath);
		if (fromDirectoryNode < 0) {
			cout << "path is invalid" << endl;
			return false;
		}
		if (toDirectoryNode < 0) {
			cout << "path is invalid" << endl;
			return false;
		}
		// cache the from directory (path) if it is not already cached
		if (fromDirectoryNode != cachedDirectoryNode) cacheDirectory(fromDirectoryNode);
		// check the fromName exists and is not a directory
		if (cachedDirectory.count(fromName) < 1) {
			cout << "file " << fromName << " does not exist" << endl;
			return false;
		} else if (iNodeList->isINodeDirectory(cachedDirectory[fromName])) {
			cout << fromName << " is a directory" << endl;
			return false;
		}
		DirectoryHeaderData fromCachedDirectoryLastBlockHeader = cachedDirectoryLastBlockHeader;
		int fromCachedDirectoryNode = cachedDirectoryNode;
		map <std::string, int> fromCachedDirectory = cachedDirectory;

		// cache the to directory (path)
		cacheDirectory(toDirectoryNode);
		// check the toName does not exist
		if (cachedDirectory.count(toName) > 0) {
			cout << "file " << toName << " already exists" << endl;
			return false;
		}
		// add the file to the to directory
		cachedDirectory[toName] = fromCachedDirectory[fromName];
		// write the to directory;
		writeCachedDirectory();

		// erase the fromName from the fromPath
		cachedDirectoryLastBlockHeader = fromCachedDirectoryLastBlockHeader;
		cachedDirectoryNode = fromCachedDirectoryNode;
		cachedDirectory = fromCachedDirectory;

		cachedDirectory.erase(fromName);
		// write the from directory
		writeCachedDirectory();
		return true;
	} else {
		cout << "Path or filename not legal" << endl;
		return false;
	}
}

bool Directory::copyFile(string fromPath, string fromName, string toPath, string toName) {
	if (!initialised) throw std::runtime_error("Directory not Initialised");
	string data;

	cout << "Reading file " << fromPath << fromName << endl;

	if (!readFile(fromPath, fromName, data)) return false;

	cout << "Creating file " << toPath << toName << endl;
	return createFile(toPath, toName, data);
}

bool Directory::makeHardLinkToFile(string fromPath, string fromName, string toPath, string toName) {
	if (!initialised) throw std::runtime_error("Directory not Initialised");
	if (isPathNameLegal(fromPath) && isNameLegal(fromName) && isPathNameLegal(toPath) && isNameLegal(toName)) {
		// check the paths are valid
		int fromDirectoryNode = directoryTree->getPathNode(fromPath);
		int toDirectoryNode = directoryTree->getPathNode(toPath);
		if (fromDirectoryNode < 0) {
			cout << "path is invalid" << endl;
			return false;
		}
		if (toDirectoryNode < 0) {
			cout << "path is invalid" << endl;
			return false;
		}

		// cache the from directory (path) if it is not already cached
		if (fromDirectoryNode != cachedDirectoryNode) cacheDirectory(fromDirectoryNode);
		// check the fromName exists and is not a directory
		if (cachedDirectory.count(fromName) < 1) {
			cout << "file " << fromName << " does not exist" << endl;
			return false;
		} else if (iNodeList->isINodeDirectory(cachedDirectory[fromName])) {
			cout << fromName << " is a directory" << endl;
			return false;
		}

		int fileNode = cachedDirectory[fromName];

		// cache the to directory (path)
		cacheDirectory(toDirectoryNode);
		// check the toName does not exist
		if (cachedDirectory.count(toName) > 0) {
			cout << "file " << toName << " already exists" << endl;
			return false;
		}
		// add the file to the to directory
		cachedDirectory[toName] = fileNode;
		// write the to directory;
		writeCachedDirectory();
		return true;
	} else {
		cout << "Path or filename not legal" << endl;
		return false;
	}
}

/*
 * Reads a file from the disk and puts the data in data and the fileLength in fileLength
 */
bool Directory::readFile(string path, string name, string &data) {
	if (!initialised) throw std::runtime_error("Directory not Initialised");
	if (isPathNameLegal(path) && isNameLegal(name)) {
		// check the path is valid
		int directoryNode = directoryTree->getPathNode(path);
		if (directoryNode < 0) {
			cout << "path is invalid" << endl;
			return false;
		}
		// cache the directory (path) if it is not already cached
		if (directoryNode != cachedDirectoryNode) cacheDirectory(directoryNode);
		// check the filename exists
		if (cachedDirectory.count(name) < 1) {
			cout << "file name does not exist" << endl;
			return false;
		} else if (iNodeList->isINodeDirectory(cachedDirectory[name])) {
			cout << path << name << " is a directory" << endl;
			return false;
		}
		data.clear();
		// read the file and put the data in data and the length in fileLength
		for (int i=0;i<iNodeList->getNumberOfBlocks(cachedDirectory[name]);i++) {
			readFileBlock(iNodeList->getBlockNumber(cachedDirectory[name],i),data);
		}
		return true;
	} else {
		cout << "Path or filename not legal" << endl;
		return false;
	}
}

/*
 * Reads a file and locks it
 */
bool Directory::openFile(string path, string name, string &data) {
	if (!initialised) throw std::runtime_error("Directory not Initialised");
	if (readFile(path,name,data)) {
		iNodeList->lockINode(cachedDirectory[name]);
		return true;
	} else {
		return false;
	}
}

/*
 * Unlocks a file
 */
bool Directory::closeFile(string path, string name) {
	if (!initialised) throw std::runtime_error("Directory not Initialised");
	if (isPathNameLegal(path) && isNameLegal(name)) {
		// check the path is valid
		int directoryNode = directoryTree->getPathNode(path);
		if (directoryNode < 0) {
			cout << "path is invalid" << endl;
			return false;
		}
		// cache the directory (path) if it is not already cached
		if (directoryNode != cachedDirectoryNode) cacheDirectory(directoryNode);
		// check the filename exists
		if (cachedDirectory.count(name) < 1) {
			cout << "file name does not exist" << endl;
			return false;
		}
		iNodeList->unLockINode(cachedDirectory[name]);
		return true;
	} else {
		cout << "Path or filename not legal" << endl;
		return false;
	}
}

// ********************** Directory operations **********************

string Directory::listDirContents(string path) {
	if (!initialised) throw std::runtime_error("Directory not Initialised");
	stringstream returnString;
	if (isPathNameLegal(path)) {
		// check the directory (path + name) exists (directoryTree)
		int directoryNode = directoryTree->getPathNode(path);
		if (directoryNode < 0) {
			cout << "path is invalid" << endl;
			return returnString.str();
		}
		// cache the directory if it is not already cached
		if (directoryNode != cachedDirectoryNode) cacheDirectory(directoryNode);
		// create the output string
		returnString << "DIRECTORY " << path << " - " << directoryNode << endl;
		for (const auto &pair : cachedDirectory) {
			returnString << pair.first;
			if (iNodeList->isINodeDirectory(pair.second)) returnString << " - Directory";
			returnString << endl;
		}
	} else {
		cout << "Path not legal" << endl;
	}
	return returnString.str();
}

bool Directory::deleteDir(string path, string name) {
	if (!initialised) throw std::runtime_error("Directory not Initialised");
	if (isPathNameLegal(path) && isNameLegal(name)) {
		// check the directory (path + name) exists (directoryTree)
		int directoryNode = directoryTree->getPathNode(path + name + "/");
		if (directoryNode < 0) {
			cout << "path is invalid" << endl;
			return false;
		}
		// cache the directory if it is not already cached
		if (directoryNode != cachedDirectoryNode) cacheDirectory(directoryNode);
		// check the directory is empty
		if (cachedDirectory.size()>0) {
			cout << "directory is not empty" << endl;
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
		return true;
	} else {
		cout << "Path or filename not legal" << endl;
		return false;
	}
}

bool Directory::createDir(string path, string name) {
	if (!initialised) throw std::runtime_error("Directory not Initialised");
	if (isPathNameLegal(path) && isNameLegal(name)) {
		int parentDirectoryNode = directoryTree->getPathNode(path);
		// check the path is valid
		if (parentDirectoryNode < 0) {
			cout << "path is invalid" << endl;
			return false;
		}
		// cache the parent directory (path) if it is not already cached
		if (parentDirectoryNode != cachedDirectoryNode) cacheDirectory(parentDirectoryNode);
		// check the name is not already in use
		if (cachedDirectory.count(name) > 0) {
			cout << "file/Directory name already used" << endl;
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
		return true;
	} else {
		cout << "Path or filename not legal" << endl;
		return false;
	}
}

bool Directory::moveDir(string fromPath, string fromName, string toPath, string toName) {
	if (!initialised) throw std::runtime_error("Directory not Initialised");
	if (isPathNameLegal(fromPath) && isNameLegal(fromName) && isPathNameLegal(toPath) && isNameLegal(toName)) {
		int toDir = directoryTree->getPathNode(toPath);
		int fromDir = directoryTree->getPathNode(fromPath);
		int toMoveDir = directoryTree->getPathNode(fromPath+fromName+"/");
		// check the fromPath and fromName exist
		if (toMoveDir < 0) {
			cout << fromPath << fromName << "/" << " does not exist" << endl;
			return false;
		}
		// check the toPath exists
		if (toDir < 0) {
			cout << toDir << " does not exist" << endl;
			return false;
		}

		// cache the to directory (path) if it is not already cached
		if (toDir != cachedDirectoryNode) cacheDirectory(toDir);
		// check the toName does not already exist
		if (cachedDirectory.count(toName) > 0) {
			cout << toPath << "/" << toDir << " already exists" << endl;
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
		return true;
	} else {
		cout << "Path or filename not legal" << endl;
		return false;
	}
}

bool Directory::makeHardLinkDir(string fromPath, string fromName, string toPath, string toName) {
	if (!initialised) throw std::runtime_error("Directory not Initialised");
	if (isPathNameLegal(fromPath) && isNameLegal(fromName) && isPathNameLegal(toPath) && isNameLegal(toName)) {
		int toDir = directoryTree->getPathNode(toPath);
		int toCopyDir = directoryTree->getPathNode(fromPath+fromName+"/");
		// check the fromPath and fromName exist
		if (toCopyDir < 0) {
			cout << fromPath << fromName << "/" << " does not exist" << endl;
			return false;
		}
		// check the toPath exists
		if (toDir < 0) {
			cout << toDir << " does not exist" << endl;
			return false;
		}

		// cache the to directory (path) if it is not already cached
		if (toDir != cachedDirectoryNode) cacheDirectory(toDir);
		// check the toName does not already exist
		if (cachedDirectory.count(toName) > 0) {
			cout << toPath << "/" << toDir << " already exists" << endl;
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
		return true;
	} else {
		cout << "Path or filename not legal" << endl;
		return false;
	}
}

// ********************** Local File operations **********************

/*
 * write file data to disk
 */
bool Directory::writeFile(int iNodeNumber, string &data) {
	// get the number of nodes the file currently has allocated (must be at least 1)
	int currentNumberOfBlocks = iNodeList->getNumberOfBlocks(iNodeNumber);
	// calculate number of Blocks needed
	int numberOfBlocksNeeded = (data.length() + getDataInBlock() - 1) / getDataInBlock();
	if (numberOfBlocksNeeded > currentNumberOfBlocks + iNodeList->getNumberOfFreeINodes()) {
		cout << "Not enough space on disk for file" << endl;
		return false;
	}
	// point to the first block number
	int currentDiskBlockNumber = 0;
	int dataPointer = 0;
	FileHeaderData fileHeaderData;
	for (int i=0;i<numberOfBlocksNeeded;i++) {
		// get currentDiskBlockNumber, add to iNode if necessary
		if (i == currentNumberOfBlocks) {
			currentDiskBlockNumber = freeBlockList->getBlock();
			iNodeList->addBlockToINode(iNodeNumber, currentDiskBlockNumber);
			currentNumberOfBlocks ++;
		} else {
			currentDiskBlockNumber = iNodeList->getBlockNumber(iNodeNumber,i);
		}
		// current block header
		dataPointer = i * getDataInBlock();
		fileHeaderData.lengthOfBlock = min((unsigned int)data.length() - dataPointer, getDataInBlock());
		// write the file block header
		writeFileBlockHeader(currentDiskBlockNumber, fileHeaderData);
		// write the file block
		writeFileBlock(currentDiskBlockNumber, data, dataPointer, dataPointer+fileHeaderData.lengthOfBlock);
	}
	// remove any un-needed blocks from the iNode
	for (int i=currentNumberOfBlocks;i>numberOfBlocksNeeded;i--) {
		iNodeList->removeBlockFromINode(iNodeNumber);
	}
	// set the iNodeSize
	iNodeList->setINodeSize(iNodeNumber,data.length());
	return true;
}

/*
 * write a file block to disk
 */
void Directory::writeFileBlock(int blockNumber, string &data, int startPointer, int endPointer) {
	hdd->seek(getStartOfBlock(blockNumber) + fileHeaderSize);
	for (int i=startPointer;i<endPointer;i++) {
		hdd->write(data.at(i));
	}
}

/*
 * write a file block header to disk
 */
void Directory::writeFileBlockHeader(int blockNumber, FileHeaderData fileHeaderData) {
	hdd->seek(getStartOfBlock(blockNumber));
	for (int i=0;i<fileHeaderSize;i++) {
		hdd->write(fileHeaderData.c[i]);
	}
}

void Directory::readFileBlock(int blockNumber, string &data) {
	hdd->seek(getStartOfBlock(blockNumber) + fileHeaderSize);
	FileHeaderData fileHeaderData = readFileBlockHeader(blockNumber);
	for (int i=0;i<fileHeaderData.lengthOfBlock;i++) {
		data += hdd->read();
	}
}

FileHeaderData Directory::readFileBlockHeader(int blockNumber) {
	hdd->seek(getStartOfBlock(blockNumber));
	FileHeaderData fileHeaderData;
	for (int i=0;i<4;i++) {
		fileHeaderData.c[i] = hdd->read();
	}
	return fileHeaderData;
}

unsigned int Directory::getDataInBlock() {
	return (superBlock->getBlockSize() - fileHeaderSize);
}

// ********************** Local Directory operations **********************

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
			blockSizeRemaining -= entrySize;
		} else {
			// write the header
			writeDirectoryHeader(currentDiskBlockNumber,directoryHeaderData);
			// reset the counters
			blockSizeRemaining = superBlock->getBlockSize() - directoryHeaderSize - entrySize;
			directoryHeaderData.data.endOfLastName = directoryHeaderSize;
			directoryHeaderData.data.startOfLastNode = superBlock->getBlockSize();
			currentNodeBlockNumber++;
			// allocate a new block to the iNode if needed
			if (currentNodeBlockNumber == currentNumberOfBlocks) {
				currentNumberOfBlocks++;
				currentDiskBlockNumber = freeBlockList->getBlock();
				iNodeList->addBlockToINode(cachedDirectoryNode, currentDiskBlockNumber);
			} else {
				currentDiskBlockNumber = iNodeList->getBlockNumber(cachedDirectoryNode,currentNodeBlockNumber);
			}
		}

		// write the entry and the name
		DirectoryEntryData directoryEntryData;
		directoryEntryData.data.iNodeRef = pair.second;
		directoryEntryData.data.nameOffset = directoryHeaderData.data.endOfLastName;
		writeDirectoryEntryData(directoryEntryData, currentDiskBlockNumber, directoryHeaderData.data.startOfLastNode);
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
	resetCache();
	cachedDirectoryNode = iNodeNumber;
	int numberOfBlocks = iNodeList->getNumberOfBlocks(cachedDirectoryNode);
	if (numberOfBlocks<1) throw std::runtime_error("Directory has 0 blocks ERROR !!");

	for (int i=0;i<numberOfBlocks;i++) {
		cacheDirectoryBlock(iNodeList->getBlockNumber(cachedDirectoryNode,i));
	}
}

/*
 * Caches a directory block
 */
void Directory::cacheDirectoryBlock(int blockNumber) {
	// read the directory header
	cachedDirectoryLastBlockHeader = readDirectoryHeader(blockNumber);
	// calculate number of entries
	int numberOfEntries = (superBlock->getBlockSize() - cachedDirectoryLastBlockHeader.data.startOfLastNode) / directoryEntrySize;
	// read the directory names
	string names = readDirectoryEntryNames(blockNumber, numberOfEntries, cachedDirectoryLastBlockHeader.data.endOfLastName);
	string currentName = "";// cache the root directory
	unsigned int namesPointer = 0;
	// read entries
	for (int i=numberOfEntries-1;i>-1;i--) {
		DirectoryEntryData entry = readDirectoryEntry(blockNumber, i);
		namesPointer = entry.data.nameOffset - directoryHeaderSize;
		while (namesPointer < names.length() && names.at(namesPointer)!='\0') {
			currentName += names.at(namesPointer);
			namesPointer++;
		}
		cachedDirectory.insert({currentName,entry.data.iNodeRef});
		currentName.clear();
		namesPointer++;
	}
}

/*
 * Used in the queue for caching directory tree
 */
struct TreeQueuePair {
	int iNodeRef;
	string path;
};

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
	hdd->seek(getStartOfBlock(blockNumber+1) - ((entryNumber+1) * directoryEntrySize));
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
bool Directory::isNameLegal(string &name) {
	unsigned int pointer = 0;
	int currentCharValue;
	while (pointer<name.length()) {
		currentCharValue = name.at(pointer);
		if (currentCharValue < 32) return false;
		if (currentCharValue > 32 && currentCharValue < 48) return false;
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
bool Directory::isPathNameLegal(string &name) {
	unsigned int pointer = 0;
	int currentCharValue;
	while (pointer<name.length()) {
		currentCharValue = name.at(pointer);
		if (currentCharValue < 32) return false;
		if (currentCharValue > 32 && currentCharValue < 47) return false;
		if (currentCharValue > 57 && currentCharValue < 65) return false;
		if (currentCharValue > 90 && currentCharValue < 97) return false;
		if (currentCharValue > 122) return false;
		pointer++;
	}
	if (name.length() > 0 && name.at(pointer-1) != 47) name += 47;
	return true;
}

#include "Directory.h"
#include "CachedDirectory.h"

#include <iostream>
#include <stdexcept>
#include <sstream>
#include <queue>
#include <assert.h>

Directory::Directory(HDD* _hdd, SuperBlock* _superBlock, FreeBlockList* _freeBlockList, INodeList* _iNodeList, int _rootINode) {
	// from input
	hdd = _hdd;
	superBlock = _superBlock;
	freeBlockList = _freeBlockList;
	iNodeList = _iNodeList;
	rootINode = _rootINode;
	directoryTree = new DirectoryTree(rootINode);
	initialised = false;
	freeCaches = numberOfCachedDirectorys;
	lastUsed = 0;
	oldest = 0;
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
	cachedDirectory[0] = new CachedDirectory(rootINode,hdd,superBlock,freeBlockList,iNodeList);
	cachedDirectory[0]->writeRootNode();
	freeCaches--;
	lastUsed++;
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
			cout << "path " << path << " is invalid" << endl;
			return false;
		}
		// cache the directory (path) if it is not already cached
		int directoryNodeCache = getCachedDirectory(directoryNode);
		// check the filename does not already exist
		if (cachedDirectory[directoryNodeCache]->cache.count(name) > 0) {
			cout << "file/Directory " << name << " already exists" << endl;
			freeCachedDirectory(directoryNodeCache);
			return false;
		}
		// create an INode
		int fileBlock = freeBlockList->getBlock();
		int fileNode = iNodeList->writeNewINode(false,data.length(),fileBlock);
		iNodeList->lockINode(fileNode);
		// write the file
		if (!writeFile(fileNode,data)) {
			iNodeList->unLockINode(fileNode);
			freeCachedDirectory(directoryNodeCache);
			return false;
		}
		// add the file to the directory
		iNodeList->lockINode(directoryNode);
		cachedDirectory[directoryNodeCache]->cache.insert({name,fileNode});
		// write the directory
		cachedDirectory[directoryNodeCache]->writeCachedDirectory();
		iNodeList->unLockINode(directoryNode);
		iNodeList->unLockINode(fileNode);
		freeCachedDirectory(directoryNodeCache);
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
			cout << "path " << path << " is invalid" << endl;
			return false;
		}
		// cache the directory (path) if it is not already cached
		int directoryNodeCache = getCachedDirectory(directoryNode);
		// check the filename exists
		if (cachedDirectory[directoryNodeCache]->cache.count(name) < 1) {
			cout << "file " << name << " does not exist" << endl;
			freeCachedDirectory(directoryNodeCache);
			return false;
		} else if (iNodeList->isINodeDirectory(cachedDirectory[directoryNodeCache]->cache[name])) {
			cout << name << " refers to directory - unable to rewrite" << endl;
			freeCachedDirectory(directoryNodeCache);
			return false;
		}
		iNodeList->lockINode(cachedDirectory[directoryNodeCache]->cache[name]);
		// write the file
		if (!writeFile(cachedDirectory[directoryNodeCache]->cache[name],data)) {
			iNodeList->unLockINode(cachedDirectory[directoryNodeCache]->cache[name]);
			freeCachedDirectory(directoryNodeCache);
			return false;
		}
		iNodeList->unLockINode(cachedDirectory[directoryNodeCache]->cache[name]);
		freeCachedDirectory(directoryNodeCache);
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
			cout << "path " << path << " is invalid" << endl;
			return false;
		}
		// cache the directory (path) if it is not already cached
		int directoryNodeCache = getCachedDirectory(directoryNode);
		// check the filename exists and is not a directory
		if (cachedDirectory[directoryNodeCache]->cache.count(name) < 1) {
			cout << "file " << name << " does not exist" << endl;
			freeCachedDirectory(directoryNodeCache);
			return false;
		} else if (iNodeList->isINodeDirectory(cachedDirectory[directoryNodeCache]->cache[name])) {
			cout << name << " refers to directory - unable to delete" << endl;
			freeCachedDirectory(directoryNodeCache);
			return false;
		}
		// remove a link from the files iNode
		iNodeList->removeLinkFromINode(cachedDirectory[directoryNodeCache]->cache[name]);
		// delete the file from the directory
		iNodeList->lockINode(directoryNode);
		cachedDirectory[directoryNodeCache]->cache.erase(name);
		// write the directory
		cachedDirectory[directoryNodeCache]->writeCachedDirectory();
		iNodeList->unLockINode(directoryNode);
		freeCachedDirectory(directoryNodeCache);
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
			cout << "path " << fromPath << " is invalid" << endl;
			return false;
		}
		// cache the directory (path) if it is not already cached
		int directoryNodeCache = getCachedDirectory(directoryNode);
		// check the from filename exists and is not a directory
		if (cachedDirectory[directoryNodeCache]->cache.count(fromName) < 1) {
			cout << "file " << fromName << " does not exist" << endl;
			freeCachedDirectory(directoryNodeCache);
			return false;
		} else if (iNodeList->isINodeDirectory(cachedDirectory[directoryNodeCache]->cache[fromName])) {
			cout << fromName << " refers to directory - unable to rename" << endl;
			freeCachedDirectory(directoryNodeCache);
			return false;
		}
		// check the toName does not exist
		if (cachedDirectory[directoryNodeCache]->cache.count(toName) > 0) {
			cout << "file/Directory " << toName << " already exists" << endl;
			freeCachedDirectory(directoryNodeCache);
			return false;
		}
		// change the name in the directory
		iNodeList->lockINode(directoryNode);
		cachedDirectory[directoryNodeCache]->cache[toName] = cachedDirectory[directoryNodeCache]->cache[fromName];
		cachedDirectory[directoryNodeCache]->cache.erase(fromName);
		// write the directory
		cachedDirectory[directoryNodeCache]->writeCachedDirectory();
		iNodeList->unLockINode(directoryNode);
		freeCachedDirectory(directoryNodeCache);
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
		int toDirectoryNode = directoryTree->getPathNode(toPath);
		int fromDirectoryNode = directoryTree->getPathNode(fromPath);
		if (fromDirectoryNode < 0) {
			cout << "path " << fromPath << " is invalid" << endl;
			return false;
		}
		if (toDirectoryNode < 0) {
			cout << "path " << toPath << " is invalid" << endl;
			return false;
		}
		// cache the from directory (path) if it is not already cached
		int fromDirectoryNodeCache = getCachedDirectory(fromDirectoryNode);
		// cache the to directory (path)
		int toDirectoryNodeCache = getCachedDirectory(toDirectoryNode);
		// check the fromName exists and is not a directory
		if (cachedDirectory[fromDirectoryNodeCache]->cache.count(fromName) < 1) {
			cout << "file " << fromName << " does not exist" << endl;
			freeCachedDirectory(fromDirectoryNodeCache);
			return false;
		} else if (iNodeList->isINodeDirectory(cachedDirectory[fromDirectoryNodeCache]->cache[fromName])) {
			cout << fromName << " refers to directory - unable to move" << endl;
			freeCachedDirectory(fromDirectoryNodeCache);
			return false;
		}
		// check the toName does not exist
		if (cachedDirectory[toDirectoryNodeCache]->cache.count(toName) > 0) {
			cout << "file/Directory " << toName << " already exists" << endl;
			freeCachedDirectory(fromDirectoryNodeCache);
			freeCachedDirectory(toDirectoryNodeCache);
			return false;
		}
		iNodeList->lockINode(toDirectoryNode);
		iNodeList->lockINode(fromDirectoryNode);
		// add the file to the to directory
		cachedDirectory[toDirectoryNodeCache]->cache[toName] = cachedDirectory[fromDirectoryNodeCache]->cache[fromName];
		// write the to directory;
		cachedDirectory[toDirectoryNodeCache]->writeCachedDirectory();
		cachedDirectory[fromDirectoryNodeCache]->cache.erase(fromName);
		// write the from directory
		cachedDirectory[fromDirectoryNodeCache]->writeCachedDirectory();
		iNodeList->unLockINode(fromDirectoryNode);
		iNodeList->unLockINode(toDirectoryNode);
		freeCachedDirectory(fromDirectoryNodeCache);
		freeCachedDirectory(toDirectoryNodeCache);
		return true;
	} else {
		cout << "Path or filename not legal" << endl;
		return false;
	}
}

bool Directory::makeHardLinkToFile(string fromPath, string fromName, string toPath, string toName) {
	if (!initialised) throw std::runtime_error("Directory not Initialised");
	if (isPathNameLegal(fromPath) && isNameLegal(fromName) && isPathNameLegal(toPath) && isNameLegal(toName)) {
		// check the paths are valid
		int fromDirectoryNode = directoryTree->getPathNode(fromPath);
		int toDirectoryNode = directoryTree->getPathNode(toPath);
		if (fromDirectoryNode < 0) {
			cout << "path " << fromPath << " is invalid" << endl;
			return false;
		}
		if (toDirectoryNode < 0) {
			cout << "path " << toPath << " is invalid" << endl;
			return false;
		}
		// cache the to directory (path)
		int toDirectoryNodeCache = getCachedDirectory(toDirectoryNode);
		// cache the from directory (path) if it is not already cached
		int fromDirectoryNodeCache = getCachedDirectory(fromDirectoryNode);
		// check the fromName exists and is not a directory
		if (cachedDirectory[fromDirectoryNodeCache]->cache.count(fromName) < 1) {
			cout << "file " << fromName << " does not exist" << endl;
			freeCachedDirectory(fromDirectoryNodeCache);
			freeCachedDirectory(toDirectoryNodeCache);
			return false;
		} else if (iNodeList->isINodeDirectory(cachedDirectory[fromDirectoryNodeCache]->cache[fromName])) {
			cout << fromName << " refers to directory - unable to make link" << endl;
			freeCachedDirectory(fromDirectoryNodeCache);
			freeCachedDirectory(toDirectoryNodeCache);
			return false;
		}

		int fileNode = cachedDirectory[fromDirectoryNodeCache]->cache[fromName];
		// check the toName does not exist
		if (cachedDirectory[toDirectoryNodeCache]->cache.count(toName) > 0) {
			cout << "file/Directory " << toName << " already exists" << endl;
			freeCachedDirectory(fromDirectoryNodeCache);
			freeCachedDirectory(toDirectoryNodeCache);
			return false;
		}
		iNodeList->lockINode(toDirectoryNodeCache);
		// add the file to the to directory
		cachedDirectory[toDirectoryNodeCache]->cache[toName] = fileNode;
		// write the to directory;
		cachedDirectory[toDirectoryNodeCache]->writeCachedDirectory();
		iNodeList->unLockINode(toDirectoryNodeCache);
		freeCachedDirectory(fromDirectoryNodeCache);
		freeCachedDirectory(toDirectoryNodeCache);
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
	if (openFile(path,name,data)) {
		return closeFile(path,name);
	} else {
		return false;
	}
}

/*
 * Reads a file and locks it
 */
bool Directory::openFile(string path, string name, string &data) {
	if (!initialised) throw std::runtime_error("Directory not Initialised");
	if (isPathNameLegal(path) && isNameLegal(name)) {
		// check the path is valid
		int directoryNode = directoryTree->getPathNode(path);
		if (directoryNode < 0) {
			cout << "path " << path << " is invalid" << endl;
			return false;
		}
		// cache the directory (path) if it is not already cached
		int directoryNodeCache = getCachedDirectory(directoryNode);
		// check the filename exists
		if (cachedDirectory[directoryNodeCache]->cache.count(name) < 1) {
			cout << "file " << name << " does not exist" << endl;
			freeCachedDirectory(directoryNodeCache);
			return false;
		} else if (iNodeList->isINodeDirectory(cachedDirectory[directoryNodeCache]->cache[name])) {
			cout << name << " refers to directory - unable to read" << endl;
			freeCachedDirectory(directoryNodeCache);
			return false;
		}
		data.clear();
		iNodeList->lockINode(cachedDirectory[directoryNodeCache]->cache[name]);
		// read the file and put the data in data and the length in fileLength
		for (int i=0;i<iNodeList->getNumberOfBlocks(cachedDirectory[directoryNodeCache]->cache[name]);i++) {
			readFileBlock(iNodeList->getBlockNumber(cachedDirectory[directoryNodeCache]->cache[name],i),data);
		}
		freeCachedDirectory(directoryNodeCache);
		return true;
	} else {
		cout << "Path or filename not legal" << endl;
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
			cout << "path " << path << " is invalid" << endl;
			return false;
		}
		// cache the directory (path) if it is not already cached
		int directoryNodeCache = getCachedDirectory(directoryNode);
		// check the filename exists
		if (cachedDirectory[directoryNodeCache]->cache.count(name) < 1) {
			cout << "file " << name << " does not exist" << endl;
			freeCachedDirectory(directoryNodeCache);
			return false;
		}
		iNodeList->unLockINode(cachedDirectory[directoryNodeCache]->cache[name]);
		freeCachedDirectory(directoryNodeCache);
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
			cout << "path " << path << " is invalid" << endl;
			return returnString.str();
		}
		// cache the directory if it is not already cached
		int directoryNodeCache = getCachedDirectory(directoryNode);
		// create the output string
		returnString << "DIRECTORY " << path << " - " << directoryNode << endl;
		for (const auto &pair : cachedDirectory[directoryNodeCache]->cache) {
			returnString << pair.first;
			if (iNodeList->isINodeDirectory(pair.second)) returnString << " - Directory";
			returnString << endl;
		}
		freeCachedDirectory(directoryNodeCache);
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
		int parentDirectoryNode = directoryTree->getPathNode(path);
		if (directoryNode < 0) {
			cout << "path " << path << " is invalid" << endl;
			return false;
		}
		// cache the directory if it is not already cached
		int directoryNodeCache = getCachedDirectory(directoryNode);
		int parentDirectoryNodeCache = getCachedDirectory(parentDirectoryNode);
		// check the directory is empty
		if (cachedDirectory[directoryNodeCache]->cache.size()>0) {
			cout << "directory is not empty" << endl;
			freeCachedDirectory(directoryNodeCache);
			freeCachedDirectory(parentDirectoryNodeCache);
			return false;
		}
		iNodeList->lockINode(directoryNode);
		iNodeList->lockINode(parentDirectoryNode);
		// remove the directory from the cache
		if (cachedDirectory[parentDirectoryNodeCache]->cache.count(name) == 0) throw runtime_error("cached directory and directory tree out of sync");
		cachedDirectory[parentDirectoryNodeCache]->cache.erase(name);
		// write the cache
		cachedDirectory[parentDirectoryNodeCache]->writeCachedDirectory();
		// remove the link from the directory's iNodeList
		iNodeList->removeLinkFromINode(directoryNode);
		// remove the directory from the directory Tree
		if (!directoryTree->deleteDirectory(path,name)) throw std::runtime_error("Error deleting directory from tree");
		iNodeList->unLockINode(parentDirectoryNode);
		iNodeList->unLockINode(directoryNode);
		freeCachedDirectory(directoryNodeCache);
		freeCachedDirectory(parentDirectoryNodeCache);
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
			cout << "path " << path << " is invalid" << endl;
			return false;
		}
		// cache the parent directory (path) if it is not already cached
		int parentDirectoryNodeCache = getCachedDirectory(parentDirectoryNode);
		// check the name is not already in use
		if (cachedDirectory[parentDirectoryNodeCache]->cache.count(name) > 0) {
			cout << "file/Directory " << name << " already exists" << endl;
			freeCachedDirectory(parentDirectoryNodeCache);
			return false;
		}
		// write a new INode if this fails, busy = false, return false
		int directoryBlock = freeBlockList->getBlock();
		int directoryNode = iNodeList->writeNewINode(true,0,directoryBlock);
		iNodeList->lockINode(directoryNode);
		iNodeList->lockINode(parentDirectoryNode);
		// write the header for the directory block
		DirectoryHeaderData directoryHeader;
		directoryHeader.data.endOfLastName = directoryHeaderSize;
		directoryHeader.data.startOfLastNode = superBlock->getBlockSize();
		cachedDirectory[parentDirectoryNodeCache]->writeDirectoryHeader(directoryBlock, directoryHeader);
		// add the directory to the cache
		cachedDirectory[parentDirectoryNodeCache]->cache.insert({name,directoryNode});
		// write the cache
		cachedDirectory[parentDirectoryNodeCache]->writeCachedDirectory();
		// add the directory to the directoryTree
		directoryTree->createDirectory(path,name,directoryNode);
		iNodeList->unLockINode(parentDirectoryNode);
		iNodeList->unLockINode(directoryNode);
		freeCachedDirectory(parentDirectoryNodeCache);
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
			cout << "path " << fromPath << fromName << " is invalid" << endl;
			return false;
		}
		// check the toPath exists
		if (toDir < 0) {
			cout << "path " << toPath << " is invalid" << endl;
			return false;
		}
		// cache the to directory (path) if it is not already cached
		int toDirectoryNodeCache = getCachedDirectory(toDir);
		// cache the from path
		int fromDirectoryNodeCache = getCachedDirectory(fromDir);
		// check the toName does not already exist
		if (cachedDirectory[toDirectoryNodeCache]->cache.count(toName) > 0) {
			cout << "file/Directory " << toName << " already exists" << endl;
			freeCachedDirectory(toDirectoryNodeCache);
			freeCachedDirectory(fromDirectoryNodeCache);
			return false;
		}
		iNodeList->lockINode(toDir);
		iNodeList->lockINode(fromDir);
		// add the directory to the cached directory
		cachedDirectory[toDirectoryNodeCache]->cache.insert({toName,toMoveDir});
		// write the directory cache
		cachedDirectory[toDirectoryNodeCache]->writeCachedDirectory();
		// remove the directory from the cache
		cachedDirectory[fromDirectoryNodeCache]->cache.erase(fromName);
		// write the cached Directory
		cachedDirectory[fromDirectoryNodeCache]->writeCachedDirectory();
		// update the directory tree
		directoryTree->moveDirectory(fromPath,fromName,toPath,toName);
		iNodeList->unLockINode(fromDir);
		iNodeList->unLockINode(toDir);
		freeCachedDirectory(toDirectoryNodeCache);
		freeCachedDirectory(fromDirectoryNodeCache);
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
			cout << "path " << fromPath << fromName << " is invalid" << endl;
			return false;
		}
		// check the toPath exists
		if (toDir < 0) {
			cout << "path " << toDir << " is invalid" << endl;
			return false;
		}
		// cache the to directory (path) if it is not already cached
		int toDirectoryNodeCache = getCachedDirectory(toDir);
		// cache the from path
		int toCopyDirectoryNodeCache = getCachedDirectory(toCopyDir);
		// check the toName does not already exist
		if (cachedDirectory[toDirectoryNodeCache]->cache.count(toName) > 0) {
			cout << "file/Directory " << toPath << toDir << " already exists" << endl;
			freeCachedDirectory(toDirectoryNodeCache);
			freeCachedDirectory(toCopyDirectoryNodeCache);
			return false;
		}
		iNodeList->lockINode(toDir);
		iNodeList->lockINode(toCopyDir);
		// add the directory to the cached directory
		cachedDirectory[toDirectoryNodeCache]->cache.insert({toName,toCopyDir});
		// write the directory cache
		cachedDirectory[toDirectoryNodeCache]->writeCachedDirectory();
		// update the directory tree
		directoryTree->copyDirectory(fromPath,fromName,toPath,toName);
		// add a link to the directorys iNode
		iNodeList->addLinkToINode(toCopyDir);
		iNodeList->unLockINode(toDir);
		iNodeList->unLockINode(toCopyDir);
		freeCachedDirectory(toDirectoryNodeCache);
		freeCachedDirectory(toCopyDirectoryNodeCache);
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
	hdd->seek(superBlock->getStartOfBlock(blockNumber) + fileHeaderSize);
	for (int i=startPointer;i<endPointer;i++) {
		hdd->write(data.at(i));
	}
}

/*
 * write a file block header to disk
 */
void Directory::writeFileBlockHeader(int blockNumber, FileHeaderData fileHeaderData) {
	hdd->seek(superBlock->getStartOfBlock(blockNumber));
	for (int i=0;i<fileHeaderSize;i++) {
		hdd->write(fileHeaderData.c[i]);
	}
}

void Directory::readFileBlock(int blockNumber, string &data) {
	hdd->seek(superBlock->getStartOfBlock(blockNumber) + fileHeaderSize);
	FileHeaderData fileHeaderData = readFileBlockHeader(blockNumber);
	for (int i=0;i<fileHeaderData.lengthOfBlock;i++) {
		data += hdd->read();
	}
}

FileHeaderData Directory::readFileBlockHeader(int blockNumber) {
	hdd->seek(superBlock->getStartOfBlock(blockNumber));
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

/*
 * Used in the queue for caching directory tree
 */
struct TreeQueuePair {
	int iNodeRef;
	string path;
};

/*
 * Caches (if not already cached) a directory and returns its index, returns -1 if none are found
 */
int Directory::getCachedDirectory(int iNodeNumber) {
	for (int i=0;i<lastUsed;i++) {
		if (cachedDirectory[i]->getINodeNumber() == iNodeNumber) return i;
	}

	while (freeCaches<1) {}
	freeCaches--;

	if (lastUsed<numberOfCachedDirectorys) {
		lastUsed++;
		oldest = (oldest+1)%numberOfCachedDirectorys;
		cachedDirsUsed[lastUsed - 1] = true;
		cachedDirectory[lastUsed - 1] = new CachedDirectory(iNodeNumber,hdd,superBlock,freeBlockList,iNodeList);
		cachedDirectory[lastUsed - 1]->readCacheDirectory();
		return lastUsed - 1;
	} else {
		while (true) {
			if (!cachedDirsUsed[oldest]) {
				cachedDirsUsed[oldest] = true;
				delete cachedDirectory[oldest];
				cachedDirectory[oldest] = new CachedDirectory(iNodeNumber,hdd,superBlock,freeBlockList,iNodeList);
				cachedDirectory[oldest]->readCacheDirectory();
				int retVal = oldest;
				oldest = (oldest+1)%numberOfCachedDirectorys;
				return retVal;
			}
			oldest = (oldest+1)%numberOfCachedDirectorys;
		}
	}
	return -1;
}

void Directory::freeCachedDirectory(int i) {
	if (cachedDirsUsed[i]) {
		cachedDirsUsed[i] = false;
		freeCaches++;
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
		int dirCache = getCachedDirectory(rootINode);
		for (const auto &pair : cachedDirectory[dirCache]->cache) {
			if (iNodeList->isINodeDirectory(pair.second)) {
				directoryTree->createDirectory(currentPair.path, pair.first, pair.second);
				myQueue.push(TreeQueuePair{pair.second,currentPair.path + "/" + pair.first});
			}
		}
		myQueue.pop();
	}
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

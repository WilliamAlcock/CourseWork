#include "CachedDirectory.h"
#include <stdexcept>
#include <sstream>

CachedDirectory::CachedDirectory(int _iNodeNumber, HDD* _hdd, SuperBlock* _superBlock, FreeBlockList* _freeBlockList, INodeList* _iNodeList) {
	iNodeNumber = _iNodeNumber;
	hdd = _hdd;
	superBlock = _superBlock;
	freeBlockList = _freeBlockList;
	iNodeList = _iNodeList;
}

CachedDirectory::~CachedDirectory() {};


int CachedDirectory::getINodeNumber() {
	return iNodeNumber;
}

void CachedDirectory::writeRootNode() {
	// writes a new header for the root node
	DirectoryHeaderData directoryHeader;
	directoryHeader.data.endOfLastName = directoryHeaderSize;
	directoryHeader.data.startOfLastNode = superBlock->getBlockSize();
	writeDirectoryHeader(iNodeList->getBlockNumber(iNodeNumber,0), directoryHeader);
}

void CachedDirectory::writeCachedDirectory() {
	// get the number of nodes the directory currently has allocated (must be at least 1)
	int currentNumberOfBlocks = iNodeList->getNumberOfBlocks(iNodeNumber);
	if (currentNumberOfBlocks<1) throw runtime_error("cached Directory Node has 0 blocks !");
	// point to the first block number
	int currentNodeBlockNumber = 0;
	int currentDiskBlockNumber = iNodeList->getBlockNumber(iNodeNumber,currentNodeBlockNumber);
	// the amount of space left in the current block
	int blockSizeRemaining = superBlock->getBlockSize() - directoryHeaderSize;
	// current entry size
	int entrySize = 0;
	// current block header
	DirectoryHeaderData directoryHeaderData;
	directoryHeaderData.data.endOfLastName = directoryHeaderSize;
	directoryHeaderData.data.startOfLastNode = superBlock->getBlockSize();

	for (const auto &pair : cache) {
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
				iNodeList->addBlockToINode(iNodeNumber, currentDiskBlockNumber);
			} else {
				currentDiskBlockNumber = iNodeList->getBlockNumber(iNodeNumber,currentNodeBlockNumber);
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
void CachedDirectory::writeDirectoryEntryData(DirectoryEntryData entry, int blockNumber, int lastNodePointer) {
	hdd->seek(superBlock->getStartOfBlock(blockNumber) + lastNodePointer - directoryEntrySize);
	for (int i=0;i<directoryEntrySize;i++) {
		hdd->write(entry.c[i]);
	}
}

/*
 * Writes a directory entry name to the top of a specified block
 */
void CachedDirectory::writeDirectoryEntryName(string name, int blockNumber, int startPointer) {
	hdd->seek(superBlock->getStartOfBlock(blockNumber) + startPointer);
	for (unsigned int i=0;i<name.length();i++) {
		hdd->write(name.at(i));
	}
	hdd->write('\0');
}

/*
 * Writes a directory header to the top of a specified block
 */
void CachedDirectory::writeDirectoryHeader(int blockNumber, DirectoryHeaderData directoryHeader) {
	hdd->seek(superBlock->getStartOfBlock(blockNumber));
	for (int i=0;i<directoryHeaderSize;i++) {
		hdd->write(directoryHeader.c[i]);
	}
}

/*
 * Caches a directory represented by an iNode
 */
void CachedDirectory::readCacheDirectory() {
	// wait until the directory is unlocked
	while (iNodeList->isINodeLocked(iNodeNumber)) {}

	int numberOfBlocks = iNodeList->getNumberOfBlocks(iNodeNumber);
	if (numberOfBlocks<1) throw std::runtime_error("Directory has 0 blocks ERROR !!");

	for (int i=0;i<numberOfBlocks;i++) {
		cacheDirectoryBlock(iNodeList->getBlockNumber(iNodeNumber,i));
	}
}

/*
 * Caches a directory block
 */
void CachedDirectory::cacheDirectoryBlock(int blockNumber) {
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
		cache.insert({currentName,entry.data.iNodeRef});
		currentName.clear();
		namesPointer++;
	}
}

/*
 * Returns the header from a directory block
 */
DirectoryHeaderData CachedDirectory::readDirectoryHeader(int blockNumber) {
	hdd->seek(superBlock->getStartOfBlock(blockNumber));
	DirectoryHeaderData directoryHeader;
	for (int i=0;i<directoryHeaderSize;i++) {
		directoryHeader.c[i] = hdd->read();
	}
	return directoryHeader;
}

/*
 * Returns all the names from the beginning of a directory block
 */
string CachedDirectory::readDirectoryEntryNames(int blockNumber, int numberOfEntries, int endOfLastName) {
	hdd->seek(superBlock->getStartOfBlock(blockNumber) + directoryHeaderSize);
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
DirectoryEntryData CachedDirectory::readDirectoryEntry(int blockNumber, int entryNumber) {
	hdd->seek(superBlock->getStartOfBlock(blockNumber+1) - ((entryNumber+1) * directoryEntrySize));
	DirectoryEntryData directoryEntry;
	for (int i=0;i<directoryEntrySize;i++) {
		directoryEntry.c[i] = hdd->read();
	}
	return directoryEntry;
}

#include "INodeList.h"
#include "bitFunctions.cpp"
#include "debugFunctions.cpp"

INodeList::INodeList(HDD* _hdd, SuperBlock* _superBlock, FreeBlockList* _freeBlockList) {
	// from input
	hdd = _hdd;
	freeBlockList = _freeBlockList;
	superBlock = _superBlock;
	startingPosition = superBlock->getINodeStart();
	numberOfINodes = superBlock->getNumberOfINodes();
	// counters
	firstFreeINode = 0;
	freeINodeCounter = 0;
	lastUsedINode = 0;
	initialised = false;
}

/*
 * Reads the INode list header
 */
void INodeList::read() {
	hdd->seek(startingPosition);
	charToInt ffiNode;
	charToInt nfiNode;
	charToInt luiNode;
	// read the firstFreeINode
	for (int i=0;i<4;i++) {
		ffiNode.c[i] = hdd->read();
	}
	// read the freeINodeCounter
	for (int i=0;i<4;i++) {
		nfiNode.c[i] = hdd->read();
	}
	// read the lastUsedINode
	for (int i=0;i<4;i++) {
		luiNode.c[i] = hdd->read();
	}
	firstFreeINode = ffiNode.i;
	freeINodeCounter = nfiNode.i;
	lastUsedINode = luiNode.i;
	initialised = true;
}

/*
 * Writes the INode list header and the root Node
 */
void INodeList::write(int rootBlock) {
	hdd->seek(startingPosition);
	firstFreeINode = 0;
	freeINodeCounter = numberOfINodes;
	lastUsedINode = 0;
	initialised = true;
	writeNewINode(true,0, rootBlock);
	writeFirstFreeINode();
	writeFreeINodeCounter();
	writeLastUsedINode();
}

/*
 * Returns the number of free INodes
 */
int INodeList::getNumberOfFreeINodes() {
	if (!initialised) throw std::runtime_error("INode List not Initialised");
	return freeINodeCounter;
}

/*
 * Writes a new INode and returns it's index
 */
int INodeList::writeNewINode(bool _directory, long _size, int firstBlock) {
	if (!initialised) throw std::runtime_error("INode List not Initialised");
	if (freeINodeCounter<=0) throw std::runtime_error("No free INodes !!");

	// create a new iNode
	iNodeData currentINode;
	currentINode.data.used = 1;
	if (_directory) currentINode.data.directory = 1;
	currentINode.data.locked = 0;			// unused
	currentINode.data.spare4 = 0;			// unused
	currentINode.data.spare5 = 0;			// unused
	currentINode.data.spare6 = 0;			// unused
	currentINode.data.spare7 = 0;			// unused
	currentINode.data.spare8 = 0;			// unused
	currentINode.data.size = _size;
	currentINode.data.creationTime = 0;			// unused
	currentINode.data.modificationTime = 0;		// unused
	currentINode.data.accessTime = 0;			// unused
	currentINode.data.count = 1;
	currentINode.data.block[0] = firstBlock;
	for (int i=1;i<11;i++) {
		currentINode.data.block[i] = 0;
	}
	currentINode.data.numberOfBlocks = 1;
	// write the iNode
	writeINode(firstFreeINode, currentINode);
	// This might be better to do before marking the bit for speed but for integrity this is better
	// decrease the free space counter
	freeINodeCounter--;
	writeFreeINodeCounter();
	int iNodeToReturn = firstFreeINode;
	if (freeINodeCounter > 0) {
		if (firstFreeINode == lastUsedINode) {
			firstFreeINode ++;
			lastUsedINode ++;
			writeFirstFreeINode();
			writeLastUsedINode();
		} else {
			// find the next free INode
			int newFirstFreeINode = firstFreeINode;
			int increment = 1;
			while (((firstFreeINode + increment) < lastUsedINode) && (newFirstFreeINode == firstFreeINode)) {
				currentINode = readINode(firstFreeINode + increment);
				if (!currentINode.data.used) newFirstFreeINode = firstFreeINode + increment;
				increment++;
			}
			if (firstFreeINode + increment == lastUsedINode) {
				firstFreeINode = lastUsedINode;
			} else {
				firstFreeINode = newFirstFreeINode;
			}
			// update firstFreeINode and lastUsedINode
			writeFirstFreeINode();
			writeLastUsedINode();
		}
		return iNodeToReturn;
	} else {
		throw std::runtime_error("Disk is Full - no more free INodes !!");
		// this should really be a print statement as its only a warning not an error !!
	}
}

/*
 * Decrements an INode's count. If it is 0 deletes the INode
 */
void INodeList::removeLinkFromINode(int iNodeNumber) {
	if (!initialised) throw std::runtime_error("INode List not Initialised");
	if (!isValid(iNodeNumber)) throw std::runtime_error("INode out of range");
	if (iNodeNumber == 0) throw std::runtime_error("Cannot delete the root Node");

	// read the INode
	iNodeData currentINode = readINode(iNodeNumber);

	if (!currentINode.data.used) throw std::runtime_error("INode not in use !!");

	// decrement the count
	currentINode.data.count--;
	// if the count is 0 the iNode is no longer being used
	if (currentINode.data.count == 0) {
		// free the directly referenced blocks
		for (int i=0;i<std::min(currentINode.data.numberOfBlocks,10);i++) {
			freeBlockList->freeBlock(currentINode.data.block[i]);
		}
		// free indirectly referenced blocks
		for (int i=10;i<currentINode.data.numberOfBlocks;i++) {
			freeBlockList->freeBlock(readReferenceBlockEntry(currentINode.data.block[10],i-10));
		}
		// free the reference block
		if (currentINode.data.numberOfBlocks>10) {
			freeBlockList->freeBlock(currentINode.data.block[10]);
		}
		// set the used flag to 0
		currentINode.data.used = 0;
		// decrement the i node counter
		freeINodeCounter++;
		writeFreeINodeCounter();
		// if the iNode is less than the first free INode swap them
		if (iNodeNumber < firstFreeINode) {
			firstFreeINode = iNodeNumber;
			writeFirstFreeINode();
		}
		// if the iNode is 1 less than the last used INode decrement the lastUsedINode
		if (iNodeNumber == lastUsedINode -1) {
			lastUsedINode--;
			writeLastUsedINode();
		}
	}
	// write the iNode
	writeINode(iNodeNumber, currentINode);

	// INode may be written the wrong way round from the counters
}

/*
 * Increments an INode's count.
 */
void INodeList::addLinkToINode(int iNodeNumber) {
	if (!initialised) throw std::runtime_error("INode List not Initialised");
	if (!isValid(iNodeNumber)) throw std::runtime_error("INode out of range");
	if (iNodeNumber == 0) throw std::runtime_error("Cannot add a link to the root Node");

	// read the INode
	iNodeData currentINode = readINode(iNodeNumber);

	if (!currentINode.data.used) throw std::runtime_error("INode not in use !!");

	// increment the count
	currentINode.data.count++;
	// write the iNode
	writeINode(iNodeNumber, currentINode);
}

// **************************** iNode setters ****************************

/*
 * Removes the last block from an INode
 */
void INodeList::removeBlockFromINode(int iNodeNumber) {
	if (!initialised) throw std::runtime_error("INode List not Initialised");
	if (!isValid(iNodeNumber)) throw std::runtime_error("INode out of range");

	iNodeData currentINode = readINode(iNodeNumber);

	if (!currentINode.data.used) throw std::runtime_error("INode not in use !!");
	if (currentINode.data.numberOfBlocks < 2) throw std::runtime_error("INode only has 1 block !!");

	if (currentINode.data.numberOfBlocks > 11) {
		int numberOfRefs = readReferenceBlockHeader(currentINode.data.block[10]);
		numberOfRefs--;
		freeBlockList->freeBlock(readReferenceBlockEntry(currentINode.data.block[10],numberOfRefs));
		writeReferenceBlockHeader(currentINode.data.block[10], numberOfRefs);
	} else if (currentINode.data.numberOfBlocks == 11) {
		freeBlockList->freeBlock(readReferenceBlockEntry(currentINode.data.block[10],0));
		freeBlockList->freeBlock(currentINode.data.block[10]);
		currentINode.data.block[10] = '\0';
	} else {
		freeBlockList->freeBlock(currentINode.data.block[currentINode.data.numberOfBlocks-1]);
		currentINode.data.block[currentINode.data.numberOfBlocks-1] = '\0';
	}
	// decrement the number of blocks
	currentINode.data.numberOfBlocks--;

	// write the iNode
	writeINode(iNodeNumber, currentINode);
}

/*
 * Adds a new block number to an INode
 */
void INodeList::addBlockToINode(int iNodeNumber, int block) {
	if (!initialised) throw std::runtime_error("INode List not Initialised");
	if (!isValid(iNodeNumber)) throw std::runtime_error("INode out of range");

	iNodeData currentINode = readINode(iNodeNumber);

	if (!currentINode.data.used) throw std::runtime_error("INode not in use !!");

	// increment the number of blocks
	currentINode.data.numberOfBlocks++;

	if (currentINode.data.numberOfBlocks > 11) {
		int numberOfRefs = readReferenceBlockHeader(currentINode.data.block[10]);
		// write the block
		writeReferenceBlockEntry(currentINode.data.block[10],numberOfRefs,block);
		numberOfRefs++;
		// write the header
		writeReferenceBlockHeader(currentINode.data.block[10], numberOfRefs);
	} else if (currentINode.data.numberOfBlocks == 11) {
		currentINode.data.block[10] = freeBlockList->getBlock();
		writeReferenceBlockHeader(currentINode.data.block[10], 1);
		writeReferenceBlockEntry(currentINode.data.block[10],0,block);
	} else {
		// assign the block
		currentINode.data.block[currentINode.data.numberOfBlocks-1] = block;
	}

	// write the iNode
	writeINode(iNodeNumber, currentINode);
}

/*
 * Set an INode size
 */
void INodeList::setINodeSize(int iNodeNumber, long newSize) {
	if (!initialised) throw std::runtime_error("INode List not Initialised");
	if (!isValid(iNodeNumber)) throw std::runtime_error("INode out of range");
	if (iNodeNumber == 0) throw std::runtime_error("Cannot not edit the root node access time");

	iNodeData currentINode = readINode(iNodeNumber);

	if (!currentINode.data.used) throw std::runtime_error("INode not in use !!");

	// set the size
	currentINode.data.size = newSize;

	// write the iNode
	writeINode(iNodeNumber, currentINode);
}

/*
 * Set the lock flag on an INode
 */
void INodeList::lockINode(int iNodeNumber) {
	if (!initialised) throw std::runtime_error("INode List not Initialised");
	if (!isValid(iNodeNumber)) throw std::runtime_error("INode out of range");

	iNodeData currentINode = readINode(iNodeNumber);

	if (!currentINode.data.used) throw std::runtime_error("INode not in use !!");
	if (currentINode.data.locked) throw std::runtime_error("INode already locked !!");

	currentINode.data.locked = true;
	writeINode(iNodeNumber, currentINode);
}

/*
 * Remove the lock flag on an INode
 */
void INodeList::unLockINode(int iNodeNumber) {
	if (!initialised) throw std::runtime_error("INode List not Initialised");
	if (!isValid(iNodeNumber)) throw std::runtime_error("INode out of range");

	iNodeData currentINode = readINode(iNodeNumber);

	if (!currentINode.data.used) throw std::runtime_error("INode not in use !!");
	if (!currentINode.data.locked) throw std::runtime_error("INode already unlocked !!");

	currentINode.data.locked = false;
	writeINode(iNodeNumber, currentINode);
}

// **************************** iNode getters ****************************

/*
 * True if an iNode represents a directory, false otherwise
 */
bool INodeList::isINodeDirectory(int iNodeNumber) {
	if (!initialised) throw std::runtime_error("INode List not Initialised");
	if (!isValid(iNodeNumber)) throw std::runtime_error("INode out of range");

	iNodeData currentINode = readINode(iNodeNumber);

	if (!currentINode.data.used) throw std::runtime_error("INode not in use !!");

	return currentINode.data.directory;
}

/*
 * Return the size the file represented by an INode
 */
bool INodeList::getINodeLockStatus(int iNodeNumber) {
	if (!initialised) throw std::runtime_error("INode List not Initialised");
	if (!isValid(iNodeNumber)) throw std::runtime_error("INode out of range");

	iNodeData currentINode = readINode(iNodeNumber);

	if (!currentINode.data.used) throw std::runtime_error("INode not in use !!");

	return currentINode.data.locked;
}


/*
 * Return the size the file represented by an INode
 */
long INodeList::getINodeSize(int iNodeNumber) {
	if (!initialised) throw std::runtime_error("INode List not Initialised");
	if (!isValid(iNodeNumber)) throw std::runtime_error("INode out of range");

	iNodeData currentINode = readINode(iNodeNumber);

	if (!currentINode.data.used) throw std::runtime_error("INode not in use !!");

	return currentINode.data.size;
}

/*
 * Return the creation time of an INode
 */
long INodeList::getINodeCreationTime(int iNodeNumber) {
	if (!initialised) throw std::runtime_error("INode List not Initialised");
	if (!isValid(iNodeNumber)) throw std::runtime_error("INode out of range");

	iNodeData currentINode = readINode(iNodeNumber);

	if (!currentINode.data.used) throw std::runtime_error("INode not in use !!");

	return currentINode.data.creationTime;
}

/*
 * Return the modification time of an INode
 */
long INodeList::getINodeModificationTime(int iNodeNumber) {
	if (!initialised) throw std::runtime_error("INode List not Initialised");
	if (!isValid(iNodeNumber)) throw std::runtime_error("INode out of range");

	iNodeData currentINode = readINode(iNodeNumber);

	if (!currentINode.data.used) throw std::runtime_error("INode not in use !!");

	return currentINode.data.accessTime;
}

/*
 * Return the access time of an INode
 */
long INodeList::getINodeAccessTime(int iNodeNumber) {
	if (!initialised) throw std::runtime_error("INode List not Initialised");
	if (!isValid(iNodeNumber)) throw std::runtime_error("INode out of range");

	iNodeData currentINode = readINode(iNodeNumber);

	if (!currentINode.data.used) throw std::runtime_error("INode not in use !!");

	return currentINode.data.accessTime;
}

int INodeList::getNumberOfBlocks(int iNodeNumber) {
	if (!initialised) throw std::runtime_error("INode List not Initialised");
	if (!isValid(iNodeNumber)) throw std::runtime_error("INode out of range");

	iNodeData currentINode = readINode(iNodeNumber);

	if (!currentINode.data.used) throw std::runtime_error("INode not in use !!");

	return currentINode.data.numberOfBlocks;
}

/*
 * Return a specified block index of an INode
 */
int INodeList::getBlockNumber(int iNodeNumber, int blockNumber) {
	if (!initialised) throw std::runtime_error("INode List not Initialised");
	if (!isValid(iNodeNumber)) throw std::runtime_error("INode out of range");

	iNodeData currentINode = readINode(iNodeNumber);

	if (!currentINode.data.used) throw std::runtime_error("INode not in use !!");
	if ((blockNumber < 0) || (blockNumber >= currentINode.data.numberOfBlocks)) throw std::runtime_error("Block number out of range !!");

	if (blockNumber > 9) {
		return readReferenceBlockEntry(currentINode.data.block[10],blockNumber-10);
	} else {
		return currentINode.data.block[blockNumber];
	}
}

// **************************** private functions ****************************

/*
 * Writes the firstFreeINode pointer
 */
void INodeList::writeFirstFreeINode() {
	hdd->seek(startingPosition);
	// write the firstFreeINode
	charToInt ffiNode;
	ffiNode.i = firstFreeINode;
	for (int i=0;i<4;i++) {
		hdd->write(ffiNode.c[i]);
	}
}

/*
 * Writes the freeINodeCounter
 */
void INodeList::writeFreeINodeCounter() {
	hdd->seek(startingPosition + 4);
	// write the freeINodeCounter
	charToInt nfiNode;
	nfiNode.i = freeINodeCounter;
	for (int i=0;i<4;i++) {
		hdd->write(nfiNode.c[i]);
	}
}

/*
 * Writes the lastUsedINode pointer
 */
void INodeList::writeLastUsedINode() {
	hdd->seek(startingPosition + 8);
	// write the lastUsedINode
	charToInt luiNode;
	luiNode.i = lastUsedINode;
	for (int i=0;i<4;i++) {
		hdd->write(luiNode.c[i]);
	}
}

/*
 * Writes an iNode at a specified offset
 */
void INodeList::writeINode(int iNodeNumber, iNodeData iNode) {
	// align the disk
	hdd->seek(startingPosition + iNodeHeaderSize + (iNodeNumber * iNodeSize));
	for (int i=0;i<iNodeSize;i++) {
		hdd->write(iNode.c[i]);
	}
}

/*
 * Reads and returns an iNode at a specified offset
 */
iNodeData INodeList::readINode(int iNodeNumber) {
	// align the disk
	hdd->seek(startingPosition + iNodeHeaderSize + (iNodeNumber * iNodeSize));
	iNodeData iNode;
	for (int i=0;i<iNodeSize;i++) {
		iNode.c[i] = hdd->read();
	}
	return iNode;
}

/*
 * True if an iNode is between 0 and the lastUsedINode, false otherwise
 */
bool INodeList::isValid(int iNodeNumber) {
	return ((iNodeNumber >=0) && (iNodeNumber < lastUsedINode));
}

/*
 * Reads and returns a reference block header
 */
int INodeList::readReferenceBlockHeader(int blockNumber) {
	// read the header
	hdd->seek(superBlock->getFirstBlockStart() + (superBlock->getBlockSize() * blockNumber));
	charToInt header;
	for (int i = 0;i<4;i++) {
		header.c[i] = hdd->read();
	}
	return header.i;
}

/*
 * Writes a reference block header
 */
void INodeList::writeReferenceBlockHeader(int blockNumber, int header) {
	// write the header
	hdd->seek(superBlock->getFirstBlockStart() + (superBlock->getBlockSize() * blockNumber));
	charToInt writeHeader;
	writeHeader.i = header;
	for (int i = 0;i<4;i++) {
		hdd->write(writeHeader.c[i]);
	}
}

/*
 * Reads and returns a reference block entry
 */
int INodeList::readReferenceBlockEntry(int blockNumber, int referenceNumber) {
	// read the reference
	hdd->seek(superBlock->getFirstBlockStart() + (superBlock->getBlockSize() * blockNumber) +  iNodeReferenceBlockHeaderSize + (referenceNumber * iNodeReferenceBlockEntrySize));
	charToInt entry;
	for (int i = 0;i<4;i++) {
		entry.c[i] = hdd->read();
	}
	return entry.i;
}

/*
 * Writes a reference block entry
 */
void INodeList::writeReferenceBlockEntry(int blockNumber, int referenceNumber, int entry) {
	// write the reference
	hdd->seek(superBlock->getFirstBlockStart() + (superBlock->getBlockSize() * blockNumber) +  iNodeReferenceBlockHeaderSize + (referenceNumber * iNodeReferenceBlockEntrySize));
	charToInt writeEntry;
	writeEntry.i = entry;
	for (int i = 0;i<4;i++) {
		hdd->write(writeEntry.c[i]);
	}
}

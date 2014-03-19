#include "SuperBlock.h"
#include "FreeBlockList.h"
#include "INodeList.h"
#include <iostream>
#include <stdexcept>

SuperBlock::SuperBlock(HDD* _hdd) {
	hdd = _hdd;
	initialised = false;
}

/*
 * write the SuperBlock
 */
void SuperBlock::write(int _blockSize) {
	data.data.numberOfBlocks = hdd->capacity() / _blockSize;
	data.data.blockSize = _blockSize;
	// 1 bit for every block + the free block list header
	data.data.freeBlockListSizeBytes = ((data.data.numberOfBlocks + 7) /8) + freeBlockHeaderSize;
	// calculate the iNodeList size so it pads out to the end of the nearest block
	int iNodeListBytes = ((data.data.numberOfBlocks / 8) * iNodeSize) + iNodeHeaderSize + iNodeFooterSize;
	data.data.systemBlocks = ((superBlockSize + iNodeListBytes + data.data.freeBlockListSizeBytes) + data.data.blockSize - 1) / data.data.blockSize;
	data.data.iNodeListBytes = (data.data.systemBlocks * data.data.blockSize) - superBlockSize - data.data.freeBlockListSizeBytes;
	data.data.numberOfINodes = (data.data.iNodeListBytes - iNodeHeaderSize - iNodeFooterSize) / iNodeSize;
	hdd->reset();
	for (int i=0;i<superBlockSize;i++) {
		hdd->write(data.c[i]);
	}
	initialised = true;
}

/*
 * Read the SuperBlock
 */
void SuperBlock::read() {
	hdd->reset();
	for (int i=0;i<superBlockSize;i++) {
		data.c[i] = hdd->read();
	}
	initialised = true;
}

/*
 * Return the Number of Blocks
 */
int SuperBlock::getNumberOfBlocks() const {
	if (!initialised) throw std::runtime_error("Super Block not Initialised");

	return data.data.numberOfBlocks - data.data.systemBlocks;
}

/*
 * Return the Block Size
 */
int SuperBlock::getBlockSize() const {
	if (!initialised) throw std::runtime_error("Super Block not Initialised");

	return data.data.blockSize;
}

/*
 * Return the FreeBlockList start position
 */
int SuperBlock::getFreeBlockStart() const {
	if (!initialised) throw std::runtime_error("Super Block not Initialised");

	return superBlockSize;
}

/*
 * Return the INodeList Start position
 */
int SuperBlock::getINodeStart() const {
	if (!initialised) throw std::runtime_error("Super Block not Initialised");

	return superBlockSize + data.data.freeBlockListSizeBytes;
}

/*
 * Return the Number of INodes
 */
int SuperBlock::getNumberOfINodes() const {
	if (!initialised) throw std::runtime_error("Super Block not Initialised");

	return data.data.numberOfINodes;
}

/*
 * Return the First Block position
 */
int SuperBlock::getFirstBlockStart() const {
	if (!initialised) throw std::runtime_error("Super Block not Initialised");

	return superBlockSize + data.data.freeBlockListSizeBytes + data.data.iNodeListBytes;
}

/*
 * Returns the starting position of a block
 */
int SuperBlock::getStartOfBlock(int blockNumber) {
	if (!initialised) throw std::runtime_error("Super Block not Initialised");

	return (getFirstBlockStart() + (blockNumber * getBlockSize()));
}


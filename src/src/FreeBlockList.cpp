#include "FreeBlockList.h"
#include "bitFunctions.cpp"
#include <iostream>
#include <stdexcept>
#include <sstream>

FreeBlockList::FreeBlockList(HDD* _hdd, int _numberOfBlocks, int _blockSize, int _startingPosition) {
	// from input
	hdd = _hdd;
	numberOfBlocksOnDisk = _numberOfBlocks;
	blockSize = _blockSize;
	startingPosition = _startingPosition;
	// counters
	firstFreeBlock = 0;
	freeBlockCounter = 0;
	initialised = false;
}

/*
 * Read the FreeBlockList header
 */
void FreeBlockList::read() {
	hdd->seek(startingPosition);
	charToInt ffb;
	charToInt nfb;
	// read the firstFreeBlock
	for (int i=0;i<4;i++) {
		ffb.c[i] = hdd->read();
	}
	// read the freeBlockCounter
	for (int i=0;i<4;i++) {
		nfb.c[i] = hdd->read();
	}
	firstFreeBlock = ffb.i;
	freeBlockCounter = nfb.i;
	initialised = true;
}

/*
 * Write a new FreeBlockList
 */
void FreeBlockList::write() {
	hdd->seek(startingPosition);
	firstFreeBlock = 0;
	freeBlockCounter = numberOfBlocksOnDisk;
	writeFirstFreeBlock();
	writeFreeBlockCounter();
	// write the list
	for (int i=0;i<numberOfBlocksOnDisk;i++) {
		hdd->write('\0');
	}
	initialised = true;
}

/*
 * Write the firstFreeBlock pointer
 */
void FreeBlockList::writeFirstFreeBlock() {
	hdd->seek(startingPosition);
	// write the first free block
	charToInt ffb;
	ffb.i = firstFreeBlock;
	for (int i=0;i<4;i++) {
		hdd->write(ffb.c[i]);
	}
}

/*
 * Write the freeBlockCounter
 */
void FreeBlockList::writeFreeBlockCounter() {
	hdd->seek(startingPosition + 4);
	charToInt nfb;
	nfb.i = freeBlockCounter;
	// write the numberOfFreeBlocks
	for (int i=0;i<4;i++) {
		hdd->write(nfb.c[i]);
	}
}

/*
 * Return the number of free blocks
 */
int FreeBlockList::getNumberOfFreeBlocks() {
	if (!initialised) throw std::runtime_error("Free Block List not Initialised");

	return freeBlockCounter;
}

/*
 * Returns and marks as read the firstFreeBlock
 */
int FreeBlockList::getBlock() {
	if (!initialised) throw std::runtime_error("Free Block List not Initialised");
	if (freeBlockCounter <= 0) throw std::runtime_error("No free blocks !!");

	// go to the byte with the first free block
	hdd->seek(startingPosition + freeBlockHeaderSize + (firstFreeBlock/8));
	// read the byte in
	char tempChar = hdd->read();
	// calculate bit to flip
	int bitToFlip = firstFreeBlock % 8;

	// check if bit is already 1, if not flip the bit
	if (getBit(tempChar, bitToFlip)) throw std::runtime_error("block is already marked as used, FreeDiskBlock corrupt !!");

	tempChar = flipBit(tempChar, bitToFlip);
	// realign the disk
	hdd->seek(startingPosition + freeBlockHeaderSize + (firstFreeBlock/8));
	// write the byte
	hdd->write(tempChar);
	// This might be better to do before marking the bit for speed but for integrity this is better
	// decrease the free space counter
	freeBlockCounter--;
	writeFreeBlockCounter();
	// realign the disk
	hdd->seek(startingPosition + freeBlockHeaderSize + (firstFreeBlock/8) + 1);
	// if there are free blocks still on the disk find the next one
	int blockToReturn = firstFreeBlock;
	if (freeBlockCounter > 0) {
		// finding the next freeBlock
		int newFirstFreeBlock = firstFreeBlock;
		int bitIncrement = 1;
		int bitInBlock = (firstFreeBlock + bitIncrement) % 8;
		// check ypu have not reached the last bit in the freeblocklist
		while (((firstFreeBlock + bitIncrement) < numberOfBlocksOnDisk) && (newFirstFreeBlock == firstFreeBlock)) {
			// read in the next byte if you have reached it
			if (bitInBlock == 0) tempChar = hdd->read();
			if (!getBit(tempChar, bitInBlock)) {
				// found the next free block !!
				newFirstFreeBlock = firstFreeBlock + bitIncrement;
			} else {
				bitIncrement++;
				bitInBlock = (firstFreeBlock + bitIncrement) % 8;
			}
		}
		// if no new free block has been found throw an error the counter is out of sync with the bitmap
		if (newFirstFreeBlock == firstFreeBlock) throw std::runtime_error("number of free blocks counter out of sync with freeDiskBlock , FreeDiskBlock corrupt !!");

		firstFreeBlock = newFirstFreeBlock;
		writeFirstFreeBlock();
	} else {
		std::cout << "Disk is Full - no free blocks !!" << std::endl;
	}
	return blockToReturn;
}

/*
 * Marks a specified block free
 */
void FreeBlockList::freeBlock(int blockNumber) {
	if (!initialised) throw std::runtime_error("Free Block List not Initialised");
	if ((blockNumber < 0) || (blockNumber >= numberOfBlocksOnDisk)) throw std::runtime_error("Block number out of range");

	hdd->seek(startingPosition + freeBlockHeaderSize + (blockNumber/8));
	char tempChar = hdd->read();
	int bitToFlip = blockNumber % 8;

	if (!getBit(tempChar, bitToFlip)) throw std::runtime_error("block is already marked as free, FreeDiskBlock corrupt !!");

	tempChar =  flipBit(tempChar, bitToFlip);
	// realign the disk
	hdd->seek(startingPosition + freeBlockHeaderSize + (blockNumber/8));
	// write the byte
	hdd->write(tempChar);
	// increase the free space counter
	freeBlockCounter++;
	writeFreeBlockCounter();
	// if this block as less than the freeBlock set freeBlock to this block
	firstFreeBlock = std::min(firstFreeBlock, blockNumber);
	// rewrite the firstFreeBlock
	writeFirstFreeBlock();
}

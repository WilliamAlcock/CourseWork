#include "INodeList.h"
#include <iostream>

inline void printINode(iNodeData iNode) {
	std::cout << "Used = " << iNode.data.used << std::endl;
	std::cout << "Directory = " << iNode.data.directory << std::endl;
	std::cout << "Locked = " << iNode.data.locked << std::endl;
	std::cout << "spare4 = " << iNode.data.spare4 << std::endl;
	std::cout << "spare5 = " << iNode.data.spare5 << std::endl;
	std::cout << "spare6 = " << iNode.data.spare6 << std::endl;
	std::cout << "spare7 = " << iNode.data.spare7 << std::endl;
	std::cout << "spare8 = " << iNode.data.spare8 << std::endl;
	std::cout << "size = " << iNode.data.size << std::endl;
	std::cout << "creationTime = " << iNode.data.creationTime << std::endl;
	std::cout << "modificationTime = " << iNode.data.modificationTime << std::endl;
	std::cout << "accessTime = " << iNode.data.accessTime << std::endl;
	std::cout << "count = " << iNode.data.count << std::endl;
	for (int i=0;i<11;i++) {
		std::cout << "block[" << i << "] = " << iNode.data.block[i] << std::endl;
	}
	std::cout << "numberOfBlocks = "<< iNode.data.numberOfBlocks << std::endl;
}

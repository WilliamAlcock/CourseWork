#pragma once
#include "DirectoryNode.h"
#include "INodeList.h"
#include <vector>
#include <iostream>

using namespace std;

class DirectoryTree {
public:
	DirectoryTree(int iNodeDef);
	~DirectoryTree();
	bool isValidPath(string path);		// Path to be divided by /							// Tested
	bool createDirectory(string path, string name, int iNodeRef);							// Tested
	bool deleteDirectory(string path, string name);											// Tested
	bool moveDirectory(string fromPath, string fromName, string toPath, string toName);
	bool copyDirectory(string fromPath, string fromName, string toPath, string toName);
	int getPathNode(string path);															// Tested
private:
	DirectoryNode* rootNode;
	DirectoryNode* currentNode;
};

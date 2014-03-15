#pragma once
#include <iostream>
#include <unordered_map>

using namespace std;

class DirectoryNode {
public:
	DirectoryNode(int _iNodeRef, DirectoryNode* _parent);
	~DirectoryNode();
	DirectoryNode* addChild(string _name, int iNodeRef);	// Tested
	bool hasChild(string _name);							// Tested
	void removeChild(string _name);							// Tested
	DirectoryNode* getChild(string _name);					// Tested
	int getINodeRef();										// Tested
	DirectoryNode* getParent();								// Tested
private:
	int iNodeRef;
	DirectoryNode* parent;
	unordered_map<string,DirectoryNode*> children;
};

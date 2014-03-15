#include "DirectoryNode.h"

DirectoryNode::DirectoryNode(int _iNodeRef,DirectoryNode* _parent) {
	iNodeRef = _iNodeRef;
	parent = _parent;
	children.clear();
}

DirectoryNode::~DirectoryNode() {
	for (const auto &Pair : children) {
		delete &Pair.second;
	}
}

DirectoryNode* DirectoryNode::addChild(string _name, int _iNodeRef) {
	DirectoryNode* myNode = new DirectoryNode(_iNodeRef, this);
	children.insert({_name, myNode});
	return myNode;
}

bool DirectoryNode::hasChild(string _name) {
	return children.count(_name);
}

void DirectoryNode::removeChild(string _name) {
	children.erase(_name);
}

DirectoryNode* DirectoryNode::getChild(string _name) {
	return children[_name];
}

int DirectoryNode::getINodeRef() {
	return iNodeRef;
}

DirectoryNode* DirectoryNode::getParent() {
	return parent;
}


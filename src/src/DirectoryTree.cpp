#include "DirectoryTree.h"
#include "DirectoryNode.h"
#include "INodeList.h"

using namespace std;

DirectoryTree::DirectoryTree(int iNodeRef) {
	rootNode = new DirectoryNode(iNodeRef,rootNode);
	currentNode = rootNode;
}

DirectoryTree::~DirectoryTree() {
	delete rootNode;
}

bool DirectoryTree::isValidPath(string path) {
	string delimiter = "/";
	currentNode = rootNode;
	size_t pos = 0;
	string directory;
	while ((pos = path.find(delimiter)) != string::npos) {
	    directory = path.substr(0, pos);
	    if (currentNode->hasChild(directory)) {
	    	currentNode = currentNode->getChild(directory);
	    } else {
	    	return false;
	    }
	    path.erase(0, pos + delimiter.length());
	}
	return true;
}

bool DirectoryTree::createDirectory(string path, string name, int iNodeRef) {
	if (!isValidPath(path)) return false;

	if (currentNode->hasChild(name)) return false;

	currentNode->addChild(name,iNodeRef);
	return true;
}

bool DirectoryTree::deleteDirectory(string path, string name) {
	if (!isValidPath(path)) return false;

	if (!currentNode->hasChild(name)) return false;

	currentNode->removeChild(name);
	return true;
}

bool DirectoryTree::moveDirectory(string fromPath, string fromName, string toPath, string toName) {
	if (!copyDirectory(fromPath, fromName, toPath, toName)) return false;

	if (!deleteDirectory(fromPath, fromName)) return false;
	return true;
}

bool DirectoryTree::copyDirectory(string fromPath, string fromName, string toPath, string toName) {
	if (!isValidPath(fromPath)) return false;

	if (!currentNode->hasChild(fromName)) return false;
	DirectoryNode* nodeToMove = currentNode->getChild(fromName);

	if (!isValidPath(toPath)) return false;
	if (currentNode->hasChild(toName)) return false;

	currentNode->addChild(toName,nodeToMove->getINodeRef());
	return true;
}

int DirectoryTree::getPathNode(string path) {
	if (!isValidPath(path)) return -1;

	return currentNode->getINodeRef();
}

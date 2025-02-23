#include <iostream>
#include "BinarySearchTree.h"


BinarySearchTree::BinarySearchTree()
	:_rootNode(nullptr), _size(0)
{

}
BinarySearchTree::~BinarySearchTree()
{
	//후위 순회하면서 할당한 노드들을 해제한다.
	deleteNode(_rootNode);
}

void BinarySearchTree::insert(int val)
{
	Node* newNode = new Node;
	newNode->data = val;
	_size++;

	if (_rootNode == nullptr)
	{
		_rootNode = newNode;
		return;
	}
	Node* curNode = _rootNode;
	while (1)
	{
		if (val < curNode->data)
		{
			if (curNode->left == nullptr)	//삽입할 곳을 찾은 경우
				break;
			curNode = curNode->left;
		}
		else if (val > curNode->data)
		{
			if (curNode->right == nullptr)	//삽입할 곳을 찾은 경우
				break;
			curNode = curNode->right;
		}
	}
	newNode->parent = curNode;

	if (val < newNode->parent->data)
	{
		curNode->left = newNode;
	}
	else if (val > newNode->parent->data)
	{
		curNode->right = newNode;
	}
	return;
}

bool BinarySearchTree::erase(int val)
{
	//우선 지울 값이 있는 노드를 찾는다.
	Node* delNode = search(val);

	if (delNode == nullptr)
		return false;
	_size--;

	//자식 노드가 없는 경우
	if (delNode->left == nullptr && delNode->right == nullptr)
	{
		if (_rootNode == delNode)
		{
			delete _rootNode;
			_rootNode = nullptr;
			return true;
		}
		//부모가 날 가리키던 포인터를 nullptr로 값을 바꿔준다.
		if (val < delNode->parent->data)
		{
			delNode->parent->left = nullptr;
		}
		else if (val > delNode->parent->data)
		{
			delNode->parent->right = nullptr;
		}
		delete delNode;
		return true;
	}
	else if (delNode->left != nullptr && delNode->right == nullptr)	//왼쪽 자식 노드만 존재하는 경우
	{
		if (_rootNode == delNode)
		{
			_rootNode = delNode->left;
			delete _rootNode->parent;
			_rootNode->parent = nullptr;
			return true;
		}

		if (val < delNode->parent->data)
		{
			delNode->parent->left = delNode->left;
		}
		else if (val > delNode->parent->data)
		{
			delNode->parent->right = delNode->left;
		}

		delNode->left->parent = delNode->parent;
		delete delNode;
		return true;
	}
	else if (delNode->left == nullptr && delNode->right != nullptr) //오른쪽 자식 노드만 존재하는 경우
	{
		if (_rootNode == delNode)
		{
			_rootNode = delNode->right;
			delete _rootNode->parent;
			_rootNode->parent = nullptr;
			return true;
		}
		if (val < delNode->parent->data)
		{
			delNode->parent->left = delNode->right;
		}
		else if (val > delNode->parent->data)
		{
			delNode->parent->right = delNode->right;
		}

		delNode->right->parent = delNode->parent;
		delete delNode;
		return true;
	}
	else //자식 노드가 둘 다 있는 경우
	{
		Node* curNode;
		curNode = delNode->right;

		while (1)
		{
			if (curNode->left == nullptr)
				break;
			curNode = curNode->left;
		}
		delNode->data = curNode->data;

		if (curNode->data < curNode->parent->data)
		{
			curNode->parent->left = nullptr;
		}
		else if (curNode->data = curNode->parent->data)
		{
			curNode->parent->right = nullptr;
		}
		delete curNode;
		return true;
	}

	return false;
}

Node* BinarySearchTree::search(int val)
{
	Node* curNode = _rootNode;
	while (1)
	{
		if (curNode->data == val)
			return curNode;

		if (val < curNode->data)
		{
			if (curNode->left == nullptr)	//끝까지 갔는데 찾는 데이터가 없는 경우
				return nullptr;
			curNode = curNode->left;
		}
		else if (val > curNode->data)
		{
			if (curNode->right == nullptr)	//끝까지 갔는데 찾는 데이터가 없는 경우
				return nullptr;
			curNode = curNode->right;
		}
	}
}

bool BinarySearchTree::isEmpty()
{
	return _size == 0;
}

void BinarySearchTree::printAllNode()
{
	print(_rootNode);
}

int BinarySearchTree::size()
{
	return _size;
}

void BinarySearchTree::print(Node* node)
{
	if (node == nullptr)
		return;
	print(node->left);
	std::cout << node->data << " ";
	print(node->right);
}

void BinarySearchTree::deleteNode(Node* node)
{
	if (node == nullptr)
		return;
	deleteNode(node->left);
	deleteNode(node->right);
	delete node;
}
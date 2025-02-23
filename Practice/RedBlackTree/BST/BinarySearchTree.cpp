#include <iostream>
#include "BinarySearchTree.h"


BinarySearchTree::BinarySearchTree()
	:_rootNode(nullptr), _size(0)
{

}
BinarySearchTree::~BinarySearchTree()
{
	//���� ��ȸ�ϸ鼭 �Ҵ��� ������ �����Ѵ�.
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
			if (curNode->left == nullptr)	//������ ���� ã�� ���
				break;
			curNode = curNode->left;
		}
		else if (val > curNode->data)
		{
			if (curNode->right == nullptr)	//������ ���� ã�� ���
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
	//�켱 ���� ���� �ִ� ��带 ã�´�.
	Node* delNode = search(val);

	if (delNode == nullptr)
		return false;
	_size--;

	//�ڽ� ��尡 ���� ���
	if (delNode->left == nullptr && delNode->right == nullptr)
	{
		if (_rootNode == delNode)
		{
			delete _rootNode;
			_rootNode = nullptr;
			return true;
		}
		//�θ� �� ����Ű�� �����͸� nullptr�� ���� �ٲ��ش�.
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
	else if (delNode->left != nullptr && delNode->right == nullptr)	//���� �ڽ� ��常 �����ϴ� ���
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
	else if (delNode->left == nullptr && delNode->right != nullptr) //������ �ڽ� ��常 �����ϴ� ���
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
	else //�ڽ� ��尡 �� �� �ִ� ���
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
			if (curNode->left == nullptr)	//������ ���µ� ã�� �����Ͱ� ���� ���
				return nullptr;
			curNode = curNode->left;
		}
		else if (val > curNode->data)
		{
			if (curNode->right == nullptr)	//������ ���µ� ã�� �����Ͱ� ���� ���
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
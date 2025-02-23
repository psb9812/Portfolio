#include <iostream>
#include "RBTree.h"

//public
RBTree::RBTree()
	:_rootNode(nullptr), _size(0)
{
	_nil = new Node;
	_nil->color = BLACK;
}
RBTree::~RBTree()
{
	//���� ��ȸ�ϸ鼭 �Ҵ��� ������ �����Ѵ�.
	deleteNode(_rootNode);
}

void RBTree::insert(int val)
{
	Node* insertNode = bstInsert(val);

	insertBalancingProc(insertNode);

}

bool RBTree::erase(int val)
{
	return false;
}
RBTree::Node* RBTree::search(int val)
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

bool RBTree::isEmpty()
{
	return _size == 0;
}

void RBTree::printAllNode()
{
	print(_rootNode);
}

int RBTree::size() { return _size; }

//private

RBTree::Node* RBTree::bstInsert(int val)
{
	Node* newNode = new Node;
	newNode->data = val;
	newNode->color = RED;
	newNode->left = _nil;
	newNode->right = _nil;
	_size++;

	if (_rootNode == nullptr)
	{
		_rootNode = newNode;
		return newNode;
	}
	Node* curNode = _rootNode;
	while (1)
	{
		if (val < curNode->data)
		{
			if (curNode->left == _nil)	//������ ���� ã�� ���
				break;
			curNode = curNode->left;
		}
		else if (val > curNode->data)
		{
			if (curNode->right == _nil)	//������ ���� ã�� ���
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
	return newNode;
}

bool RBTree::bstErase(int val)
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

void RBTree::print(Node* node)
{
	if (node == _nil)
		return;

	print(node->left);
	std::cout << node->data << '(' << ((node->color == RED) ? 'R' : 'B') << ')' << " ";
	print(node->right);
}

void RBTree::deleteNode(Node* node)
{
	if (node == _nil)
		return;
	deleteNode(node->left);
	deleteNode(node->right);
	delete node;
}

void RBTree::rotateL(Node* node)
{
	if (_rootNode != node)
	{
		node->right->parent = node->parent;
		if (node->parent->left == node)
			node->parent->left = node->right;
		else
			node->parent->right = node->right;
	}
	else
	{
		node->right->parent = nullptr;
		_rootNode = _rootNode->right;
	}

	Node* sNode = node->right->left;

	node->right->left = node;
	node->parent = node->right;

	node->right = sNode;
	sNode->parent = node;
}

void RBTree::rotateR(Node* node)
{
	if (_rootNode != node)
	{
		node->left->parent = node->parent;
		if (node->parent->left == node)
			node->parent->left = node->left;
		else
			node->parent->right = node->left;
	}
	else 
	{
		node->left->parent = nullptr;
		_rootNode = _rootNode->left;
	}
	Node* sNode = node->left->right;

	node->left->right = node;
	node->parent = node->left;

	node->left = sNode;
	sNode->parent = node;

}

void RBTree::insertBalancingProc(Node* node)
{ 
	if (node == _rootNode)
	{
		node->color = BLACK;
		return;
	}

	//�θ� ���̸� ��Ģ�� �������� �����Ƿ� �ٷ� �����Ѵ�.
	if (node->parent->color == BLACK)
		return;
	
	Node* pNode = node->parent;		//�θ� ���
	Node* gpNode = pNode->parent;	//�Ҿƹ��� ���
	Node* uNode = nullptr;			//���� ���
	if (pNode == gpNode->right)
	{
		uNode = gpNode->left;
	}
	else
	{
		uNode = gpNode->right;
	}

	//case1. �θ� ����, ���̵� ������
	if (uNode->color == RED)
	{
		//�θ�� ������ ������ �ٲ������ �Ҿƹ����� ����� �ٲ۴� 
		pNode->color = BLACK; uNode->color = BLACK; gpNode->color = RED;
		//�Ҿƹ����� �������� �ٽ� �뷱�� �˻�
		insertBalancingProc(gpNode);
		return;
	}
	else    //���� ��尡 ���� ���
	{
		//���� ��尡 �θ� ����� ���� �ڽ��̰� �θ� ��尡 �Ҿƹ��� ����� ���� �ڽ��� ���
		if (gpNode->left == pNode && pNode->left == node)
		{
			//�θ�� �Ҿƹ����� ���� �ٲ۴�.
			gpNode->color = RED;
			pNode->color = BLACK;
			//�Ҿƹ��� �������� ���������� ȸ���Ѵ�.
			rotateR(gpNode);
		}
		else if (gpNode->right == pNode && pNode->right == node)	//���� ��尡 �θ��� ������ �ڽ��̰�, �� �θ� �Ҿƹ����� ������ �ڽ��� ���
		{
			//�θ�� �Ҿƹ����� ���� �ٲ۴�.
			gpNode->color = RED;
			pNode->color = BLACK;
			//�Ҿƹ��� �������� �������� ȸ���Ѵ�.
			rotateL(gpNode);
		}
		else if (gpNode->left == pNode && pNode->right == node)	//���� ��尡 �θ��� ������ �ڽ��̰�, �� �θ�� �Ҿƹ����� ���� �ڽ��� ���.
		{
			rotateL(pNode);
			//�θ�� �Ҿƹ����� ���� �ٲ۴�.
			gpNode->color = RED;
			pNode->color = BLACK;
			//�Ҿƹ��� �������� ���������� ȸ���Ѵ�.
			rotateR(gpNode);
		}
		else if (gpNode->right == pNode && pNode->left == node)	//���� ��尡 �θ��� ���� �ڽ��̰�, �� �θ�� �Ҿƹ����� ������ �ڽ��� ���.
		{
			rotateR(pNode);
			//�θ�� �Ҿƹ����� ���� �ٲ۴�.
			gpNode->color = RED;
			node->color = BLACK;
			//�Ҿƹ��� �������� �������� ȸ���Ѵ�.
			rotateL(gpNode);
		}
		_rootNode->color = BLACK;
		return;
	}
}
void RBTree::eraseBalancingProc(Node* node)
{

}
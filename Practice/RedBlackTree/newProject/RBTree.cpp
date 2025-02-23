#include <iostream>
#include "RBTree.h"

//public
RBTree::RBTree()
	:_rootNode(nullptr), _size(0), _eraseColor(RED)
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
	Node* targetNode = bstErase(val);
	if (targetNode == nullptr)
	{
		if (_rootNode == nullptr)
			return true;
		else
			return false;
	}

	if(_eraseColor == BLACK)
		eraseBalancingProc(targetNode);

	if(_rootNode->color != BLACK)
		_rootNode->color = BLACK;
	return true;
}
RBTree::Node* RBTree::search(int val)
{
	if (_rootNode == nullptr) return nullptr;
	Node* curNode = _rootNode;
	
	while (1)
	{
		if (curNode->data == val)
			return curNode;

		if (val < curNode->data)
		{
			if (curNode->left == _nil)	//������ ���µ� ã�� �����Ͱ� ���� ���
				return nullptr;
			curNode = curNode->left;
		}
		else if (val > curNode->data)
		{
			if (curNode->right == _nil)	//������ ���µ� ã�� �����Ͱ� ���� ���
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

RBTree::Node* RBTree::bstErase(int val)
{
	//�켱 ���� ���� �ִ� ��带 ã�´�.
	Node* delNode = search(val);

	if (delNode == nullptr)
		return nullptr;
	_size--;

	//�ڽ� ��尡 ���� ���
	if (delNode->left == _nil && delNode->right == _nil)
	{
		//��Ʈ��带 ���� ��
		if (_rootNode == delNode)
		{
			delete _rootNode;
			_rootNode = nullptr;
			return nullptr;
		}
		//�θ� �� ����Ű�� �����͸� nil���� ���� �ٲ��ش�.
		if (val < delNode->parent->data)
		{
			delNode->parent->left = _nil;
		}
		else if (val > delNode->parent->data)
		{
			delNode->parent->right = _nil;
		}
		_nil->parent = delNode->parent;
		_eraseColor = delNode->color;
		delete delNode;
		return _nil;
	}
	else if (delNode->left != _nil && delNode->right == _nil)	//���� �ڽ� ��常 �����ϴ� ���
	{
		Node* replaceNode;
		if (_rootNode == delNode)
		{
			_rootNode = delNode->left;
			delete _rootNode->parent;
			_rootNode->parent = nullptr;
			_eraseColor = delNode->color;
			return _rootNode;
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
		replaceNode = delNode->left;
		_eraseColor = delNode->color;
		delete delNode;
		return replaceNode;
	}
	else if (delNode->left == _nil && delNode->right != _nil) //������ �ڽ� ��常 �����ϴ� ���
	{
		if (_rootNode == delNode)
		{
			_rootNode = delNode->right;
			delete _rootNode->parent;
			_rootNode->parent = nullptr;
			_eraseColor = delNode->color;
			return _rootNode;
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
		Node* replaceNode = delNode->right;
		_eraseColor = delNode->color;
		delete delNode;
		return replaceNode;
	}
	else //�ڽ� ��尡 �� �� �ִ� ���
	{
		Node* curNode;
		curNode = delNode->right;

		while (1)
		{
			if (curNode->left == _nil)
				break;
			curNode = curNode->left;
		}

		delNode->data = curNode->data;

		if (curNode == delNode->right)
			curNode->parent->right = curNode->right;
		else
			curNode->parent->left = curNode->right;

		curNode->right->parent = curNode->parent;

		_eraseColor = curNode->color;
		Node* retNode = curNode->right;

		delete curNode;
		return retNode;
	}
}

void RBTree::print(Node* node)
{
	if (node == _nil || node == nullptr)
		return;

	print(node->left);
	std::cout << node->data << '(' << ((node->color == RED) ? 'R' : 'B') << ')' << " ";
	print(node->right);
}

void RBTree::deleteNode(Node* node)
{
	if (node == _nil || node == nullptr)
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
			node->color = BLACK;
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
	//�θ� �������� ���� ���ʿ� �ִ��� �����ʿ� �ִ��� ������ ��.
	//�׷��� ���� �ڽ��� �������� ���������� �Ǵ��ϴ� ���� ����
	Node* parentNode = node->parent;
	//�θ� ���ٸ� �ٷ� ����������.
	if (parentNode == nullptr)
		return;

	bool isRightChild = (parentNode->right == node) ? true : false;
	Node* siblingNode = isRightChild ? (parentNode->left) : (parentNode->right);

	//2.1 ���� ����� �ڽ��� ������ ���
	if (node->color == RED)
	{
		node->color = BLACK;
		return;
	}
	else if (siblingNode->color == RED)		//2.2 ���� ����� ������ ����
	{
		//������ ������ �ٲ۴�.
		siblingNode->color = BLACK;
		//�θ� �������� ��/�� ȸ��
		if (isRightChild)
			rotateR(parentNode);
		else
			rotateL(parentNode);
		node->parent = parentNode;		//���� node�� nil����� ȸ���ϸ鼭 �θ� ���� �� �ִ� ���ɼ��� �����ϴ� �κ�
		//���� �θ� ����� �ٲ۴�.
		parentNode->color = RED;
		//�׸��� ���� ��带 Ÿ������ �ٽ� �˻�.
		eraseBalancingProc(node);
		return;
	}
	else if (siblingNode->color == BLACK && siblingNode->right->color == BLACK && siblingNode->left->color == BLACK)		//2.3 ���� ����� ������ ���̰� ������ ���� �ڽ��� ��
	{
		//������ ����� �ٲ㼭 �� ���ΰ� ���� ������ �� ���� ���� ��.
		siblingNode->color = RED;

		//������ �׷��� �Ҿƹ��� ���ο��� �θ� ������ �� �� �θ��� ������ �� �뷱���� ���� �����Ƿ�
		//�θ� �������� �ٽ� �˻��Ѵ�.
		eraseBalancingProc(parentNode);
		return;
	}
	else if (siblingNode->color == BLACK && 
		(!isRightChild ? siblingNode->left : siblingNode->right)->color == RED && 
		(isRightChild ? siblingNode->left : siblingNode->right)->color == BLACK) //2.4 ���� ����� ������ ���̰� ������ ���ڽ��� ����, �����ڽ��� ��
	{
		(!isRightChild ? siblingNode->left : siblingNode->right)->color = BLACK;
		siblingNode->color = RED;

		if (isRightChild)
			rotateL(siblingNode);
		else
			rotateR(siblingNode);

		node->parent = parentNode;		//���� node�� nil����� ȸ���ϸ鼭 �θ� ���� �� �ִ� ���ɼ��� �����ϴ� �κ�

		eraseBalancingProc(node);	//�� ��ʹ� 2.5���̽��� �ٷ� �� ����.
		return;
	}
	else if (siblingNode->color == BLACK &&
		(isRightChild ? siblingNode->left : siblingNode->right)->color == RED)	//2.5 ���� ����� ������ ���̰� ������ �����ڽ��� ����
	{
		siblingNode->color = parentNode->color;
		parentNode->color = BLACK;

		(isRightChild ? siblingNode->left : siblingNode->right)->color = BLACK;
		
		if (isRightChild)
			rotateR(parentNode);
		else
			rotateL(parentNode);
		node->parent = parentNode;
	}
}

void RBTree::toVector(std::vector<int>& vector)
{
	writeInVector(_rootNode, vector);
}

void RBTree::writeInVector(Node* node, std::vector<int>& vector)
{
	if (node == _nil || node == nullptr)
		return;

	writeInVector(node->left, vector);
	vector.push_back(node->data);
	writeInVector(node->right, vector);
}
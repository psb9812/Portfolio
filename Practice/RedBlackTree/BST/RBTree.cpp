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
	//후위 순회하면서 할당한 노드들을 해제한다.
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
			if (curNode->left == _nil)	//삽입할 곳을 찾은 경우
				break;
			curNode = curNode->left;
		}
		else if (val > curNode->data)
		{
			if (curNode->right == _nil)	//삽입할 곳을 찾은 경우
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

	//부모가 블랙이면 규칙을 위반하지 않으므로 바로 리턴한다.
	if (node->parent->color == BLACK)
		return;
	
	Node* pNode = node->parent;		//부모 노드
	Node* gpNode = pNode->parent;	//할아버지 노드
	Node* uNode = nullptr;			//삼촌 노드
	if (pNode == gpNode->right)
	{
		uNode = gpNode->left;
	}
	else
	{
		uNode = gpNode->right;
	}

	//case1. 부모가 레드, 삼촌도 레드라면
	if (uNode->color == RED)
	{
		//부모와 삼촌을 블랙으로 바꿔버리고 할아버지를 레드로 바꾼다 
		pNode->color = BLACK; uNode->color = BLACK; gpNode->color = RED;
		//할아버지를 기준으로 다시 밸런싱 검사
		insertBalancingProc(gpNode);
		return;
	}
	else    //삼촌 노드가 블랙인 경우
	{
		//삽입 노드가 부모 노드의 왼쪽 자식이고 부모 노드가 할아버지 노드의 왼쪽 자식인 경우
		if (gpNode->left == pNode && pNode->left == node)
		{
			//부모와 할아버지의 색을 바꾼다.
			gpNode->color = RED;
			pNode->color = BLACK;
			//할아버지 기준으로 오른쪽으로 회전한다.
			rotateR(gpNode);
		}
		else if (gpNode->right == pNode && pNode->right == node)	//삽입 노드가 부모의 오른쪽 자식이고, 그 부모도 할아버지의 오른쪽 자식인 경우
		{
			//부모와 할아버지의 색을 바꾼다.
			gpNode->color = RED;
			pNode->color = BLACK;
			//할아버지 기준으로 왼쪽으로 회전한다.
			rotateL(gpNode);
		}
		else if (gpNode->left == pNode && pNode->right == node)	//삽입 노드가 부모의 오른쪽 자식이고, 그 부모는 할아버지의 왼쪽 자식인 경우.
		{
			rotateL(pNode);
			//부모와 할아버지의 색을 바꾼다.
			gpNode->color = RED;
			pNode->color = BLACK;
			//할아버지 기준으로 오른쪽으로 회전한다.
			rotateR(gpNode);
		}
		else if (gpNode->right == pNode && pNode->left == node)	//삽입 노드가 부모의 왼쪽 자식이고, 그 부모는 할아버지의 오른쪽 자식인 경우.
		{
			rotateR(pNode);
			//부모와 할아버지의 색을 바꾼다.
			gpNode->color = RED;
			node->color = BLACK;
			//할아버지 기준으로 왼쪽으로 회전한다.
			rotateL(gpNode);
		}
		_rootNode->color = BLACK;
		return;
	}
}
void RBTree::eraseBalancingProc(Node* node)
{

}
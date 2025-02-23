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
			if (curNode->left == _nil)	//끝까지 갔는데 찾는 데이터가 없는 경우
				return nullptr;
			curNode = curNode->left;
		}
		else if (val > curNode->data)
		{
			if (curNode->right == _nil)	//끝까지 갔는데 찾는 데이터가 없는 경우
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

RBTree::Node* RBTree::bstErase(int val)
{
	//우선 지울 값이 있는 노드를 찾는다.
	Node* delNode = search(val);

	if (delNode == nullptr)
		return nullptr;
	_size--;

	//자식 노드가 없는 경우
	if (delNode->left == _nil && delNode->right == _nil)
	{
		//루트노드를 지울 때
		if (_rootNode == delNode)
		{
			delete _rootNode;
			_rootNode = nullptr;
			return nullptr;
		}
		//부모가 날 가리키던 포인터를 nil노드로 값을 바꿔준다.
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
	else if (delNode->left != _nil && delNode->right == _nil)	//왼쪽 자식 노드만 존재하는 경우
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
	else if (delNode->left == _nil && delNode->right != _nil) //오른쪽 자식 노드만 존재하는 경우
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
	else //자식 노드가 둘 다 있는 경우
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
			node->color = BLACK;
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
	//부모 기준으로 내가 왼쪽에 있는지 오른쪽에 있는지 나눠야 함.
	//그러기 위해 자신이 왼쪽인지 오른쪽인지 판단하는 변수 정의
	Node* parentNode = node->parent;
	//부모가 없다면 바로 나가버린다.
	if (parentNode == nullptr)
		return;

	bool isRightChild = (parentNode->right == node) ? true : false;
	Node* siblingNode = isRightChild ? (parentNode->left) : (parentNode->right);

	//2.1 삭제 노드의 자식이 레드인 경우
	if (node->color == RED)
	{
		node->color = BLACK;
		return;
	}
	else if (siblingNode->color == RED)		//2.2 삭제 노드의 형제가 레드
	{
		//형제를 블랙으로 바꾼다.
		siblingNode->color = BLACK;
		//부모를 기준으로 좌/우 회전
		if (isRightChild)
			rotateR(parentNode);
		else
			rotateL(parentNode);
		node->parent = parentNode;		//현재 node가 nil노드라면 회전하면서 부모가 꼬일 수 있는 가능성을 제거하는 부분
		//기존 부모를 레드로 바꾼다.
		parentNode->color = RED;
		//그리고 기준 노드를 타겟으로 다시 검사.
		eraseBalancingProc(node);
		return;
	}
	else if (siblingNode->color == BLACK && siblingNode->right->color == BLACK && siblingNode->left->color == BLACK)		//2.3 삭제 노드의 형제가 블랙이고 형제의 양쪽 자식이 블랙
	{
		//형제를 레드로 바꿔서 내 라인과 형제 라인의 블랙 수를 같게 함.
		siblingNode->color = RED;

		//하지만 그러면 할아버지 라인에서 부모 라인을 볼 때 부모의 형제와 블랙 밸런스가 맞지 않으므로
		//부모를 기준으로 다시 검사한다.
		eraseBalancingProc(parentNode);
		return;
	}
	else if (siblingNode->color == BLACK && 
		(!isRightChild ? siblingNode->left : siblingNode->right)->color == RED && 
		(isRightChild ? siblingNode->left : siblingNode->right)->color == BLACK) //2.4 삭제 노드의 형제가 블랙이고 형제의 왼자식이 레드, 오른자식이 블랙
	{
		(!isRightChild ? siblingNode->left : siblingNode->right)->color = BLACK;
		siblingNode->color = RED;

		if (isRightChild)
			rotateL(siblingNode);
		else
			rotateR(siblingNode);

		node->parent = parentNode;		//현재 node가 nil노드라면 회전하면서 부모가 꼬일 수 있는 가능성을 제거하는 부분

		eraseBalancingProc(node);	//이 재귀는 2.5케이스로 바로 갈 것임.
		return;
	}
	else if (siblingNode->color == BLACK &&
		(isRightChild ? siblingNode->left : siblingNode->right)->color == RED)	//2.5 삭제 노드의 형제가 블랙이고 형제의 오른자식이 레드
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
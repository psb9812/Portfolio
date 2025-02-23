#pragma once

/*
	레드 블랙 트리입니다.

	- 함수 소개

*/

class RBTree
{
public:
	enum NODE_COLOR
	{
		BLACK = 0,
		RED
	};

	struct Node
	{
		int data = 0;
		NODE_COLOR color;
		Node* parent = nullptr;
		Node* left = nullptr;
		Node* right = nullptr;
	};

	RBTree();
	~RBTree();

	void insert(int val);
	bool erase(int val);
	Node* search(int val);
	bool isEmpty();
	void printAllNode();
	int size();

private:
	Node* bstInsert(int val);
	bool bstErase(int val);

	void print(Node* node);
	void deleteNode(Node* node);

	void rotateL(Node* node);
	void rotateR(Node* node);

	void insertBalancingProc(Node* node);
	void eraseBalancingProc(Node* node);

private:
	Node* _rootNode;
	int _size;
	Node* _nil;
};
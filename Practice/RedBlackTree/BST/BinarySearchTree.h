#pragma once

/*
	�ߺ��� ������� �ʴ� ���� Ž�� Ʈ���Դϴ�.

	- �Լ� �Ұ�
	void insert(int val) : data���� ���� Ž�� Ʈ���� �����մϴ�.

	bool erase(int val) : data���� ���� Ž�� Ʈ������ �����մϴ�. data���� ���� ���Ұ� ���� ��� false�� �����մϴ�.

	Node* search(int val) : data���� ���� ���Ұ� �����ϴ��� Ž���մϴ�.

	bool isEmpty() : ���� Ž�� Ʈ���� ����ִ����� �����մϴ�.

	void printAllNode() : ��� ���Ҹ� ������ݴϴ�.(���� ��ȸ)

	int size() : ���� ������ ������ ����� �ݴϴ�.
*/
struct Node
{
	int data = 0;
	Node* parent = nullptr;
	Node* left = nullptr;
	Node* right = nullptr;
};
class BinarySearchTree
{
public:

	BinarySearchTree();
	~BinarySearchTree();

	void insert(int val);
	bool erase(int val);
	Node* search(int val);
	bool isEmpty();
	void printAllNode();
	int size();

private:
	void print(Node* node);
	void deleteNode(Node* node);

private:
	Node* _rootNode;
	int _size;
};
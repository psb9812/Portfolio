#pragma once

/*
	중복을 허락하지 않는 이진 탐색 트리입니다.

	- 함수 소개
	void insert(int val) : data값을 이진 탐색 트리에 삽입합니다.

	bool erase(int val) : data값을 이진 탐색 트리에서 제거합니다. data값을 가진 원소가 없을 경우 false를 리턴합니다.

	Node* search(int val) : data값을 가진 원소가 존재하는지 탐색합니다.

	bool isEmpty() : 이진 탐색 트리가 비어있는지를 리턴합니다.

	void printAllNode() : 모든 원소를 출력해줍니다.(중위 순회)

	int size() : 현재 원소의 개수를 출력해 줍니다.
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
#pragma once
#include <list>
#include <queue>
#include "framework.h"
#include "Node.h"
#include "Point.h"

#define GRID_WIDTH 100
#define GRID_HEIGHT 100

class AStarAlgorithm
{
private:
	const int _maxSearchCount;	//�ִ� Ž�� �ܰ� ��
	int _searchCount;
	bool _isFinding;
	HBRUSH _closeListTileBrush;
	HBRUSH _openListTileBrush;
	HPEN _redPen;
	HPEN _finalRoutePen;

	int dx[8] = { 0,  1,  1, 1, 0, -1, -1, -1 };
	int dy[8] = { -1, -1, 0, 1, 1, 1,  0, -1 };
	std::priority_queue<Node*, std::vector<Node*>, cmp> _openList;
	std::list<Point> _openList_Point;
	std::list<Node*> _closeList;

	
public:
	AStarAlgorithm();
	~AStarAlgorithm();


	//�� �ܰ辿 ���� ã�� �Լ� 
	void PathFind(HWND hWnd, HDC hdc, Node (&tileMap)[GRID_HEIGHT][GRID_WIDTH], int gridSize, int gridPosX, int gridPosY, Point startPos, Point endPos);

	//�� ���� ���� ã�� �Լ�
	void PathFindAll(HWND hWnd, HDC hdc, Node (&tileMap)[GRID_HEIGHT][GRID_WIDTH], int gridSize, int gridPosX, int gridPosY, Point startPos, Point endPos);

	//������ ���� ��ã�� �Լ�
	void PathFindNoRender(Node(&tileMap)[GRID_HEIGHT][GRID_WIDTH], Point startPos, Point endPos);

	bool isFinding() const;

	//openList�� closeList�� ��� ����� �θ� ����Ű�� ������ �׸��� �Լ�
	void Render(HDC hdc, Node (&tileMap)[GRID_HEIGHT][GRID_WIDTH], int gridSize, int gridPosX, int gridPosY);
private:

	//��ã�⸦ ������ �� ���ҽ� �����ϴ� �Լ�
	void PathFindEndProcess();
};
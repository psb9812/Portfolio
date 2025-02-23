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
	const int _maxSearchCount;	//최대 탐색 단계 수
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


	//한 단계씩 길을 찾는 함수 
	void PathFind(HWND hWnd, HDC hdc, Node (&tileMap)[GRID_HEIGHT][GRID_WIDTH], int gridSize, int gridPosX, int gridPosY, Point startPos, Point endPos);

	//한 번에 길을 찾는 함수
	void PathFindAll(HWND hWnd, HDC hdc, Node (&tileMap)[GRID_HEIGHT][GRID_WIDTH], int gridSize, int gridPosX, int gridPosY, Point startPos, Point endPos);

	//렌더링 없는 길찾기 함수
	void PathFindNoRender(Node(&tileMap)[GRID_HEIGHT][GRID_WIDTH], Point startPos, Point endPos);

	bool isFinding() const;

	//openList와 closeList의 모든 노드의 부모를 가리키는 라인을 그리는 함수
	void Render(HDC hdc, Node (&tileMap)[GRID_HEIGHT][GRID_WIDTH], int gridSize, int gridPosX, int gridPosY);
private:

	//길찾기를 종료할 때 리소스 정리하는 함수
	void PathFindEndProcess();
};
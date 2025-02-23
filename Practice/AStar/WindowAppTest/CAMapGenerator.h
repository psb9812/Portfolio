#pragma once
#include "Node.h"
#include "AStarAlgorithm.h"

class CAMapGenerator
{
public:
	CAMapGenerator(Node(&tile)[GRID_HEIGHT][GRID_WIDTH], int width, int height, size_t _deathLimit, size_t _birthLimit, int numOfSteps);
	~CAMapGenerator();
	void GenerateMap();

private:
	void DoSimulationStep();
	int CountEmptyNeighbours(int x, int y);

private:
	const int _chanceToStartAlive = 45;
	const size_t _width;
	const size_t _height;
	Node(&_tileMap)[GRID_HEIGHT][GRID_WIDTH];

	size_t _deathLimit;
	size_t _birthLimit;
	int _numberOfSteps;
};
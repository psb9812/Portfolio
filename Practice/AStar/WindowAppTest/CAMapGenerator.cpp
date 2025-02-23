#include "CAMapGenerator.h"
#include <random>

CAMapGenerator::CAMapGenerator(Node(&tile)[GRID_HEIGHT][GRID_WIDTH], int width, int height, size_t deathLimit, size_t birthLimit, int numOfSteps)
    :_tileMap(tile), _width(width), _height(height), _deathLimit(deathLimit), _birthLimit(birthLimit), _numberOfSteps(numOfSteps)
{

}

CAMapGenerator::~CAMapGenerator()
{

}

void CAMapGenerator::GenerateMap()
{
    srand(time(NULL));

    //모든 맵을 비운다.
    for (int i = 0; i < _height; ++i) 
    {
        for (int j = 0; j < _width; ++j) 
        {
            _tileMap[i][j].isObstacle = 0;
        }
    }

    //랜덤으로 장애물을 설치한다.
    for (int i = 0; i < _height; ++i)
    {
        for (int j = 0; j < _width; ++j)
        {
            if(rand() % 100 < _chanceToStartAlive)
                _tileMap[i][j].isObstacle = true;
        }
    }

    for (int i = 0; i < _numberOfSteps; i++)
    {
        DoSimulationStep();
    }
}

void CAMapGenerator::DoSimulationStep()
{
    //2차원 배열 생성
    bool** newMap = new bool* [_height];

    for (int i = 0; i < _height; ++i) {
        newMap[i] = new bool[_width];
    }

    for (int y = 0; y < _height; y++) {
        for (int x = 0; x < _width; x++) {
            int nbs = CountEmptyNeighbours(x, y);
            //The new value is based on our simulation rules 
            //First, if a cell is alive but has too few neighbours, kill it. 
            if (_tileMap[x][y].isObstacle) {
                if (nbs < _deathLimit) {
                    newMap[x][y] = false;
                }
                else {
                    newMap[x][y] = true;
                }
            }
            else {
                if (nbs > _birthLimit) {
                    newMap[x][y] = true;
                }
                else {
                    newMap[x][y] = false;
                }
            }
        }
    }
    
    for (int i = 0; i < _height; ++i)
    {
        for (int j = 0; j < _width; ++j)
        {
            _tileMap[i][j].isObstacle = newMap[i][j];
        }
    }

    for (int i = 0; i < _height; ++i) {
        delete[] newMap[i];
    }
}

int CAMapGenerator::CountEmptyNeighbours(int x, int y)
{
    int count = 0;
    for (int i = -1; i < 2; i++) 
    {
        for (int j = -1; j < 2; j++) 
        {
            int neighbour_x = x + j;
            int neighbour_y = y + i;
            if (i == 0 && j == 0) continue;
            else if (neighbour_x < 0 || neighbour_y < 0 || neighbour_x >= _width || neighbour_y >= _height) 
            {
                count = count + 1;
            }
            else if (_tileMap[neighbour_x][neighbour_y].isObstacle) 
            {
                count = count + 1;
            }
        }
    }
    return count;
}
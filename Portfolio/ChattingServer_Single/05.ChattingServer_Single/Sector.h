#pragma once
#include <list>
#include <Windows.h>

class User;
struct SectorAround;

struct SectorPos
{
	WORD _x;
	WORD _y;

	SectorPos()
		:_x(0), _y(0) {}

	SectorPos(WORD x, WORD y)
		:_x(x), _y(y) {}


	bool operator==(SectorPos& other)
	{
		return _x == other._x && _y == other._y;
	}
	bool operator!=(SectorPos& other)
	{
		return !(_x == other._x && _y == other._y);
	}

	bool GetSectorAround(SectorAround& pSectorAround);
};

struct SectorAround
{
	int count;
	SectorPos around[9];

	SectorAround() = default;
};

struct Sector
{
	std::list<User*>	_users;
};
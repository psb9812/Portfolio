#pragma once
#include <list>
#include <Windows.h>
#include "MemoryLog.h"

class User;
struct SectorAround;
inline MemoryLog g_mLog;

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
	SRWLOCK				_lock;
	SectorPos			_myPos;

	Sector();

	void PushBack(User* pUser);
	void Remove(User* pUser);
	void Clear();

	__forceinline void ELock() { AcquireSRWLockExclusive(&_lock); }
	__forceinline void EUnlock() { ReleaseSRWLockExclusive(&_lock); }
	__forceinline void SLock() { AcquireSRWLockShared(&_lock); }
	__forceinline void SUnlock() { ReleaseSRWLockShared(&_lock); }
};
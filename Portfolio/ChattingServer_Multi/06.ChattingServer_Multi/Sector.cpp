#include "pch.h"
#include "Sector.h"


bool SectorPos::GetSectorAround(SectorAround& sectorAround)
{
	if (_x < 0 || _x >= MAX_SECTOR_X || _y < 0 || _y >= MAX_SECTOR_Y)
	{
		return false;
	}

	int count = 9;
	if (_x == 0 || _x == MAX_SECTOR_X - 1)
	{
		count -= 3;
	}
	if (_y == 0 || _y == MAX_SECTOR_Y - 1)
	{
		count -= 3;
	}
	if ((_x == 0 && _y == 0)
		|| (_x == 0 && _y == MAX_SECTOR_Y - 1)
		|| (_x == MAX_SECTOR_X - 1 && _y == 0)
		|| (_x == MAX_SECTOR_X - 1 && _y == MAX_SECTOR_Y - 1))
	{
		count++;
	}

	sectorAround.count = count;
	//락 획득 순서와 동일하게 주변 섹터를 구한다
	WORD d_x[9] = { -1, 0, 1, -1, 0, 1, -1, 0, 1 };
	WORD d_y[9] = { -1, -1, -1, 0, 0, 0, 1, 1, 1 };
	int curid_x = 0;
	for (int i = 0; i < 9; i++)
	{
		WORD curSectorX = _x + d_x[i];
		WORD curSectorY = _y + d_y[i];

		//섹터 유효 범위 검사
		if (curSectorX < 0 || curSectorX >= MAX_SECTOR_X || 
			curSectorY < 0 || curSectorY >= MAX_SECTOR_Y)
		{
			continue;
		}
		//주변 섹터 저장
		sectorAround.around[curid_x++] = { curSectorX, curSectorY };
	}
	return true;
}

Sector::Sector()
{
	InitializeSRWLock(&_lock);
}

void Sector::PushBack(User* pUser)
{
	_users.push_back(pUser);
}

void Sector::Remove(User* pTargetUser)
{
	_users.remove(pTargetUser);
}

void Sector::Clear()
{
	_users.clear();
}



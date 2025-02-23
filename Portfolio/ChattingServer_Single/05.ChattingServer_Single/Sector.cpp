#include "Sector.h"
#include "CommonDefine.h"

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

	WORD d_x[9] = { -1, 0, 1, -1, 0, 1, -1, 0, 1 };
	WORD d_y[9] = { -1, -1, -1, 0, 0, 0, 1, 1, 1 };
	int curid_x = 0;
	for (int i = 0; i < 9; i++)
	{
		WORD curSectorX = _x + d_x[i];
		WORD curSectorY = _y + d_y[i];

		if (curSectorX < 0 || curSectorX >= MAX_SECTOR_X || curSectorY < 0 || curSectorY >= MAX_SECTOR_Y)
		{
			continue;
		}
		sectorAround.around[curid_x++] = { curSectorX, curSectorY };
	}
	return true;
}

#include "User.h"
#include "CommonDefine.h"

User::User()
	:_sessionID(-1), _userID(-1), _curSector(MAX_SECTOR_X, MAX_SECTOR_Y), _isLogin(false),
	_accountNo(NULL)
{
}

void User::Init(INT64 sessionID)
{
	_sessionID = sessionID;
	_userID = _userIDCounter++;
	_lastRecvTime = GetTickCount64();
	_curSector._x = MAX_SECTOR_X;
	_curSector._y = MAX_SECTOR_Y;
	_isLogin = false;
	_accountNo = 0;

}

void User::UpdateLastRecvTime()
{
	_lastRecvTime = GetTickCount64();
}

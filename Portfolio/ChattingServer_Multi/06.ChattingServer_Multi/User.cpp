#include "pch.h"
#include "User.h"

User::User()
	:_sessionID(-1), _userID(-1), _curSector(MAX_SECTOR_X, MAX_SECTOR_Y), _isLogin(false),
	_accountNo(NULL)
{
	InitializeSRWLock(&_lock);
}

void User::Init(INT64 sessionID)
{
	_sessionID = sessionID;
	_userID = _userIDCounter++;
	_lastRecvTime = GetTickCount64();
}

void User::UpdateLastRecvTime()
{
	_lastRecvTime = GetTickCount64();
}

void User::ELock()
{
	AcquireSRWLockExclusive(&_lock);
}

void User::EUnlock()
{
	ReleaseSRWLockExclusive(&_lock);
}

void User::SLock()
{
	AcquireSRWLockShared(&_lock);
}

void User::SUnlock()
{
	ReleaseSRWLockShared(&_lock);
}

#include "pch.h"
#include "User.h"

User::User()
	:_sessionID(-1), _userID(-1), _isLogin(false), _accountNo(NULL)
{
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
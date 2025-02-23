#include "NetServer.h"
#include "Contents.h"
#include <algorithm>

Contents::Contents(NetServer* server, INT32 fps, bool serialFlag)
	:_pNetworkLib(server), _FPS(fps), _serialFlag(serialFlag)
{
	InitializeSRWLock(&_contentsLock);
}

Contents::~Contents()
{
}

void Contents::MoveContents(INT64 sessionID, INT32 destContentsID)
{
	_pNetworkLib->RequestMoveContents(sessionID, destContentsID);
}

void Contents::ExitContents(INT64 sessionID)
{
	if (!DeleteSessionID(sessionID))
	{
		__debugbreak();
	}

	OnLeave(sessionID);
}

void Contents::DisconnectInContents(INT64 sessionID)
{
	_pNetworkLib->Disconnect(sessionID);
}

bool Contents::NotifyEnterContents(INT64 sessionID)
{
	InsertSessionID(sessionID);
	IUser* pUser = _pNetworkLib->GetUser(sessionID);
	if (pUser == nullptr)
	{
		return false;
	}
	OnEnter(sessionID, pUser);
	return true;
}

bool Contents::NotifyLeaveContents(INT64 sessionID)
{
	if (!DeleteSessionID(sessionID))
	{
		return false;
	}
	OnLeave(sessionID);
	return true;
}

bool Contents::DeleteSessionID(INT64 sessionID)
{
	auto iter = std::find(_sessionIDList.begin(), _sessionIDList.end(), sessionID);
	if (iter == _sessionIDList.end())
	{
		return false;
	}

	_sessionIDList.erase(iter);

	return true;
}

void Contents::InsertSessionID(INT64 sessionID)
{
	_sessionIDList.push_back(sessionID);
}

IUser* Contents::SearchUser(INT64 sessionID)
{
	SLock();
	IUser* pUser = _userContainer[sessionID];
	SUnlock();

	return pUser;
}

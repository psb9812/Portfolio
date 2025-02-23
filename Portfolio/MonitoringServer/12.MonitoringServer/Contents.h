#pragma once
#include "Windows.h"
#include <list>
#include <unordered_map>

class IUser;
class Message;
class NetServer;

class Contents
{
public:
	Contents(NetServer* server, INT32 fps, bool serialFlag);
	virtual ~Contents();

	void MoveContents(INT64 sessionID, INT32 destContentsID);
	void ExitContents(INT64 sessionID);
	void DisconnectInContents(INT64 sessionID);
	bool NotifyEnterContents(INT64 sessionID);
	bool NotifyLeaveContents(INT64 sessionID);

	inline bool IsSerial() { return _serialFlag; }

	inline void ELock() { AcquireSRWLockExclusive(&_contentsLock); }
	inline void EUnlock() { ReleaseSRWLockExclusive(&_contentsLock); }
	inline void SLock() { AcquireSRWLockShared(&_contentsLock); }
	inline void SUnlock() { ReleaseSRWLockShared(&_contentsLock); }

	inline INT32 GetFPS() const { return _FPS; }

	bool DeleteSessionID(INT64 sessionID);
	void InsertSessionID(INT64 sessionID);

	IUser* SearchUser(INT64 sessionID);


public:
	/////////////////////////////
	// Event callback function
	/////////////////////////////
	virtual void OnRecv(INT64 sessionID, Message* pMessage) = 0;
	virtual void OnEnter(INT64 sessionID, IUser* pUser) = 0;
	virtual void OnLeave(INT64 sessionID) = 0;
	virtual void OnDisconnect(INT64 sessionID) = 0;
	virtual void OnUpdate(INT64 lateTime) = 0;

private:
	SRWLOCK _contentsLock;
	std::list<INT64> _sessionIDList;
	INT32 _FPS;
	bool _serialFlag;
protected:
	NetServer* _pNetworkLib;
	std::unordered_map<INT64, IUser*> _userContainer;
public:
	LONG _prevFrameCnt = 0;
	LONG _frameCnt = 0;
};
#pragma once
#include "NetServer.h"
#include "TLSMemoryPool.h"
#include <unordered_map>
#include <vector>
#include "Contents.h"
#include "CommonProtocol.h"
#include "HWPerformanceCounter.h"
#include "CpuUsage.h"
#include "ProcessPerformanceCounter.h"

class User;

class EchoServer : public NetServer
{
public:
	EchoServer();
	virtual ~EchoServer();

	bool Start(const WCHAR* configFileName) override;
	void Stop() override;

	bool ServerControl();

	inline LONG GetCurrentUserNum() const { return _currentUserNum; }
	inline bool GetServerAlive() const { return _isServerAlive; }
	inline int GetLoginContentsSessionNum() { return _contentsHashTable[static_cast<int>(ContentsID::Login)]->GetSessionNum();  }
	inline int GetEchoContentsSessionNum() { return _contentsHashTable[static_cast<int>(ContentsID::Echo)]->GetSessionNum();  }
	inline int GetLoginContentsFPS() { return _contentsHashTable[static_cast<int>(ContentsID::Login)]->_prevFrameCnt; }
	inline int GetEchoContentsFPS() { return _contentsHashTable[static_cast<int>(ContentsID::Echo)]->_prevFrameCnt; }
	inline int GetUserPoolCapacity() const { return _userPool.GetCapacityCount(); }
	inline int GetUserPoolUsage() const { return _userPool.GetUseCount(); }
	inline int GetUserNum() const { return _currentUserNum; }

	inline void IncreaseCurrentUserNum() { InterlockedIncrement(&_currentUserNum); }
	inline void DecreaseCurrentUserNum() { InterlockedDecrement(&_currentUserNum); }

	bool InsertUser(INT64 sessionID, User* pUser);
	bool DeleteUser(INT64 sessionID);
	void FreeUserMemory(User* pUser);
	IUser* GetUser(INT64 sessionID) override;
protected:

	//-----------------------
	//     콜백 함수      
	//-----------------------
	bool OnConnectionRequest(const WCHAR* connectIP, INT32 connectPort) override;
	void OnAccept(INT64 sessionID) override;
	void OnRelease(INT64 sessionID) override;
	void OnRecv(INT64 sessionID, Message* pMessage) override;
	void OnSend(INT64 sessionID, INT32 sendSize) override;
	void OnTime() override;
	void OnError(INT32 errorCode, WCHAR* comment) override;


private:
	std::unordered_map<INT64, User*> _userContainer;
	SRWLOCK _userContainerLock;

	LONG _currentUserNum = 0;

	TLSMemoryPool<User> _userPool;
	bool _isServerAlive = false;

public:
	HWPerformanceCounter _hwPC;
	ProcessPerformanceCounter _pPC;
	CpuUsage _cpuUsage;
};


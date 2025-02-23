#pragma once
#include <unordered_map>
#include "NetServer.h"
#include "TLSMemoryPool.h"
#include "MemoryPool.h"
#include "Job.h"
#include "User.h"
#include "CommonDefine.h"
#include "Sector.h"
#include "HWPerformanceCounter.h"
#include "ProcessPerformanceCounter.h"
#include "CpuUsage.h"

class ChattingServer :public NetServer
{
public:

	ChattingServer();
	virtual ~ChattingServer();

	bool Start() override;

	void Stop() override;

	LONG GetJobQueueSize() const;
	int GetJobPoolSize() const;
	int GetJobPoolCapacity() const;
	int GetUserPoolCapacity() const;
	int GetUserPoolSize() const;
	LONG GetUserCount() const;
	INT64 GetUpdateTPS() const;
	bool GetServerAlive() const;

protected:
	bool OnConnectionRequest(const wchar_t* connectIP, int connectPort) override;

	void OnAccept(__int64 sessionID) override;

	void OnRelease(__int64 sessionID) override;

	void OnRecv(__int64 sessionID, Message* pMessage) override;

	void OnSend(__int64 sessionID, int sendSize) override;

	void OnError(int errorCode, wchar_t* comment) override;

private:
	HANDLE LogicThreadStarter();
	HANDLE TimerThreadStarter();

	void LogicThreadProc();
	void TimerThreadProc();

	void HandleAcceptJob(__int64 sessionID);
	void HandleReleaseJob(__int64 sessionID);
	void HandleRecvJob(__int64 sessionID, Message* pMessage);
	void HandleTimerJob();
	void HandleTerminateJob();

	//패킷 처리 함수
	void PacketProc_Login(INT64 sessionID, Message* pMessage);
	void PacketProc_SectorMove(INT64 sessionID, Message* pMessage);
	void PacketProc_Message(INT64 sessionID, Message* pMessage);
	void PacketProc_HeartBeat(INT64 sessionID, Message* pMessage);
	void PacketProc_UserDistribution(INT64 sessionID);

	//응답 패킷 값 세팅 함수
	void MakePacket_LoginResponse(Message* pMessage, BYTE status, INT64 accountNo);
	void MakePacket_SectorMoveResponse(Message* pMessage, INT64 accountNo, WORD sectorX, WORD sectorY);
	void MakePacket_MessageResponse(Message* pMessage, INT64 accountNo, WCHAR* id, WCHAR* nickName, WORD msgLen, WCHAR* message);

	bool SendPacket_Around(Message* pMessage, SectorAround& sectorAround);
private:
	//Job Queue
	LockFreeQueue<Job*> _jobQueue;
	//Job Event
	HANDLE _jobQueueEvent;

	//Job Memory Pool
	TLSMemoryPool<Job> _jobPool;

	//Sectors
	Sector _sectors[MAX_SECTOR_Y][MAX_SECTOR_X];

	//UserContainer
	std::unordered_map<__int64, User*> _userContainer;
	LONG _currentUserNum;

	//UserPool
	MemoryPool<User> _userPool;

	bool _isServerAlive = true;
	//logic Thread Handle
	HANDLE _hLogicThread;
	//logic Thread Flag
	bool _isLogicThreadRun;

	HANDLE _hTimerThread;
	bool _isTimerThreadRun;

	INT64 _updateTPS;
	INT64 _prevUpdateTPS;

public:
	HWPerformanceCounter _hwPC;
	ProcessPerformanceCounter _pPc;
	CpuUsage _cpuUsage;
};


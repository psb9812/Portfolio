#pragma once
#include <unordered_map>
#include "NetServer.h"
#include "TLSMemoryPool.h"
#include "MemoryPool.h"
#include "User.h"
#include "CommonDefine.h"
#include "Sector.h"
#include "MemoryLog.h"
#include "HWPerformanceCounter.h"
#include "ProcessPerformanceCounter.h"
#include "CpuUsage.h"

class ChattingServer :public NetServer
{
public:

	ChattingServer();
	virtual ~ChattingServer();

	bool Start(const WCHAR* configFileName) override;

	void Stop() override;

	LONG GetUserPoolCapacity() const;
	LONG GetUserPoolSize() const;
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

	void OnTime() override;

private:
	HANDLE TimerThreadStarter();

	void TimerThreadProc();

	//패킷 처리 함수
	void PacketProc_Login(INT64 sessionID, Message* pMessage);
	void PacketProc_SectorMove(INT64 sessionID, Message* pMessage);
	void PacketProc_Message(INT64 sessionID, Message* pMessage);
	void PacketProc_HeartBeat(INT64 sessionID, Message* pMessage);

	//응답 패킷 값 세팅 함수
	void MakePacket_LoginResponse(Message* pMessage, BYTE status, INT64 accountNo);
	void MakePacket_SectorMoveResponse(Message* pMessage, INT64 accountNo, WORD sectorX, WORD sectorY);
	void MakePacket_MessageResponse(Message* pMessage, INT64 accountNo, WCHAR* id, WCHAR* nickName, WORD msgLen, WCHAR* message);

	bool SendPacket_Around(Message* pMessage, SectorAround& sectorAround);

	void ELockUserContainer();
	void EUnlockUserContainer();
	void SLockUserContainer();
	void SUnlockUserContainer();

	void MoveSector(SectorPos oldSectorPos, SectorPos curSectorPos, Sector& oldSector, Sector& curSector, User* pUser);

	void ELockAround(SectorAround& sectorAround);
	void EUnlockAround(SectorAround& sectorAround);
	__forceinline void SLockAround(SectorAround& sectorAround)
	{
		//왼쪽->오른쪽 순서대로 SLock을 걸어준다.(데드락 방지)
		for (int i = 0; i < sectorAround.count; i++)
		{
			_sectors[sectorAround.around[i]._y][sectorAround.around[i]._x].SLock();
		}
	}
	__forceinline void SUnlockAround(SectorAround& sectorAround)
	{
		//락을 잡은 역순으로 해제해준다.
		for (int i = sectorAround.count - 1; i >= 0; i--)
		{
			_sectors[sectorAround.around[i]._y][sectorAround.around[i]._x].SUnlock();
		}
	}

	bool LoadChatServerConfig();
	void ConvertWcharToChar(const WCHAR* wideStr, char* charStr, size_t charStrSize);
	void ConvertCharToWchar(char* charStr, WCHAR* wideStr, size_t wideStrSize);
private:
	//Sectors
	Sector _sectors[MAX_SECTOR_Y][MAX_SECTOR_X];

	//UserContainer
	std::unordered_map<__int64, User*> _userContainer;
	SRWLOCK _userContainerLock;

	LONG _currentUserNum;

	//UserPool
	TLSMemoryPool<User> _userPool;

	inline static thread_local cpp_redis::client* _pRedisConnection = nullptr;
	SRWLOCK _redisInitLock;
	std::vector<cpp_redis::client*> _redisConnections;
	WCHAR _tokenServerIP[IP_LEN] = { 0, };
	USHORT _tokenServerPort = 0;


	HANDLE _hTimerThread;
	bool _isTimerThreadRun;
	bool _isServerAlive = false;

	INT64 _updateTPS;
	INT64 _prevUpdateTPS;

public:
	HWPerformanceCounter _hwPC;
	ProcessPerformanceCounter _pPc;
	CpuUsage _cpuUsage;
};
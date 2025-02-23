#pragma once
#include "NetServer.h"
#include "Windows.h"
#include <unordered_map>
#include "User.h"
#include "HWPerformanceCounter.h"
#include "ProcessPerformanceCounter.h"
#include "CpuUsage.h"

class DBConnection;

class LoginServer :
    public NetServer
{
public:

	LoginServer();
	virtual ~LoginServer();

	bool Start(const WCHAR* configFileName) override;

	void Stop() override;

	int GetUserPoolCapacity() const;
	int GetUserPoolSize() const;
	int GetUserCount() const;
	INT64 GetLoginAttemptCountPerSec() const;
	INT64 GetLoginWaitSessionNum() const;
	INT64 GetLoginCompleteCountPerSec() const;
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
	bool LoadLoginServerConfig();

	void ConvertWcharToChar(const WCHAR* wideStr, char* charStr, size_t charStrSize);
	void ConvertCharToWchar(char* charStr, WCHAR* wideStr, size_t wideStrSize);

	HANDLE TimerThreadStarter();

	void TimerThreadProc();

	//패킷 처리 함수
	void PacketProc_Login(INT64 sessionID, Message* pMessage);

	//응답 패킷 값 세팅 함수
	void MakePacket_LoginResponse(Message* pMessage, INT64 accountNo, BYTE status,
		WCHAR* ID, WCHAR* nickname, WCHAR* gameServerIP, USHORT gameServerPort,
		WCHAR* ChatServerIP, USHORT chatServerPort);

	void ELockUserContainer();
	void EUnlockUserContainer();
	void SLockUserContainer();
	void SUnlockUserContainer();

private:
	//UserContainer
	std::unordered_map<__int64, User*> _userContainer;
	SRWLOCK	_userContainerLock;

	LONG _currentUserNum;

	HANDLE _hTimerThread;
	bool _isTimerThreadRun;

	//UserPool
	TLSMemoryPool<User> _userPool;

	inline static thread_local DBConnection* _pDBConnection = nullptr;
	SRWLOCK _DBInitLock;
	std::vector<DBConnection*> _DBConnections;

	inline static thread_local cpp_redis::client* _pRedisConnection = nullptr;
	SRWLOCK _redisInitLock;
	std::vector<cpp_redis::client*> _redisConnections;

	WCHAR	_gameServerIP[IP_LEN] = { 0, };
	USHORT	_gameServerPort;
	WCHAR	_chatServerIP[IP_LEN] = { 0, };
	USHORT	_chatServerPort;
	WCHAR	_DBServerIP[IP_LEN] = { 0, };
	USHORT	_DBServerPort;
	WCHAR	_tokenServerIP[IP_LEN] = { 0, };
	USHORT	_tokenServerPort;
	WCHAR	_DBUserName[DB_CONFIG_LEN] = { 0, };
	WCHAR	_DBPassword[DB_CONFIG_LEN] = { 0, };
	WCHAR	_DBSchemaName[DB_CONFIG_LEN] = { 0, };

	bool _isServerAlive = false;

	//모니터링 변수
	// False Share 방지 패딩
	// 초당 로그인 시도 수
	alignas(64) INT64 _loginAttemptCountPerSec;
	char _padding1[64 - sizeof(INT64)] = { 0, };

	INT64 _prevLoginAttemptCountPerSec;
	char _padding2[64 - sizeof(INT64)] = { 0, };

	//로그인 처리 중인 세션 수
	INT64 _loginWaitSessionNum;
	char _padding3[64 - sizeof(INT64)] = { 0, };

	//초당 로그인 완료 수
	INT64 _loginCompleteCountPerSec;
	char _padding4[64 - sizeof(INT64)] = { 0, };

	INT64 _prevLoginCompleteCountPerSec;

public:
	HWPerformanceCounter _hwPC;
	ProcessPerformanceCounter _pPc;
	CpuUsage _cpuUsage;
};


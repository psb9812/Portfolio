#pragma once
#include <vector>
#include <WinSock2.h>
#include <Windows.h>
#include "SessionContainer.h"
#include "SmartMessagePtr.h"
//#define PROFILE
#include "Profiler.h"
#include <chrono>

class Message;
class Session;

class NetServer
{
public:

	NetServer();
	virtual ~NetServer();

	virtual bool Start();
	virtual void Stop();
	int GetSessionCount();

	void Disconnect(__int64 sessionID);

	bool SendPacket(__int64 sessionID, Message* pMessage);

	inline INT64 GetAcceptTPS() const { return _prevTPS._acceptTPS; }
	inline INT64 GetRecvMessageTPS() const { return _prevTPS._recvTPS; }
	inline INT64 GetSendMessageTPS() const { return _prevTPS._sendTPS; }
	inline INT64 GetTotalAccept() const { return _totalAccept; }
	inline INT64 GetSendThreadTPS() const { return _prvSendThreadTPS; }

private:
	//this 포인터를 전달하기 위한 스레드 스타터 함수
	HANDLE WorkerThreadStarter();
	HANDLE AcceptThreadStarter();
	HANDLE DebugInfoThreadStarter();
	HANDLE SendPostThreadStarter();

	void WorkerThread();
	void AcceptThread();
	void DebugInfoThread();
	void SendPostThread();

public:
	//네트워크 라이브러리에서만 직렬화 버퍼의 헤더 포인터를 얻도록 하는 friend 함수
	char* GetHeaderPtr(Message* pMessage);

protected:
	//===============================
	// 이벤트 콜백 함수
	//==============================
	
	virtual bool OnConnectionRequest(const wchar_t* connectIP, int connectPort) = 0;
	virtual void OnAccept(__int64 sessionID) = 0;
	virtual void OnRelease(__int64 sessionID) = 0;
	virtual void OnRecv(__int64 sessionID, Message* pMessage) = 0;
	virtual void OnSend(__int64 sessionID, int sendSize) = 0;
	virtual void OnError(int errorCode, wchar_t* comment) = 0;

private:
	void RecvCompleteProc(DWORD transferred, UINT64 sessionID, Session* pSession);
	void SendCompleteProc(DWORD transferred, UINT64 sessionID, Session* pSession);

	bool SessionRelease(Session* pSession);

	//IOCount를 조사해서 릴리즈 된 세션인지 검사하는 함수
	bool IsSessionRelease(ULONG ioCount);

	//세션의 사용권을 획득하는 함수
	Session* AcquireSessionLicense(UINT64 sessionID);
	//세션의 사용권을 반환하는 함수
	void ReleaseSessionLicense(Session* pSession);

private:
	//PQCS로 SendPacket 사인을 보낼 때 사용할 식별자 pOverlapped 자리에 넣는다.
	const int SEND_PACKET_SIGN_PQCS				= 1;

	SessionContainer			_sessionContainer;
	INT64						_sessionIDGenerator = 0;

	SOCKET						_listenSocket = INVALID_SOCKET;

	HANDLE						_hIOCP = INVALID_HANDLE_VALUE;

	HANDLE						_hAcceptThread;
	std::vector<HANDLE>			_workerThreadVector;
	HANDLE						_hDebugInfoThread;
	HANDLE						_hSendPostThread;

	bool						_isAcceptThreadRun;
	bool						_isWorkerThreadRun;
	bool						_isDebugInfoThreadRun;
	bool						_isSendPostThreadRun;

	DWORD	_sendTick = 0;

protected:
	//config 파일에서 값을 얻어올 변수
	WCHAR						_bindIP[16] = { 0, };
	int							_bindPort;
	int							_nagleOption;
	int							_IOCP_WorkerThreadNum;
	int							_IOCP_ConcurrentThreadNum;
	int							_numOfMaxSession;
	int							_numOfMaxUser;
	int							_packetCode;
	int							_encodeStaticKey;
	WCHAR						_systemLogLevel[32] = { 0, };
	int							_timeoutDisconnectForUser;
	int							_timeoutDisconnectForSession;
	int							_sendIntervalTime;
	int							_sendOnceMin;
	
private:
	// 모니터링용 변수
	// 같은 캐시라인에 묶이면 성능이 저하되므로 메모리 상에 패딩을 두었다.
	struct alignas(64) TPS
	{
		INT64	_acceptTPS	= 0;
		char padding1[64 - sizeof(_acceptTPS)];

		INT64 _recvTPS	= 0;
		char padding2[64 - sizeof(_recvTPS)];

		INT64 _sendTPS	= 0;
	};
	TPS							_TPS;
	TPS							_prevTPS;
	INT64						_totalAccept;

	INT64						_prvSendThreadTPS = 0;
	INT64						_sendThreadTPS = 0;
};


#pragma once
#include <vector>
#include <unordered_map>
#include <queue>
#include <WinSock2.h>
#include <Windows.h>
#include <chrono>
#include "SessionContainer.h"
#include "MemoryLog.h"

class Message;
class Job;
struct Session;
class Contents;

extern MemoryLog mLog;

class NetServer
{
	friend Contents;
public:
	NetServer();
	virtual ~NetServer();

	virtual bool Start(const WCHAR* configFileName);
	virtual void Stop();

	//세션의 연결 끊는 함수
	void Disconnect(UINT64 sessionID);
	//특정 세션에게 송신하는 함수
	bool SendPacket(UINT64 sessionID, Message* pMessage, bool usePQCS = false, bool disconnect = false, bool onlyEnqueue = false);

	//직렬 Contents 등록 함수
	bool AddContents(INT32 key, const Contents* pContents);

	//세션이 접속했을 때, 바로 소속시키려는 컨텐츠 설정 함수
	void SetDefaultContents(INT32 contentsID);

	//디버그용 수치 Getter 함수
	inline INT32 GetSessionCount() const { return _sessionContainer.GetSessionNum(); }
	inline INT64 GetAcceptTPS() const { return _prevTPS._acceptTPS; }
	inline INT64 GetRecvMessageTPS() const { return _prevTPS._recvTPS; }
	inline INT64 GetSendMessageTPS() const { return _prevTPS._sendTPS; }
	inline INT64 GetTotalAccept() const { return _totalAccept; }
	inline INT64 GetSendThreadTPS() const { return _sendThreadTPS; }

	//네트워크 라이브러리에서만 직렬화 버퍼의 헤더 포인터를 얻도록 하는 friend 함수
	char* GetHeaderPtr(Message* pMessage);

protected:
	//===============================
	// Event callback function
	//===============================
	virtual bool OnConnectionRequest(const WCHAR* connectIP, INT32 connectPort) = 0;
	virtual void OnAccept(INT64 sessionID) = 0;
	virtual void OnRelease(INT64 sessionID) = 0;
	virtual void OnRecv(INT64 sessionID, Message* pMessage) = 0;
	virtual void OnSend(INT64 sessionID, INT32 sendSize) = 0;
	virtual void OnTime() = 0;
	virtual void OnError(INT32 errorCode, WCHAR* comment) = 0;

	virtual IUser* GetUser(INT64 sessionID) = 0;

private:
	void RecvCompleteProc(DWORD transferred, UINT64 sessionID, Session* pSession);
	void SendCompleteProc(DWORD transferred, UINT64 sessionID, Session* pSession);

	bool SessionRelease(Session* pSession);
	bool IsSessionRelease(ULONG ioCount);
	void AwakeContentsThread(LONG contentsID);
	void PQCSSendPost(INT64 sessionID);

	Session* AcquireSessionLicense(UINT64 sessionID);
	void ReleaseSessionLicense(Session* pSession);

	void HandleSendPakcetPQCS(INT64 sessionID);
	void HandleReleasePQCS(INT64 sessionID);

	//thread function
	HANDLE WorkerThreadStarter();
	HANDLE AcceptThreadStarter();
	HANDLE DebugInfoThreadStarter();
	HANDLE SendPostThreadStarter();
	void WorkerThread();
	void AcceptThread();
	void DebugInfoThread();
	void SendPostThread();

protected:
	void PQCS(DWORD transferred, ULONG_PTR completionKey, LPOVERLAPPED overlapped);

protected:
	// PQCS 식별자
	enum PQCS_SIGN
	{
		SEND_PACKET = 1,
		RELEASE,
	};

private:
	SessionContainer							_sessionContainer;
	INT64										_sessionIDGenerator = 0;

	SOCKET										_listenSocket = INVALID_SOCKET;

	HANDLE										_hIOCP = INVALID_HANDLE_VALUE;

	HANDLE										_hAcceptThread;
	std::vector<HANDLE>							_workerThreadVector;
	HANDLE										_hDebugInfoThread;
	HANDLE										_hSendPostThread;

	bool										_isAcceptThreadRun;
	bool										_isWorkerThreadRun;
	bool										_isDebugInfoThreadRun;
	bool										_isSendPostThreadRun;

	DWORD	_sendTick = 0;

protected:
	std::unordered_map<int, Contents*>			_contentsHashTable;
	std::optional<INT32>						_opt_defaultContentsID;

public:
	//config 파일에서 값을 얻어올 변수
	WCHAR										_bindIP[16] = { 0, };
	INT32										_bindPort;
	INT32										_nagleOption;
	INT32										_IOCP_WorkerThreadNum;
	INT32										_IOCP_ConcurrentThreadNum;
	INT32										_numOfMaxSession;
	INT32										_numOfMaxUser;
	INT32										_packetCode;
	INT32										_encodeStaticKey;
	WCHAR										_systemLogLevel[32] = { 0, };
	INT32										_timeoutDisconnectForUser;
	INT32										_timeoutDisconnectForSession;
	INT32										_sendIntervalTime;

private:
	// 모니터링용 변수
	struct TPS
	{
		alignas(64)INT64 _acceptTPS = 0;
		alignas(64)INT64 _recvTPS = 0;
		alignas(64)INT64 _sendTPS = 0;
	};

	TPS										_TPS;
	TPS										_prevTPS;
	alignas(64) UINT64						_totalAccept;

	INT64						_prvSendThreadTPS = 0;
	INT64						_sendThreadTPS = 0;
};
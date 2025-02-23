#pragma once
#include <vector>
#include <unordered_map>
#include <queue>
#include <WinSock2.h>
#include <Windows.h>
#include <chrono>
#include "SessionContainer.h"

class Message;
class Job;
struct Session;
class Contents;

class NetServer
{
public:
	NetServer();
	virtual ~NetServer();

	virtual bool Start(const WCHAR* configFileName);
	virtual void Stop();

	void Disconnect(UINT64 sessionID);
	bool SendPacket(UINT64 sessionID, Message* pMessage, bool usePQCS = false, bool disconnect = false);

	bool AddContents(INT32 key, const Contents* pContents);
	void SetDefaultContents(INT32 contentsID);
	void RequestMoveContents(UINT64 sessionID, INT32 contentsID);

	inline INT32 GetSessionCount() const { return _sessionContainer.GetSessionNum(); }
	inline INT64 GetAcceptTPS() const { return _prevTPS._acceptTPS; }
	inline INT64 GetRecvMessageTPS() const { return _prevTPS._recvTPS; }
	inline INT64 GetSendMessageTPS() const { return _prevTPS._sendTPS; }
	inline INT64 GetTotalAccept() const { return _totalAccept; }
	inline INT64 GetSendThreadTPS() const { return _sendThreadTPS; }

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

	//��Ʈ��ũ ���̺귯�������� ����ȭ ������ ��� �����͸� �򵵷� �ϴ� friend �Լ�
	char* GetHeaderPtr(Message* pMessage);

private:
	void RecvCompleteProc(DWORD transferred, UINT64 sessionID, Session* pSession);
	void SendCompleteProc(DWORD transferred, UINT64 sessionID, Session* pSession);

	bool SessionRelease(Session* pSession);
	bool IsSessionRelease(ULONG ioCount);

	Session* AcquireSessionLicense(UINT64 sessionID);
	void ReleaseSessionLicense(Session* pSession);

	void HandleSendPakcetPQCS(INT64 sessionID);
	void HandleReleasePQCS(INT64 sessionID);
	void HandleTimerPQCS();
	void HandleMoveContentsPQCS(DWORD transferred, INT64 sessionID);
	void HandleFrameLogicPQCS(DWORD transferred, INT64 sessionID);

	//thread function
	HANDLE WorkerThreadStarter();
	HANDLE AcceptThreadStarter();
	HANDLE DebugInfoThreadStarter();
	HANDLE SendPostThreadStarter();
	HANDLE TimerThreadStarter();
	void WorkerThread();
	void AcceptThread();
	void DebugInfoThread();
	void SendPostThread();
	void TimerThread();

protected:
	void PQCS(DWORD transferred, ULONG_PTR completionKey, LPOVERLAPPED overlapped);

protected:
	// PQCS �ĺ���
	enum PQCS_SIGN
	{
		SEND_PACKET = 1,
		RELEASE,
		TIMER,
		MOVE_CONTENTS,
		FRAME_LOGIC,
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
	HANDLE										_hTimerThread;

	bool										_isAcceptThreadRun;
	bool										_isWorkerThreadRun;
	bool										_isDebugInfoThreadRun;
	bool										_isSendPostThreadRun;
	bool										_isTimerThreadRun;

	DWORD	_sendTick = 0;

	//������ ���� ����ü
	struct FrameTask 
	{
		std::chrono::steady_clock::time_point _executionTime;	// ���� �ð�
		int _contentID;											// ������ ID
		int _intervalMs;										// ȣ�� ���� (ms)

		FrameTask(std::chrono::steady_clock::time_point exTime, int contentID, int intervalMs)
			:_executionTime(exTime), _contentID(contentID), _intervalMs(intervalMs)
		{}

		bool operator>(const FrameTask& other) const
		{
			return _executionTime > other._executionTime;
		}
	};
	std::priority_queue<FrameTask, std::vector<FrameTask>, std::greater<>> _frameTimePriorityQueue;
	SRWLOCK _frameTimePriorityQueueLock;

protected:
	std::unordered_map<int, Contents*>			_contentsHashTable;
	std::optional<INT32>						_defaultContentID;

public:
	//config ���Ͽ��� ���� ���� ����
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
	// ����͸��� ����
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
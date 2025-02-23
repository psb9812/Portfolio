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
	//this �����͸� �����ϱ� ���� ������ ��Ÿ�� �Լ�
	HANDLE WorkerThreadStarter();
	HANDLE AcceptThreadStarter();
	HANDLE DebugInfoThreadStarter();
	HANDLE SendPostThreadStarter();

	void WorkerThread();
	void AcceptThread();
	void DebugInfoThread();
	void SendPostThread();

public:
	//��Ʈ��ũ ���̺귯�������� ����ȭ ������ ��� �����͸� �򵵷� �ϴ� friend �Լ�
	char* GetHeaderPtr(Message* pMessage);

protected:
	//===============================
	// �̺�Ʈ �ݹ� �Լ�
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

	//IOCount�� �����ؼ� ������ �� �������� �˻��ϴ� �Լ�
	bool IsSessionRelease(ULONG ioCount);

	//������ ������ ȹ���ϴ� �Լ�
	Session* AcquireSessionLicense(UINT64 sessionID);
	//������ ������ ��ȯ�ϴ� �Լ�
	void ReleaseSessionLicense(Session* pSession);

private:
	//PQCS�� SendPacket ������ ���� �� ����� �ĺ��� pOverlapped �ڸ��� �ִ´�.
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
	//config ���Ͽ��� ���� ���� ����
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
	// ����͸��� ����
	// ���� ĳ�ö��ο� ���̸� ������ ���ϵǹǷ� �޸� �� �е��� �ξ���.
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


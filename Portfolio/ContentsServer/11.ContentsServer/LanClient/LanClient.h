#pragma once
#pragma comment(lib, "ws2_32.lib")
#include <ws2tcpip.h>
#include <process.h>
#include <vector>

class Message;
class ClientSession;

class LanClient
{
protected:
	LanClient();
	virtual ~LanClient();

protected:
	bool InitializeNetwork(const WCHAR* ip, short port, int numOfWorkerThread, int numOfConcurrentThread, bool nagle);
	void TerminateNetwork();

	bool Disconnect();
	bool SendPacket(Message* pMessage);

	virtual void OnInit() = 0;
	virtual void OnTerminate() = 0;
	virtual void OnDisconnectServer() = 0;
	virtual void OnRecv(Message* pMessage) = 0;
	virtual void OnSend(int sendBytes) = 0;
	virtual void OnError(WCHAR* errorMessage) = 0;

private:
	static unsigned int WINAPI NetworkThread(LPVOID pParma);

	void HandleRelease();
	bool HandleSendComplete(DWORD sendBytes);
	bool HandleRecvComplete(DWORD recvBytes);

	bool RequestRecv();
	bool RequestSend();

	void IncreseIOCount();
	void DecreseIOCount();

	char* GetMessageHeaderPtr(Message* pMessage);

private:
	static constexpr int RELEASE_SIGN_PQCS = 1;

	std::vector<HANDLE> _hWorkerThreads;
	HANDLE _hIOCP;


	WCHAR _IP[16];
	short _port;
	bool _nagle;
	int _numOfWorkerThread;
	int _numOfConcurrentThread;

	ClientSession* _session;

};


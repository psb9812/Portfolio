#pragma once
#include "LanClient.h"
#include "Windows.h"

// ����͸� Ŭ���̾�Ʈ
// * ����
//	����͸� ������ �α��� ��û
//	����͸� ������ 1�� �ֱ�� ����͸� ������ �۽�

class EchoServer;
class MonitoringClient :
    public LanClient
{
public:
	MonitoringClient();
	~MonitoringClient();

public:
	bool Start(EchoServer* pServer);
	void Terminate();

private:
	void OnInit() override;
	void OnTerminate() override;
	void OnDisconnectServer() override;
	void OnRecv(Message* pMessage) override;
	void OnSend(int sendBytes) override;
	void OnError(WCHAR* errorMessage) override;

	static unsigned int WINAPI MonitorThread(LPVOID pParam);

private:
	EchoServer* _pServer = nullptr;
	static constexpr DWORD SEND_TIME_INTERVAL = 1000;
	HANDLE _hMoniterThread = INVALID_HANDLE_VALUE;


	bool _isRunMonitorThread = true;
	bool _connection = false;
};


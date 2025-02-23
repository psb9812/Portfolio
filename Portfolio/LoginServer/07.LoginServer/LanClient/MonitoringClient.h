#pragma once
#include "LanClient.h"
#include "Windows.h"

// ����͸� Ŭ���̾�Ʈ
// * ����
//	����͸� ������ �α��� ��û
//	����͸� ������ 1�� �ֱ�� ����͸� ������ �۽�

class LoginServer;
class MonitoringClient :
	public LanClient
{
public:
	MonitoringClient();
	~MonitoringClient();

public:
	bool Start(LoginServer* pServer);
	void Terminate();

private:
	void OnInit() override;
	void OnTerminate() override;
	void OnDisconnectServer() override;
	void OnRecv(Message* pMessage) override;
	void OnSend(int sendBytes) override;
	void OnError(WCHAR* errorMessage) override;

	void ConnectToMonitorServer();
	void MakePacket_LoginMonitorServer(Message* pMessage);
	void SendData(BYTE type, int dataValue, int timeStamp);

	static unsigned int WINAPI MonitorThread(LPVOID pParam);

private:
	LoginServer* _pServer = nullptr;
	static constexpr DWORD SEND_TIME_INTERVAL = 1000;
	HANDLE _hMoniterThread = INVALID_HANDLE_VALUE;

	bool _serverAlive = false;
	bool _connection = false;
};


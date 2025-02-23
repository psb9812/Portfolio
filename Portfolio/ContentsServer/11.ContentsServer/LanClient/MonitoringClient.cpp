#include "pch.h"
#include "MonitoringClient.h"
#include "Message.h"

MonitoringClient::MonitoringClient()
{
}

MonitoringClient::~MonitoringClient()
{
}

bool MonitoringClient::Start(EchoServer* pServer)
{
	_pServer = pServer;

	//��Ʈ��ũ �ʱ�ȭ
	InitializeNetwork(MONITOR_SERVER_IP, MONITOR_SERVER_PORT, 5, 2, true);

	//����� ������ ����
	_hMoniterThread = (HANDLE)_beginthreadex(NULL, 0, MonitorThread, this, 0, nullptr);

	return false;
}

void MonitoringClient::Terminate()
{
	//��Ʈ��ũ ����
	TerminateNetwork();

	//������ ����
	_isRunMonitorThread = false;
	WaitForSingleObject(_hMoniterThread, INFINITE);

	//�ڵ� ����
	CloseHandle(_hMoniterThread);
}

void MonitoringClient::OnInit()
{
	wprintf(L"# ��Ʈ��ũ �ʱ�ȭ �Ϸ�.\n");
}

void MonitoringClient::OnTerminate()
{
	wprintf(L"# ����͸� Ŭ���̾�Ʈ ����.\n");
}

void MonitoringClient::OnDisconnectServer()
{
	wprintf(L"# ����͸� ���� ���� ����.\n");
}

void MonitoringClient::OnRecv(Message* pMessage)
{

}

void MonitoringClient::OnSend(int sendBytes)
{

}

void MonitoringClient::OnError(WCHAR* errorMessage)
{

}

unsigned int __stdcall MonitoringClient::MonitorThread(LPVOID pParam)
{
	MonitoringClient* pClient = reinterpret_cast<MonitoringClient*>(pParam);
	DWORD _tickCount = 0;

	while (pClient->_isRunMonitorThread)
	{
		//1�� ���
#pragma region Frame
		if ((timeGetTime() - _tickCount) < SEND_TIME_INTERVAL)
		{
			Sleep(SEND_TIME_INTERVAL - (timeGetTime() - _tickCount));
		}
		_tickCount += SEND_TIME_INTERVAL;
#pragma endregion

#ifdef MONITOR_SERVER_ON
		pClient->ConnectToMonitorServer();
#endif // MONITOR_SERVER_ON

	}
	
	return 0;
}

void MonitoringClient::ConnectToMonitorServer()
{
	while (_isRunMonitorThread && _connection == false)
	{
		// ����� ������ ���� ���� ���� ���� ���� ������ �ݺ��ؼ� ���� �õ��Ѵ�.
		_connection = InitializeNetwork(MONITOR_SERVER_IP, MONITOR_SERVER_PORT, 5, 2, true);
		Sleep(1000);
	}

	//����� ������ ������ �Ǿ��ٸ� �α��� ��û ��Ŷ�� ���.
	Message* pLoginMessage = Message::Alloc();
	MakePacket_LoginMonitorServer(pLoginMessage);
	SendPacket(pLoginMessage);
}

void MonitoringClient::MakePacket_LoginMonitorServer(Message* pMessage)
{
	*pMessage << static_cast<WORD>(en_PACKET_SS_MONITOR) << static_cast<int>(GAME_SERVER_ID);
}

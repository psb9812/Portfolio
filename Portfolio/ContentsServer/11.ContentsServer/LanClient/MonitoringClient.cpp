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

	//네트워크 초기화
	InitializeNetwork(MONITOR_SERVER_IP, MONITOR_SERVER_PORT, 5, 2, true);

	//모니터 스레드 생성
	_hMoniterThread = (HANDLE)_beginthreadex(NULL, 0, MonitorThread, this, 0, nullptr);

	return false;
}

void MonitoringClient::Terminate()
{
	//네트워크 종료
	TerminateNetwork();

	//스레드 종료
	_isRunMonitorThread = false;
	WaitForSingleObject(_hMoniterThread, INFINITE);

	//핸들 정리
	CloseHandle(_hMoniterThread);
}

void MonitoringClient::OnInit()
{
	wprintf(L"# 네트워크 초기화 완료.\n");
}

void MonitoringClient::OnTerminate()
{
	wprintf(L"# 모니터링 클라이언트 종료.\n");
}

void MonitoringClient::OnDisconnectServer()
{
	wprintf(L"# 모니터링 서버 연결 끊김.\n");
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
		//1초 대기
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
		// 모니터 서버가 아직 켜지 있지 않을 수도 있으니 반복해서 접속 시도한다.
		_connection = InitializeNetwork(MONITOR_SERVER_IP, MONITOR_SERVER_PORT, 5, 2, true);
		Sleep(1000);
	}

	//모니터 서버와 연결이 되엇다면 로그인 요청 패킷을 쏜다.
	Message* pLoginMessage = Message::Alloc();
	MakePacket_LoginMonitorServer(pLoginMessage);
	SendPacket(pLoginMessage);
}

void MonitoringClient::MakePacket_LoginMonitorServer(Message* pMessage)
{
	*pMessage << static_cast<WORD>(en_PACKET_SS_MONITOR) << static_cast<int>(GAME_SERVER_ID);
}

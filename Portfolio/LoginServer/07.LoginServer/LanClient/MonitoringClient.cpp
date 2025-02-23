#include "pch.h"
#include "MonitoringClient.h"
#include "../LoginServer.h"
#include "Message.h"


MonitoringClient::MonitoringClient()
{
}

MonitoringClient::~MonitoringClient()
{
}

bool MonitoringClient::Start(LoginServer* pServer)
{
	_pServer = pServer;

	_serverAlive = _pServer->GetServerAlive();
	//모니터 스레드 생성
	_hMoniterThread = (HANDLE)_beginthreadex(NULL, 0, MonitorThread, this, 0, nullptr);

	wprintf(L"# 모니터링 클라이언트 스타트.\n");
	return true;
}

void MonitoringClient::Terminate()
{
	//네트워크 종료
	TerminateNetwork();

	//스레드 종료
	WaitForSingleObject(_hMoniterThread, INFINITE);

	//핸들 정리
	CloseHandle(_hMoniterThread);
}

void MonitoringClient::OnInit()
{
	wprintf(L"# 모니터링 클라이언트 네트워크 초기화 완료.\n");
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
	LoginServer* pServer = pClient->_pServer;
	DWORD _tickCount = timeGetTime();

	while (pClient->_serverAlive)
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
		if(!pClient->_connection)
			pClient->ConnectToMonitorServer();
#endif // MONITOR_SERVER_ON

		//데이터 수집
		pServer->_cpuUsage.UpdateCpuTime();
		pClient->_serverAlive = pServer->GetServerAlive();
		int processOnOff			= pClient->_serverAlive;
		int processCPUUsage			= static_cast<int>(pServer->_cpuUsage.ProcessTotal());
		int processMemoryUsage_MB	= static_cast<int>(pServer->_pPc.GetPrivateMemoryBytes() / 1000000);
		
		int numOfSessions			= pServer->GetSessionCount();
		int numOfAuthTPS			= pServer->GetLoginCompleteCountPerSec();
		int messagePoolUsage		= Message::GetMemoryPoolUseSize();

		//공통 수집 사항
		int totalCPUUsage			= static_cast<int>(pServer->_cpuUsage.ProcessorTotal());
		int NPPool_MB				= static_cast<int>(pServer->_hwPC.GetNPPoolMemoryBytes() / 1000000);
		int recvBytes_KB			= pServer->_hwPC.GetEthernetRecvBytes() / 1000;
		int sendBytes_KB			= pServer->_hwPC.GetEthernetSendBytes() / 1000;
		int availableMemory			= static_cast<int>(pServer->_hwPC.GetAvilableMBytes());

#ifdef MONITOR_SERVER_ON
		//데이터 송신
		int timeStamp = static_cast<int>(time(NULL));

		pClient->SendData(dfMONITOR_DATA_TYPE_LOGIN_SERVER_RUN, processOnOff, timeStamp);
		pClient->SendData(dfMONITOR_DATA_TYPE_LOGIN_SERVER_CPU, processCPUUsage, timeStamp);
		pClient->SendData(dfMONITOR_DATA_TYPE_LOGIN_SERVER_MEM, processMemoryUsage_MB, timeStamp);
		pClient->SendData(dfMONITOR_DATA_TYPE_LOGIN_SESSION, numOfSessions, timeStamp);
		pClient->SendData(dfMONITOR_DATA_TYPE_LOGIN_AUTH_TPS, numOfAuthTPS, timeStamp);
		pClient->SendData(dfMONITOR_DATA_TYPE_LOGIN_PACKET_POOL, messagePoolUsage, timeStamp);
		pClient->SendData(dfMONITOR_DATA_TYPE_MONITOR_CPU_TOTAL, totalCPUUsage, timeStamp);
		pClient->SendData(dfMONITOR_DATA_TYPE_MONITOR_NONPAGED_MEMORY, NPPool_MB, timeStamp);
		pClient->SendData(dfMONITOR_DATA_TYPE_MONITOR_NETWORK_RECV, recvBytes_KB, timeStamp);
		pClient->SendData(dfMONITOR_DATA_TYPE_MONITOR_NETWORK_SEND, sendBytes_KB, timeStamp);
		pClient->SendData(dfMONITOR_DATA_TYPE_MONITOR_AVAILABLE_MEMORY, availableMemory, timeStamp);

#endif // MONITOR_SERVER_ON

		// 콘솔 출력
		{
			wprintf(L"==================Login Server==============\n");
			wprintf(
				L"Session Num : %d\n"
				L"----------------------------------------\n"
				L"Message Pool Capacity : %d\n"
				L"Message Pool UseSize : %d\n"
				L"----------------------------------------\n"
				L"User Pool Capacity : %d\n"
				L"User Pool User Size : %d\n"
				L"----------------------------------------\n"
				L"User Num : %d\n"
				L"----------------------------------------\n"
				L"TotalAccept : %lld\n"
				L"AcceptTPS : %lld\n"
				L"SendTPS : %lld\n"
				L"RecvTPS : %lld\n"
				L"----------------------------------------\n"
				L"Login Attempt Per Sec : %lld\n"
				L"Login Complete Per Sec : %lld\n"
				L"Login Wait Session Num : %lld\n"
				L"----------------------------------------\n"
				L"Total CPU Usage : %d\n"
				L"Using NPPool(byte) : %lld\n"
				L"Avilable Memory (MB) : %lf\n"
				L"----------------------------------------\n"
				L"Process CPU Usage : %d\n"
				L"Private Memory (byte) : %lld\n"
				,
				numOfSessions,
				Message::GetMemoryPoolCapacity(),
				messagePoolUsage,
				pServer->GetUserPoolCapacity(),
				pServer->GetUserPoolSize(),
				pServer->GetUserCount(),
				pServer->GetTotalAccept(),
				pServer->GetAcceptTPS(),
				pServer->GetSendMessageTPS(),
				pServer->GetRecvMessageTPS(),
				pServer->GetLoginAttemptCountPerSec(),
				numOfAuthTPS,
				pServer->GetLoginWaitSessionNum(),
				totalCPUUsage,
				pServer->_hwPC.GetNPPoolMemoryBytes(),
				pServer->_hwPC.GetAvilableMBytes(),
				processCPUUsage,
				pServer->_pPc.GetPrivateMemoryBytes()
			);
		}
	}
	
	return 0;
}

void MonitoringClient::ConnectToMonitorServer()
{
	while (_serverAlive && _connection == false)
	{
		// 모니터 서버가 아직 켜지 있지 않을 수도 있으니 반복해서 접속 시도한다.
		_connection = InitializeNetwork(MONITOR_SERVER_IP, MONITOR_SERVER_PORT, 5, 2, true);
		Sleep(1000);
	}

	//모니터 서버와 연결이 됐다면 로그인 요청 패킷을 쏜다.
	Message* pLoginMessage = Message::Alloc();
	MakePacket_LoginMonitorServer(pLoginMessage);
	SendPacket(pLoginMessage);
}

void MonitoringClient::MakePacket_LoginMonitorServer(Message* pMessage)
{
	*pMessage << static_cast<WORD>(en_PACKET_SS_MONITOR_LOGIN) << static_cast<int>(LOGIN_SERVER_ID);
}

void MonitoringClient::SendData(BYTE type, int dataValue, int timeStamp)
{
	// 메시지 생성 및 제작
	Message* pMessage = Message::Alloc();
	*pMessage << static_cast<WORD>(en_PACKET_SS_MONITOR_DATA_UPDATE) << type << dataValue << timeStamp;
	
	//메시지 송신
	SendPacket(pMessage);
}

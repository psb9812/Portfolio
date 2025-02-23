#include "pch.h"
#include "MonitoringClient.h"
#include "EchoServer.h"
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

	_serverAlive = _pServer->GetServerAlive();
	//����� ������ ����
	_hMoniterThread = (HANDLE)_beginthreadex(NULL, 0, MonitorThread, this, 0, nullptr);

	wprintf(L"# ����͸� Ŭ���̾�Ʈ ��ŸƮ.\n");
	return true;
}

void MonitoringClient::Terminate()
{
	//��Ʈ��ũ ����
	TerminateNetwork();

	//������ ����
	WaitForSingleObject(_hMoniterThread, INFINITE);

	//�ڵ� ����
	CloseHandle(_hMoniterThread);
}

void MonitoringClient::OnInit()
{
	wprintf(L"# ����͸� Ŭ���̾�Ʈ ��Ʈ��ũ �ʱ�ȭ �Ϸ�.\n");
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
	EchoServer* pServer = pClient->_pServer;
	DWORD _tickCount = timeGetTime();

	while (pClient->_serverAlive)
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
		if (!pClient->_connection)
			pClient->ConnectToMonitorServer();
#endif // MONITOR_SERVER_ON

		//������ ����
		pServer->_cpuUsage.UpdateCpuTime();
		pClient->_serverAlive = pServer->GetServerAlive();
		int processOnOff = pClient->_serverAlive;
		int processCPUUsage = static_cast<int>(pServer->_cpuUsage.ProcessTotal());
		int processMemoryUsage_MB = static_cast<int>(pServer->_pPC.GetPrivateMemoryBytes() / 1000000);

		int numOfSessions = pServer->GetSessionCount();
		int numOfLoginContentsUser = pServer->GetLoginContentsSessionNum();
		int numOfEchoContentsUser = pServer->GetEchoContentsSessionNum();
		int acceptTps = pServer->GetAcceptTPS();
		int recvTps = pServer->GetRecvMessageTPS();
		int sendTps = pServer->GetSendMessageTPS();
		//DB ������
		int loginContentsFPS = pServer->GetLoginContentsFPS();
		int echoContentsFPS = pServer->GetEchoContentsFPS();
		int messagePoolUsage = Message::GetMemoryPoolUseSize();

		//���� ���� ����
		int totalCPUUsage = static_cast<int>(pServer->_cpuUsage.ProcessorTotal());
		int NPPool_MB = static_cast<int>(pServer->_hwPC.GetNPPoolMemoryBytes() / 1000000);
		int recvBytes_KB = pServer->_hwPC.GetEthernetRecvBytes() / 1000;
		int sendBytes_KB = pServer->_hwPC.GetEthernetSendBytes() / 1000;
		int availableMemory = static_cast<int>(pServer->_hwPC.GetAvilableMBytes());

#ifdef MONITOR_SERVER_ON
		//������ �۽�
		int timeStamp = static_cast<int>(time(NULL));

		pClient->SendData(dfMONITOR_DATA_TYPE_GAME_SERVER_RUN, processOnOff, timeStamp);
		pClient->SendData(dfMONITOR_DATA_TYPE_GAME_SERVER_CPU, processCPUUsage, timeStamp);
		pClient->SendData(dfMONITOR_DATA_TYPE_GAME_SERVER_MEM, processMemoryUsage_MB, timeStamp);
		pClient->SendData(dfMONITOR_DATA_TYPE_GAME_SESSION, numOfSessions, timeStamp);
		pClient->SendData(dfMONITOR_DATA_TYPE_GAME_AUTH_PLAYER, numOfLoginContentsUser, timeStamp);
		pClient->SendData(dfMONITOR_DATA_TYPE_GAME_GAME_PLAYER, numOfEchoContentsUser, timeStamp);
		pClient->SendData(dfMONITOR_DATA_TYPE_GAME_ACCEPT_TPS, acceptTps, timeStamp);
		pClient->SendData(dfMONITOR_DATA_TYPE_GAME_PACKET_RECV_TPS, recvTps, timeStamp);
		pClient->SendData(dfMONITOR_DATA_TYPE_GAME_PACKET_SEND_TPS, sendTps, timeStamp);
		pClient->SendData(dfMONITOR_DATA_TYPE_GAME_AUTH_THREAD_FPS, loginContentsFPS, timeStamp);
		pClient->SendData(dfMONITOR_DATA_TYPE_GAME_GAME_THREAD_FPS, echoContentsFPS, timeStamp);
		pClient->SendData(dfMONITOR_DATA_TYPE_GAME_PACKET_POOL, messagePoolUsage, timeStamp);
		
		pClient->SendData(dfMONITOR_DATA_TYPE_MONITOR_CPU_TOTAL, totalCPUUsage, timeStamp);
		pClient->SendData(dfMONITOR_DATA_TYPE_MONITOR_NONPAGED_MEMORY, NPPool_MB, timeStamp);
		pClient->SendData(dfMONITOR_DATA_TYPE_MONITOR_NETWORK_RECV, recvBytes_KB, timeStamp);
		pClient->SendData(dfMONITOR_DATA_TYPE_MONITOR_NETWORK_SEND, sendBytes_KB, timeStamp);
		pClient->SendData(dfMONITOR_DATA_TYPE_MONITOR_AVAILABLE_MEMORY, availableMemory, timeStamp);

#endif // MONITOR_SERVER_ON

		// �ܼ� ���
		{
			wprintf(L"===============EchoGameServer===================\n");
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
				L"AcceptTPS : %d\n"
				L"SendTPS : %d\n"
				L"RecvTPS : %d\n"
				L"----------------------------------------\n"
				L"Total CPU Usage : %d\n"
				L"Using NPPool(byte) : %d\n"
				L"Avilable Memory (MB) : %d\n"
				L"----------------------------------------\n"
				L"Process CPU Usage : %d\n"
				L"Private Memory (byte) : %d\n"
				L"---------------------------------------\n"
				L"SendThreadTPS : %lld\n"
				L"---------------------------------------\n"
				L"Login Content frame Cnt : %d\n"
				L"Echo Content frame Cnt : %d\n"
				,
				numOfSessions,
				Message::GetMemoryPoolCapacity(),
				Message::GetMemoryPoolUseSize(),
				pServer->GetUserPoolCapacity(),
				pServer->GetUserPoolUsage(),
				pServer->GetUserNum(),
				pServer->GetTotalAccept(),
				acceptTps,
				sendTps,
				recvTps,
				totalCPUUsage,
				NPPool_MB,
				availableMemory,
				processCPUUsage,
				processMemoryUsage_MB,
				pServer->GetSendThreadTPS(),
				loginContentsFPS,
				echoContentsFPS
			);
		}
	}

	return 0;
}

void MonitoringClient::ConnectToMonitorServer()
{
	while (_serverAlive && _connection == false)
	{
		// ����� ������ ���� ���� ���� ���� ���� ������ �ݺ��ؼ� ���� �õ��Ѵ�.
		_connection = InitializeNetwork(MONITOR_SERVER_IP, MONITOR_SERVER_PORT, 2, 1, true);
		Sleep(1000);
	}

	//����� ������ ������ �ƴٸ� �α��� ��û ��Ŷ�� ���.
	Message* pLoginMessage = Message::Alloc();
	MakePacket_LoginMonitorServer(pLoginMessage);
	SendPacket(pLoginMessage);
}

void MonitoringClient::MakePacket_LoginMonitorServer(Message* pMessage)
{
	*pMessage << static_cast<WORD>(en_PACKET_SS_MONITOR_LOGIN) << static_cast<int>(GAME_SERVER_ID);
}

void MonitoringClient::SendData(BYTE type, int dataValue, int timeStamp)
{
	// �޽��� ���� �� ����
	Message* pMessage = Message::Alloc();
	*pMessage << static_cast<WORD>(en_PACKET_SS_MONITOR_DATA_UPDATE) << type << dataValue << timeStamp;

	//�޽��� �۽�
	SendPacket(pMessage);
}

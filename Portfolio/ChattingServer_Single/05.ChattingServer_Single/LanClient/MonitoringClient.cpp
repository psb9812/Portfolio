#include "MonitoringClient.h"
#include "../ChattingServer.h"
#include "../Message.h"
#include "../CommonDefine.h"
#include "../../../CommonProtocol.h"


MonitoringClient::MonitoringClient()
{
}

MonitoringClient::~MonitoringClient()
{
}

bool MonitoringClient::Start(ChattingServer* pServer)
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
	ChattingServer* pServer = pClient->_pServer;
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
		if(!pClient->_connection)
			pClient->ConnectToMonitorServer();
#endif // MONITOR_SERVER_ON

		//������ ����
		pServer->_cpuUsage.UpdateCpuTime();
		pClient->_serverAlive = pServer->GetServerAlive();
		int processOnOff			= pClient->_serverAlive;
		int processCPUUsage			= static_cast<int>(pServer->_cpuUsage.ProcessTotal());
		int processMemoryUsage_MB	= static_cast<int>(pServer->_pPc.GetPrivateMemoryBytes() / 1000000);
		
		int numOfSessions			= pServer->GetSessionCount();
		int numOfUsers				= pServer->GetUserCount();
		int updateTPS				= static_cast<int>(pServer->GetUpdateTPS());
		int messagePoolUsage		= Message::GetMemoryPoolUseSize();
		int jobQueueSize			= pServer->GetJobQueueSize();

		int totalCPUUsage			= static_cast<int>(pServer->_cpuUsage.ProcessorTotal());
		int NPPool_MB				= static_cast<int>(pServer->_hwPC.GetNPPoolMemoryBytes() / 1000000);
		int recvBytes_KB			= pServer->_hwPC.GetEthernetRecvBytes() / 1000;
		int sendBytes_KB			= pServer->_hwPC.GetEthernetSendBytes() / 1000;
		int availableMemory			= static_cast<int>(pServer->_hwPC.GetAvilableMBytes());

#ifdef MONITOR_SERVER_ON
		//������ �۽�
		int timeStamp = static_cast<int>(time(NULL));

		pClient->SendData(dfMONITOR_DATA_TYPE_CHAT_SERVER_RUN, processOnOff, timeStamp);
		pClient->SendData(dfMONITOR_DATA_TYPE_CHAT_SERVER_CPU, processCPUUsage, timeStamp);
		pClient->SendData(dfMONITOR_DATA_TYPE_CHAT_SERVER_MEM, processMemoryUsage_MB, timeStamp);
		pClient->SendData(dfMONITOR_DATA_TYPE_CHAT_SESSION, numOfSessions, timeStamp);
		pClient->SendData(dfMONITOR_DATA_TYPE_CHAT_PLAYER, numOfUsers, timeStamp);
		pClient->SendData(dfMONITOR_DATA_TYPE_CHAT_UPDATE_TPS, updateTPS, timeStamp);
		pClient->SendData(dfMONITOR_DATA_TYPE_CHAT_PACKET_POOL, messagePoolUsage, timeStamp);
		pClient->SendData(dfMONITOR_DATA_TYPE_CHAT_UPDATEMSG_POOL, jobQueueSize, timeStamp);
		pClient->SendData(dfMONITOR_DATA_TYPE_MONITOR_CPU_TOTAL, totalCPUUsage, timeStamp);
		pClient->SendData(dfMONITOR_DATA_TYPE_MONITOR_NONPAGED_MEMORY, NPPool_MB, timeStamp);
		pClient->SendData(dfMONITOR_DATA_TYPE_MONITOR_NETWORK_RECV, recvBytes_KB, timeStamp);
		pClient->SendData(dfMONITOR_DATA_TYPE_MONITOR_NETWORK_SEND, sendBytes_KB, timeStamp);
		pClient->SendData(dfMONITOR_DATA_TYPE_MONITOR_AVAILABLE_MEMORY, availableMemory, timeStamp);

#endif // MONITOR_SERVER_ON

		// �ܼ� ���
		{
			wprintf(L"===============ChatServer_Single===================\n");
			wprintf(
				L"---------------Connection-------------\n"
				L"Session Num : %d\n"
				L"User Num : %d\n"
				L"---------------Memory-----------------\n"
				L"Message Pool Capacity : %d\n"
				L"Message Pool UseSize : %d\n"
				L"--------------------------------------\n"
				L"Job Pool Capacity : %d\n"
				L"Job Pool Use Size : %d\n"
				L"Job Queue Size : %d\n"
				L"----------------------------------------\n"
				L"User Pool Capacity : %d\n"
				L"User Pool User Size : %d\n"
				L"------------------TPS-------------------\n"
				L"TotalAccept : %lld\n"
				L"AcceptTPS : %lld\n"
				L"SendTPS : %lld\n"
				L"RecvTPS : %lld\n"
				L"UpdateTPS : %d\n"
				L"SendThread_FPS : %lld\n"
				L"----------------HW Performance-----------------\n"
				L"Total CPU Usage : %d %%\n"
				L"Using NPPool(MB) : %d\n"
				L"Avilable Memory (MB) : %d\n"
				L"Total EtherNet Send/sec(KB) : %d\n"
				L"Total EtherNet Recv/sec(KB) : %d\n"
				L"------------------Process Performance-----------------\n"
				L"Process CPU Usage : %d %%\n"
				L"Private Memory (MB) : %d\n"
				L"====================================\n"
				,
				numOfSessions,
				numOfUsers,
				Message::GetMemoryPoolCapacity(),
				messagePoolUsage,
				pServer->GetJobPoolCapacity(),
				pServer->GetJobPoolSize(),
				pServer->GetJobQueueSize(),
				pServer->GetUserPoolCapacity(),
				pServer->GetUserPoolSize(),
				pServer->GetTotalAccept(),
				pServer->GetAcceptTPS(),
				pServer->GetSendMessageTPS(),
				pServer->GetRecvMessageTPS(),
				updateTPS,
				pServer->GetSendThreadTPS(),
				totalCPUUsage,
				NPPool_MB,
				availableMemory,
				sendBytes_KB,
				recvBytes_KB,
				processCPUUsage,
				processMemoryUsage_MB
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
		_connection = InitializeNetwork(MONITOR_SERVER_IP, MONITOR_SERVER_PORT, 5, 2, true);
		Sleep(1000);
	}

	//����� ������ ������ �ƴٸ� �α��� ��û ��Ŷ�� ���.
	Message* pLoginMessage = Message::Alloc();
	MakePacket_LoginMonitorServer(pLoginMessage);
	SendPacket(pLoginMessage);
}

void MonitoringClient::MakePacket_LoginMonitorServer(Message* pMessage)
{
	*pMessage << static_cast<WORD>(en_PACKET_SS_MONITOR_LOGIN) << static_cast<int>(CHAT_SERVER_ID);
}

void MonitoringClient::SendData(BYTE type, int dataValue, int timeStamp)
{
	// �޽��� ���� �� ����
	Message* pMessage = Message::Alloc();
	*pMessage << (WORD)en_PACKET_SS_MONITOR_DATA_UPDATE << type << dataValue << timeStamp;
	
	//�޽��� �۽�
	SendPacket(pMessage);
}

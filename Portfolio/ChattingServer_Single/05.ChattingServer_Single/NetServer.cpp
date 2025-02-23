#pragma comment(lib, "ws2_32")
#include "NetServer.h"
#include <WS2tcpip.h>
#include "PacketProtocol.h"
#include "SystemLog.h"
#include "Message.h"
#include "Session.h"
#include "Parser.h"
#include <chrono>


NetServer::NetServer()
	:_totalAccept(0), _hAcceptThread(INVALID_HANDLE_VALUE), _hDebugInfoThread(INVALID_HANDLE_VALUE), _hSendPostThread(INVALID_HANDLE_VALUE),
	_isAcceptThreadRun(true), _isWorkerThreadRun(true), _isDebugInfoThreadRun(true), _isSendPostThreadRun(true),
	_numOfMaxSession(0), _numOfMaxUser(0), _nagleOption(false), _IOCP_WorkerThreadNum(0), _IOCP_ConcurrentThreadNum(0),
	_packetCode(0), _encodeStaticKey(0), _timeoutDisconnectForSession(0), _timeoutDisconnectForUser(0), _sendIntervalTime(0)
{
}

NetServer::~NetServer()
{
}

bool NetServer::Start()
{
	SystemLog* logger = SystemLog::GetInstance();

	///////////////////////////////////////////////////
	//		config ���Ͽ��� Parser�� �� �о����
	///////////////////////////////////////////////////
	Parser parser;
	if (!parser.LoadFile(L"ChatServer.cnf"))
	{
		logger->WriteLogConsole(SystemLog::LOG_LEVEL_ERROR, L"[����] Config ���� �ε� ���� : %d\n", GetLastError());
		return false;
	}

	parser.GetValue(L"BIND_IP", _bindIP, 16);
	parser.GetValue(L"BIND_PORT", &_bindPort);
	parser.GetValue(L"NAGLE_OPTION", &_nagleOption);
	parser.GetValue(L"IOCP_WORKER_THREAD", &_IOCP_WorkerThreadNum);
	parser.GetValue(L"IOCP_ACTIVE_THREAD", &_IOCP_ConcurrentThreadNum);
	parser.GetValue(L"SESSION_MAX", &_numOfMaxSession);
	parser.GetValue(L"USER_MAX", &_numOfMaxUser);
	parser.GetValue(L"SEND_INTERVAL_TIME", &_sendIntervalTime);
	parser.GetValue(L"SEND_ONCE_MIN", &_sendOnceMin);
	parser.GetValue(L"PACKET_CODE", &_packetCode);
	parser.GetValue(L"PACKET_KEY", &_encodeStaticKey);
	parser.GetValue(L"LOG_LEVEL", _systemLogLevel, 32);
	parser.GetValue(L"TIMEOUT_DISCONNECT_USER", &_timeoutDisconnectForUser);
	parser.GetValue(L"TIMEOUT_DISCONNECT_SESSION", &_timeoutDisconnectForSession);

	//�ý��� �α� ����
	if (_tcscmp(_systemLogLevel, L"DEBUG"))
	{
		logger->SetLogLevel(SystemLog::LOG_LEVEL_DEBUG);
	}
	else if (_tcscmp(_systemLogLevel, L"ERROR"))
	{
		logger->SetLogLevel(SystemLog::LOG_LEVEL_ERROR);
	}
	else if (_tcscmp(_systemLogLevel, L"SYSTEM"))
	{
		logger->SetLogLevel(SystemLog::LOG_LEVEL_SYSTEM);
	}
	else
	{
		logger->WriteLogConsole(SystemLog::LOG_LEVEL_ERROR, L"[����] �� �� ���� �α� ���� : %d\n", GetLastError());
		return false;
	}
	logger->SetLogDirectory(L"ChattingServer_Log_Directory");


	///////////////////////////////////////////////////
	//		��Ʈ��ũ ���� ����
	///////////////////////////////////////////////////
	WSADATA wsa;
	int ret = WSAStartup(MAKEWORD(2, 2), &wsa);
	if (ret != 0)
	{
		logger->WriteLogConsole(SystemLog::LOG_LEVEL_ERROR, L"[����] WSAStartup()�Լ� ���� : %d\n", GetLastError());
		return false;
	}

	//���� ���� ����
	_listenSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (_listenSocket == INVALID_SOCKET)
	{
		logger->WriteLogConsole(SystemLog::LOG_LEVEL_ERROR, L"[����] socket() �Լ� ���� : %d\n", GetLastError());
	}


	//���� �ּ� ����ü ����
	SOCKADDR_IN serverAddr;
	memset(&serverAddr, 0, sizeof(SOCKADDR_IN));
	serverAddr.sin_family = AF_INET;
	//InetPton(AF_INET, serverIP, &serverAddr.sin_addr);
	serverAddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
	serverAddr.sin_port = htons(_bindPort);

	//���� ���Ͽ� bind �ϱ�
	ret = bind(_listenSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr));
	if (ret != 0)
	{
		logger->WriteLogConsole(SystemLog::LOG_LEVEL_ERROR, L"[����] bind()�Լ� ���� : %d\n", GetLastError());
		return false;
	}

	//���� ���� listen ���·� �����
	ret = listen(_listenSocket, SOMAXCONN);
	if (ret != 0)
	{
		logger->WriteLogConsole(SystemLog::LOG_LEVEL_ERROR, L"[����] listen()�Լ� ���� : %d\n", GetLastError());
		return false;
	}

	//RST�� �������� �� �� �ֵ��� ���� �ɼ��� �����Ѵ�.
	linger linger;
	linger.l_linger = 0;
	linger.l_onoff = true;
	setsockopt(_listenSocket, SOL_SOCKET, SO_LINGER, (char*)&linger, sizeof(linger));

	//�񵿱� IO�� Send�� �����ϵ��� L4�� Send���۸� 0���� �����.
	int sendBufSize = 0;
	setsockopt(_listenSocket, SOL_SOCKET, SO_SNDBUF, (const char*) &sendBufSize, sizeof(sendBufSize));

	//���ڷ� ���� nagle �ɼ��� �����Ѵ�.
	bool nagleOption = !_nagleOption;	//���̱��� ���°� TRUE �̹Ƿ� �ǵ��� �°� ��ȯ.
	setsockopt(_listenSocket, IPPROTO_TCP, TCP_NODELAY, (char*) &nagleOption, sizeof(nagleOption));

	_sessionContainer.InitContainer(_numOfMaxSession);

	//IOCP ����
	_hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, _IOCP_ConcurrentThreadNum);
	if (_hIOCP == NULL)
	{
		logger->WriteLogConsole(SystemLog::LOG_LEVEL_ERROR, L"[Error] CreateIoCompletionPort() ���� ���� : %d\n", GetLastError());
		return false;
	}

	//accept ������ ����
	_hAcceptThread = AcceptThreadStarter();

	//worker ������ ����
	for (int i = 0; i < _IOCP_WorkerThreadNum; i++)
	{
		HANDLE hWorkerThread = WorkerThreadStarter();
		_workerThreadVector.push_back(hWorkerThread);
	}

	//DebugInfo Thread ����
	_hDebugInfoThread = DebugInfoThreadStarter();

	//��ƺ����ڴٴ� ���� ���� ��ٸ� ���� Ÿ�̹��� ����ִ� ������ ����
	if (_sendIntervalTime != 0)
	{
		_hSendPostThread = SendPostThreadStarter();
	}

	logger->WriteLogConsole(
		SystemLog::LOG_LEVEL_SYSTEM, L"[System] ��Ʈ��ũ ���̺귯�� ��ŸƮ ����...\nBindIP: %s, BindPort: %d\nWorkerThreadNum : %d / ConcurrentThreadNum : %d\n",
		_bindIP, _bindPort, _IOCP_WorkerThreadNum, _IOCP_ConcurrentThreadNum);
	return true;
}

void NetServer::Stop()
{
	_isAcceptThreadRun = false;
	_isDebugInfoThreadRun = false;
	_isSendPostThreadRun = false;

	//Accept ������ ����
	closesocket(_listenSocket);
	WaitForSingleObject(_hAcceptThread, INFINITE);
	wprintf(L"# Accept ������ ���� �Ϸ�...\n");

	//���� ���� ���� �����ϱ�
	for (int i = 0; i < _numOfMaxSession; i++)
	{
		Session& session = _sessionContainer._sessionArray[i];
		//��ȿ�� �����̸� ������ ���� _sessionContainer���� Delete���ش�.
		if (!session.IsEmptyIndex())
		{
			Disconnect(session.GetSessionID());
		}
	}
	wprintf(L"# ���� ���� ���� ���� �Ϸ�...\n");

	//��Ŀ ������ �����ϱ�
	//_isWorkerThreadRun = false;
	for (int i = 0; i < _workerThreadVector.size(); i++)
	{
		PostQueuedCompletionStatus(_hIOCP, 0, 0, nullptr);
	}
	wprintf(L"# IOCP ��Ŀ ������ ���� ���� ��...\n");
	WaitForMultipleObjects(_workerThreadVector.size(), _workerThreadVector.data(), TRUE, INFINITE);
	wprintf(L"# IOCP ��Ŀ ������ ���� �Ϸ�...\n");

	for (HANDLE threadHandle : _workerThreadVector)
	{
		CloseHandle(threadHandle);
	}

	WSACleanup();
	wprintf(L"# ��Ʈ��ũ ���̺귯�� ����.\n");
}

int NetServer::GetSessionCount()
{
	return _sessionContainer.GetSessionNum();
}

void NetServer::Disconnect(__int64 sessionID)
{
	Session* pSession = AcquireSessionLicense(sessionID);
	if (pSession == nullptr)
	{
		return;
	}

	pSession->InvalidateSession();
	//IOCount�� 0���� �����Ѵ�.
	pSession->CancelIO();

	ReleaseSessionLicense(pSession);

	return;
}

char* NetServer::GetHeaderPtr(Message* pMessage)
{
	return pMessage->GetHeaderPtr();
}

void NetServer::RecvCompleteProc(DWORD transferred, UINT64 sessionID, Session* pSession)
{
	//recvQ ����
	int moveSize = pSession->RecvBufferMoveRear(transferred);

	//��Ŷ�� �������ݴ�� �ؼ��ؼ� �޽��� ������ OnRecv�� ���������� �޽��� ���ú긦 �˸���.
	while (1)
	{
		//����� �ְų� ����� ���ٸ� �ݺ��� Ż��
		if (pSession->GetIOBufferUseSize(IOOperation::RECV) <= sizeof(NetHeader))
			break;

		NetHeader header;
		pSession->RecvBufferPeek((char*)&header, sizeof(NetHeader));

		//����� �ڵ� �˻��ؼ� ��ó���� ���� ���̸� ���� ����
		if (header.code != _packetCode)
		{
			pSession->InvalidateSession();
			break;
		}
		if (header.len >= 65535)
		{
			Disconnect(sessionID);
		}

		//����� ��õ� ���̷ε庸�� ���� �����Ͱ� ������ �ݺ��� Ż��
		if (header.len > pSession->GetIOBufferUseSize(IOOperation::RECV) - sizeof(NetHeader))
		{
			break;
		}

		pSession->RecvBufferMoveFront(sizeof(NetHeader));

		//�������� ��Ŷ�� �������ֱ� ���� ����ȭ ���� �Ҵ�
		Message* pMessage = Message::Alloc();
		//���۷��� ī��Ʈ ���� ����
		pMessage->AddRefCount();
		//���ڵ� �Ǿ������� �����.
		pMessage->SetIsEncode(true);
		//���ۿ� �ִ� ���� ����ȭ ���ۿ� �ֱ�
		pSession->RecvBufferDequeue(pMessage->GetPayloadPtr(), header.len);
		//pSession->RecvBufferPeek(pMessage->GetPayloadPtr(), header.len);
		pMessage->MoveWritePos(header.len);

		//üũ���� ���ڵ��� ����̹Ƿ� ����ȭ ���ۿ� ����κп� ���� �ִ´�.
		NetHeader* pMessageHeader = reinterpret_cast<NetHeader*>(GetHeaderPtr(pMessage));
		pMessageHeader->checkSum = header.checkSum;

		//�޽��� ��ȣȭ
		bool decodeOK = pMessage->Decode(header.randKey, _encodeStaticKey);
		if (!decodeOK)
		{
			//���ڵ� ��� üũ�� ���� ��ġ���� ������ ���۵� �޽����� �����ϰ� ������ ���´�.
			Disconnect(sessionID);
			pMessage->SubRefCount();
			break;
		}

		OnRecv(sessionID, pMessage);
		if (!(pSession->IsValid()))
			break;
		InterlockedIncrement64(&_TPS._recvTPS);
	}

	//�ٽ� WSARecv �ɱ�
	bool retOfReqRecv = pSession->RequestRecv();
}

void NetServer::SendCompleteProc(DWORD transferred, UINT64 sessionID, Session* pSession)
{
	int sentMessageCount = pSession->SetSendingMessageCount(0);
	pSession->SentMessageFree(sentMessageCount);

	pSession->CompleteSending();

	//��Ƽ� ������ �ʴ� ���, ���⼭ �ٽ� SendQ�� ����� �˻��ؼ� ���� ���� �ִٸ� �ٽ� ������.
	//��Ƽ� ������ ���, ���� ���ݿ� ���� �� ���̹Ƿ� ���⼭ ������ �ʴ´�.
	if (_sendIntervalTime == 0)
	{
		int retOfReqSend = pSession->RequestSend();
	}

	OnSend(sessionID, transferred);
	InterlockedAdd64(&_TPS._sendTPS, sentMessageCount);
}

bool NetServer::SessionRelease(Session* pSession)
{
	//Release�� �ص� �Ǵ��� üũ
	if (!pSession->CheckCanRelease())
		return false;

	//����ȭ ���� Ǯ���� �Ҵ���� ����ȭ ���۰� ���� ���ŵ��� ���� ��� �޸� ������ �������� ���⼭ Ǯ�� ��ȯ���ش�.
	int sentMessageCount = pSession->SetSendingMessageCount(0);
	pSession->SentMessageFree(sentMessageCount);
	
	//SendQ ����ֱ�
	while (true)
	{
		Message* pMessage = pSession->SendBufferDequeue();
		if (pMessage == nullptr)
			break;
		pMessage->SubRefCount();
	}
	pSession->CloseSocket();

	INT64 sessionID = pSession->GetSessionID();
	OnRelease(sessionID);

	_sessionContainer.DeleteSession(sessionID);

	return true;
}

bool NetServer::IsSessionRelease(ULONG ioCount)
{
	return (ioCount & Session::RELEASE_BIT_MASK) == Session::RELEASE_BIT_MASK;
}

Session* NetServer::AcquireSessionLicense(UINT64 sessionID)
{
	//���� �˻�
	Session* pSession = _sessionContainer.SearchSession(sessionID);
	if (pSession == nullptr)
	{
		return nullptr;
	}

	//SendPaqcket ��ƾ�� �ϱ� ���� IOCount �������Ѽ� �ش� ������ �������� �ʵ��� ���´�.
	ULONG ioCount = pSession->IncreaseIOCount();

	//IOCount�� ���� 1��Ʈ�� ������ �÷����̹Ƿ� �� ��Ʈ�� �˻��ؼ� ���ǿ� ���� �۾��� ���������� �����Ѵ�.
	if (IsSessionRelease(ioCount))
	{
		ULONG ioCountForCheckingRelease = pSession->DecreaseIOCount();
		if (ioCountForCheckingRelease == 0)
		{
			SessionRelease(pSession);
		}
		return nullptr;
	}

	//�˻��ؿ� pSession�� �Լ��� ���� ������ ������ �´��� �˻�
	if (pSession->GetSessionID() != sessionID)
	{
		ULONG ioCountForCheckingRelease = pSession->DecreaseIOCount();
		if (ioCountForCheckingRelease == 0)
		{
			SessionRelease(pSession);
		}
		return nullptr;
	}

	return pSession;
}

void NetServer::ReleaseSessionLicense(Session* pSession)
{
	ULONG ioCountForCheckingRelease = pSession->DecreaseIOCount();
	if (ioCountForCheckingRelease == 0)
	{
		SessionRelease(pSession);
	}
}

bool NetServer::SendPacket(__int64 sessionID, Message* pMessage)
{
	pMessage->AddRefCount();

	Session* pSession = AcquireSessionLicense(sessionID);
	if (pSession == nullptr)
	{
		pMessage->SubRefCount();
		return false;
	}

	// ��Ŷ ���ڵ� 
	// (�ϳ��� ����ȭ ���ۿ� ���� �����尡 Send�� �ϴ� ���� ���� �ʿ����� �����Ƿ� ������� �ʴ´�. ���� �׷��� ����� �ʿ��� ��� ����)
	if (!pMessage->GetIsEncode())
	{
		pMessage->WriteHeader<Net>(_packetCode);
		pMessage->Encode(_encodeStaticKey);
	}


	//SendQueue�� ����ȭ ������ ������ �ֱ�
	std::optional<LONG> optionalEnqueueRet = pSession->SendBufferEnqueue(pMessage);	

	if (!optionalEnqueueRet.has_value())
	{
		//SendQueue�� ���� �� ���
		// 1. ��밡 ������ �� �ؼ� ����� ���� ���۰� ���� �������� �۽� �Ϸ� ������ �ȿ��� ���� �� �� ���� (������)
		// 2. ������ �� ���� �ʹ� ���� ��Ȳó�� ������ �ʹ� ������ ���� �� ���� �ִ�.
		// ���� �׷� ��� ������ ��� �ذ��Ѵ�.
		SystemLog::GetInstance()->WriteLog(
			L"[ERROR]", SystemLog::LOG_LEVEL_ERROR, L"[Error] sessionID: %lld / Full Of Send Queue\n",
			sessionID
		);
		Disconnect(sessionID);

		ReleaseSessionLicense(pSession);
		pMessage->SubRefCount();
		return false;
	}

	if (_sendIntervalTime == 0)
	{
		//WSASend ��û�ϱ�
		if (!PostQueuedCompletionStatus(_hIOCP, -1, sessionID, reinterpret_cast<LPOVERLAPPED>(SEND_PACKET_SIGN_PQCS)))
		{
			pMessage->SubRefCount();
			SystemLog::GetInstance()->WriteLog(
				L"[ERROR]", SystemLog::LOG_LEVEL_ERROR, L"[Error] sessionID: %lld / PQCS Failed\n",
				sessionID);
		}
	}
	else
	{
		ReleaseSessionLicense(pSession);
	}

	return true;
}

HANDLE NetServer::WorkerThreadStarter()
{
	auto threadFunc = [](void* pThis) -> unsigned int
		{
			NetServer* pNetServer = static_cast<NetServer*>(pThis);
			pNetServer->WorkerThread();
			return 0;
		};

	return (HANDLE)_beginthreadex(NULL, 0, threadFunc, this, 0, nullptr);
}

HANDLE NetServer::AcceptThreadStarter()
{
	auto threadFunc = [](void* pThis) -> unsigned int
		{
			NetServer* pNetServer = static_cast<NetServer*>(pThis);
			pNetServer->AcceptThread();
			return 0;
		};

	return (HANDLE)_beginthreadex(NULL, 0, threadFunc, this, 0, nullptr);
}

HANDLE NetServer::DebugInfoThreadStarter()
{
	auto threadFunc = [](void* pThis) -> unsigned int
		{
			NetServer* pNetServer = static_cast<NetServer*>(pThis);
			pNetServer->DebugInfoThread();
			return 0;
		};

	return (HANDLE)_beginthreadex(NULL, 0, threadFunc, this, 0, nullptr);
}

HANDLE NetServer::SendPostThreadStarter()
{
	auto threadFunc = [](void* pThis) -> unsigned int
		{
			NetServer* pNetServer = static_cast<NetServer*>(pThis);
			pNetServer->SendPostThread();
			return 0;
		};

	return (HANDLE)_beginthreadex(NULL, 0, threadFunc, this, 0, nullptr);
}

void NetServer::WorkerThread()
{
	while (_isWorkerThreadRun)
	{
		bool retOfGQCS = false;
		DWORD transferred = -1;
		unsigned __int64 sessionID = 10000000;
		OverlappedEx* pOverlapped = nullptr;
		Session* pSession = nullptr;

		retOfGQCS = GetQueuedCompletionStatus(_hIOCP, &transferred, &sessionID, (LPOVERLAPPED*)&pOverlapped, INFINITE);

		//////////////////////////////////
		//			���� ó��
		//////////////////////////////////
		//�����带 �ǵ������� �����Ű�� �� ��(���� ���� ��) 0, 0, 0�� Completion Queue�� �ִ´�.
		if (transferred == 0 && sessionID == 0 && pOverlapped == nullptr)
		{
			break;
		}

		// ���� �˻�
		pSession = _sessionContainer.SearchSession(sessionID);
		if (pSession == nullptr)
		{
			SystemLog::GetInstance()->WriteLogConsole(SystemLog::LOG_LEVEL_ERROR,
				L"[ERROR] sessionID: %lld / Session not found\n", sessionID);
			__debugbreak();
		}

		//��밡 fin�� ���������� ���� ���
		if (transferred == 0)
		{
			if (pOverlapped->_operation == IOOperation::SEND && retOfGQCS == true)
			{
				SystemLog::GetInstance()->WriteLogConsole(SystemLog::LOG_LEVEL_DEBUG,
					L"[DEBUG] sessionID: %lld / Send Transferred == 0\n", pSession->GetSessionID());
			}
			if (pOverlapped->_operation == IOOperation::RECV)
			{
				pSession->InvalidateSession();
			}
		}

		//////////////////////////////////
		//		IO Send ó��
		//////////////////////////////////
		if (reinterpret_cast<int>(pOverlapped) == SEND_PACKET_SIGN_PQCS)
		{
			pSession->RequestSend();
			ReleaseSessionLicense(pSession);
			continue;
		}

		//////////////////////////////////
		//		IO �Ϸ� ó��
		//////////////////////////////////
		if (pSession->IsValid() == true)
		{
			switch (pOverlapped->_operation)
			{
			case IOOperation::RECV:
				RecvCompleteProc(transferred, sessionID, pSession);
				break;
			case IOOperation::SEND:
				SendCompleteProc(transferred, sessionID, pSession);
				break;
			default:
				SystemLog::GetInstance()->WriteLog(L"ERROR", SystemLog::LOG_LEVEL_ERROR, L"�� �� ���� operation �Ϸ� ����");
				break;
			}
		}

		//////////////////////////////////////////
		//    IOCount ���� �� ���� ����ó��    //
		/////////////////////////////////////////
		int IOCountRet = pSession->DecreaseIOCount();
		if (pSession->IsPossibleToDisconnect())
		{
			SessionRelease(pSession);
		}

		if (IOCountRet < 0)
		{
			SystemLog::GetInstance()->WriteLogConsole(SystemLog::LOG_LEVEL_ERROR,
				L"[Error] sessionID : %d -> IOCount is negative number, IOCount : %d\n", sessionID, IOCountRet);
		}
	}

	return;
}

void NetServer::AcceptThread()
{
	while (_isAcceptThreadRun)
	{
		SOCKADDR_IN clientAddr;
		memset(&clientAddr, 0, sizeof(SOCKADDR_IN));
		int addrSize = sizeof(SOCKADDR_IN);

		SOCKET clientSock = accept(_listenSocket, (SOCKADDR*) &clientAddr, &addrSize);
		//���� ���Ḧ ���� _listenSocket�� ���� ���
		if (clientSock == INVALID_SOCKET)
		{
			break;
		}

		//MaxSession�� �ѱ�� ������ ���� �ʴ´�.
		if (_numOfMaxSession <= _sessionContainer.GetSessionNum())
		{
			SystemLog::GetInstance()->WriteLog(L"SYSTEM", SystemLog::LOG_LEVEL_SYSTEM, L"User accept over Max Session");
			closesocket(clientSock);
			continue;
		}

		WCHAR clientIPStr[16] = { 0, };
		InetNtop(AF_INET, &clientAddr.sin_addr, clientIPStr, 16);
		int clientPort = ntohs(clientAddr.sin_port);

		if (OnConnectionRequest(clientIPStr, clientPort) == false)
		{
			closesocket(clientSock);
			continue;
		}

		Session* pSession = _sessionContainer.InsertSession(_sessionIDGenerator++, clientSock, clientAddr.sin_addr, clientPort);
		if (pSession == nullptr)
		{
			SystemLog::GetInstance()->WriteLogConsole(SystemLog::LOG_LEVEL_ERROR, L"Insert Session Failed in Session Contaier");
		}

		//���ϰ� IOCP ����
		if (pSession->ConnectSocketToIOCP(_hIOCP) == false)
		{
			SystemLog::GetInstance()->WriteLogConsole(SystemLog::LOG_LEVEL_ERROR,
				L"sessionID : %d ConnectSocketToIOCP() fail\n", pSession->GetSessionID());
		}

		OnAccept(pSession->GetSessionID());
		InterlockedIncrement64(&_TPS._acceptTPS);

		//�񵿱� ���ú� �ɾ�α�
		bool retOfReqRecv = pSession->RequestRecv();
		pSession->DecreaseIOCount();
		//���� Recv �����ϸ� ���⼭ �������ش�.
		if (pSession->IsPossibleToDisconnect())
		{
			SessionRelease(pSession);
		}
	}
}

void NetServer::DebugInfoThread()
{
	while (_isDebugInfoThreadRun)
	{
		Sleep(1000);
		//TPS ������ ���
		InterlockedExchange64(&_prevTPS._acceptTPS, _TPS._acceptTPS);
		InterlockedExchange64(&_prevTPS._recvTPS, _TPS._recvTPS);
		InterlockedExchange64(&_prevTPS._sendTPS, _TPS._sendTPS);

		//accpet�� total�� �����ϱ�
		_totalAccept += _TPS._acceptTPS;

		//1�� �������� 0���� �����
		InterlockedExchange64(&_TPS._acceptTPS, 0);
		InterlockedExchange64(&_TPS._recvTPS, 0);
		InterlockedExchange64(&_TPS._sendTPS, 0);

		_prvSendThreadTPS = _sendThreadTPS;
		InterlockedExchange64(&_sendThreadTPS, 0);
	}

	return;
}

void NetServer::SendPostThread()
{
	_sendTick = timeGetTime();

	while (_isSendPostThreadRun)
	{
		// _sendIntervalTime�� �������� �˻�
		DWORD deltaTime = timeGetTime() - _sendTick;

		if (deltaTime < static_cast<DWORD>(_sendIntervalTime))
		{
			Sleep(_sendIntervalTime - deltaTime);
		}
		_sendTick += _sendIntervalTime;
		_sendThreadTPS++;

		//��ü ���� ���鼭 ��Ҵ� SendPacket���� Send �ϱ�
		for (int i = 0; i < _numOfMaxSession; i++)
		{
			UINT64 sessionID = _sessionContainer._sessionArray[i].GetSessionID();
			Session* pSession = AcquireSessionLicense(sessionID);
			if (pSession == nullptr) 
				continue;

			if(pSession->GetIOBufferUseSize(IOOperation::SEND) >= _sendOnceMin)
				pSession->RequestSend();

			ReleaseSessionLicense(pSession);
		}
	}
	return;
}
#pragma comment(lib, "ws2_32")
#pragma comment(lib, "Winmm.lib")
#include "LanServer.h"
#include <WS2tcpip.h>
#include "PacketProtocol.h"
#include "Tools/SystemLog.h"
#include "Tools/Message.h"
#include "Session.h"
#include "Tools/Parser.h"

LanServer::LanServer()
	:_totalAccept(0), _hAcceptThread(INVALID_HANDLE_VALUE), _hDebugInfoThread(INVALID_HANDLE_VALUE), _hSendPostThread(INVALID_HANDLE_VALUE), _hTimerThread(INVALID_HANDLE_VALUE),
	_isAcceptThreadRun(true), _isWorkerThreadRun(true), _isDebugInfoThreadRun(true), _isSendPostThreadRun(true), _isTimerThreadRun(true),
	_numOfMaxSession(0), _numOfMaxUser(0), _nagleOption(false), _IOCP_WorkerThreadNum(0), _IOCP_ConcurrentThreadNum(0),
	_packetCode(0), _timeoutDisconnectForSession(0), _timeoutDisconnectForUser(0), _defaultContentID(std::nullopt)
{
	InitializeSRWLock(&_frameTimePriorityQueueLock);
}

LanServer::~LanServer()
{
}

bool LanServer::Start(const WCHAR* configFileName)
{
	SystemLog* logger = SystemLog::GetInstance();

	///////////////////////////////////////////////////
	//		config ���Ͽ��� Parser�� �� �о����
	///////////////////////////////////////////////////
	Parser parser;
	if (!parser.LoadFile(configFileName))
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
	logger->SetLogDirectory(L"Log_Directory");


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
	ret = listen(_listenSocket, SOMAXCONN_HINT(1000));
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
	setsockopt(_listenSocket, SOL_SOCKET, SO_SNDBUF, (const char*)&sendBufSize, sizeof(sendBufSize));

	//���ڷ� ���� nagle �ɼ��� �����Ѵ�.
	bool nagleOption = !_nagleOption;	//���̱��� ���°� TRUE �̹Ƿ� �ǵ��� �°� ��ȯ.
	setsockopt(_listenSocket, IPPROTO_TCP, TCP_NODELAY, (char*)&nagleOption, sizeof(nagleOption));

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

	//DebugInfo ������ ����
	_hDebugInfoThread = DebugInfoThreadStarter();

	//TimerThread ����
	_hTimerThread = TimerThreadStarter();

	//��ƺ����ڴٴ� ���� ���� ��ٸ� SendPostThread ����
	if (_sendIntervalTime != 0)
	{
		_hSendPostThread = SendPostThreadStarter();
	}

	logger->WriteLogConsole(
		SystemLog::LOG_LEVEL_SYSTEM, L"[System] ��Ʈ��ũ ���̺귯�� ��ŸƮ ����...\nBindIP: %s, BindPort: %d\nWorkerThreadNum : %d / ConcurrentThreadNum : %d\n",
		_bindIP, _bindPort, _IOCP_WorkerThreadNum, _IOCP_ConcurrentThreadNum);
	return true;
}

void LanServer::Stop()
{
	_isAcceptThreadRun = false;
	_isDebugInfoThreadRun = false;
	_isSendPostThreadRun = false;
	_isTimerThreadRun = false;
		
	//Accept ������ ����
	closesocket(_listenSocket);
	WaitForSingleObject(_hAcceptThread, INFINITE);
	wprintf(L"# Accept ������ ���� �Ϸ�...\n");

	//���� ���� ���� �����ϱ�
	for (int i = 0; i < _numOfMaxSession; i++)
	{
		Session& session = _sessionContainer._sessionArray[i];
		//��ȿ�� �����̸� ������ ���� _sessionContainer���� Delete���ش�.
		if (!session._isEmptyIndex)
		{
			Disconnect(session._sessionID);
		}
	}
	wprintf(L"# ���� ���� ���� ���� �Ϸ�...\n");

	//��Ŀ ������ �����ϱ�
	for (int i = 0; i < _workerThreadVector.size(); i++)
	{
		PostQueuedCompletionStatus(_hIOCP, 0, 0, nullptr);
	}
	wprintf(L"# IOCP ��Ŀ ������ ���� ���� ��...\n");
	WaitForMultipleObjects(static_cast<DWORD>(_workerThreadVector.size()), _workerThreadVector.data(), TRUE, INFINITE);
	wprintf(L"# IOCP ��Ŀ ������ ���� �Ϸ�...\n");

	for (HANDLE threadHandle : _workerThreadVector)
	{
		CloseHandle(threadHandle);
	}

	WSACleanup();
	wprintf(L"# ��Ʈ��ũ ���̺귯�� ����.\n");
}

void LanServer::Disconnect(UINT64 sessionID)
{
	Session* pSession = AcquireSessionLicense(sessionID);
	if (pSession == nullptr)
	{
		return;
	}

	pSession->InvalidateSession();
	//IOCount�� 0���� �����Ѵ�.
	CancelIoEx((HANDLE)pSession->_socket, NULL);

	ReleaseSessionLicense(pSession);
	return;
}

bool LanServer::AddContents(INT32 key, const Contents* pContents)
{
	if (pContents == nullptr)
		return false;

	auto result = _contentsHashTable.insert({ key, const_cast<Contents*>(pContents) });
	if (result.second == false)
		return false;

	int contentsFPS = pContents->GetFPS();
	if (contentsFPS != 0)
	{
		int intervalMs = 1000 / contentsFPS; // FPS�� ���� ȣ�� ����(ms) ���
		auto now = std::chrono::steady_clock::now();
		FrameTask task{ now + std::chrono::milliseconds(intervalMs), key, intervalMs };

		AcquireSRWLockExclusive(&_frameTimePriorityQueueLock);
		_frameTimePriorityQueue.push(task);
		ReleaseSRWLockExclusive(&_frameTimePriorityQueueLock);
	}


	return true;
}

void LanServer::SetDefaultContents(INT32 contentsID)
{
	_defaultContentID = contentsID;
}

void LanServer::RecvCompleteProc(DWORD transferred, UINT64 sessionID, Session* pSession)
{
	//recvQ ����
	int moveSize = pSession->_recvQ.MoveRear(transferred);

	//��Ŷ�� �������ݴ�� �ؼ��ؼ� �޽��� ������ OnRecv�� ���������� �޽��� ���ú긦 �˸���.
	while (1)
	{
		//����� �ְų� ����� ���ٸ� �ݺ��� Ż��
		if (pSession->_recvQ.GetUseSize() <= sizeof(LanHeader))
			break;

		LanHeader header;
		pSession->_recvQ.Peek((char*)&header, sizeof(LanHeader));

		//�͹��� ���� ū len���� ������ ���´�.
		if (header.len >= 65535)
		{
			SystemLog::GetInstance()->WriteLog(L"ERROR", SystemLog::LOG_LEVEL_ERROR, L"header len is over65535");
			Disconnect(sessionID);
		}

		//����� ��õ� ���̷ε庸�� ���� �����Ͱ� ������ �ݺ��� Ż��
		if (header.len > pSession->_recvQ.GetUseSize() - sizeof(LanHeader))
		{
			break;
		}

		pSession->_recvQ.MoveFront(sizeof(LanHeader));

		//�������� ��Ŷ�� �������ֱ� ���� ����ȭ ���� �Ҵ�
		Message* pMessage = Message::Alloc();
		//���۷��� ī��Ʈ ���� ����
		pMessage->AddRefCount();
		//���ۿ� �ִ� ���� ����ȭ ���ۿ� �ֱ�
		pSession->_recvQ.Dequeue(pMessage->GetPayloadPtr(), header.len);
		//pSession->RecvBufferPeek(pMessage->GetBufferPtr(), header.len);
		pMessage->MoveWritePos(header.len);

		auto opt_contentsID = pSession->_contentsID;
		if (opt_contentsID.has_value())
		{
			Contents* pContents = _contentsHashTable[opt_contentsID.value()];
			if (pContents->IsSerial())	//�޽��� ó���� ����ȭ�� �ʿ��� ������ (ex. �ʵ�)
			{
				pContents->ELock();
				pContents->OnRecv(sessionID, pMessage);
				pContents->EUnlock();
			}
			else						//�޽��� ó���� ����ȭ�� �ʿ����� ���� ������ (ex. �κ�)
			{
				pContents->OnRecv(sessionID, pMessage);
			}
		}
		else
		{
			OnRecv(sessionID, pMessage);
		}

		if (!(pSession->_isValid))
		{
			break;
		}
		InterlockedIncrement64(&_TPS._recvTPS);
	}

	pSession->CompleteRecving();

	// SendPacket_Disconnect �Լ��� ȣ���� ��� �ٽ� Recv�� ���� �ʴ´�.
	if (pSession->GetAsyncRecvBlockFlag() == FALSE)
	{
		//�ٽ� WSARecv �ɱ�
		bool retOfReqRecv = pSession->RequestRecv();
	}
}

void LanServer::SendCompleteProc(DWORD transferred, UINT64 sessionID, Session* pSession)
{
	int sentMessageCount = InterlockedExchange(&pSession->_sendingMessageCount, 0);
	pSession->SentMessageFree(sentMessageCount);

	pSession->CompleteSending();

	//��Ƽ� ������ �ʴ� ���, ���⼭ �ٽ� ������
	//��Ƽ� ������ ���, ���� ���ݿ� ���� �� ���̹Ƿ� ���⼭ ������ �ʴ´�.
	if (_sendIntervalTime == 0)
	{
			int retOfReqSend = pSession->RequestSend();
	}

	//OnSend(sessionID, transferred);
	InterlockedAdd64(&_TPS._sendTPS, sentMessageCount);
}

bool LanServer::SessionRelease(Session* pSession)
{
	//Release�� �ص� �Ǵ��� üũ
	if (!pSession->CheckCanRelease())
	{
		return false;
	}
	closesocket(pSession->_socket);
	pSession->_socket = INVALID_SOCKET;
	//����ȭ ���� Ǯ���� �Ҵ���� ����ȭ ���۰� ���� ���ŵ��� ���� ��� �޸� ������ �������� ���⼭ Ǯ�� ��ȯ���ش�.
	int sentMessageCount = InterlockedExchange(&pSession->_sendingMessageCount, 0);

	pSession->SentMessageFree(sentMessageCount);

	//SendQ ����ֱ�
	while (true)
	{
		auto pMessageOpt = pSession->_sendQ.Dequeue();
		if (pMessageOpt.has_value())
			pMessageOpt.value()->SubRefCount();
		else
			break;
	}

	__int64 sessionID = pSession->_sessionID;
	//�ٸ� ��Ŀ �����尡 OnRelease�� ȣ�� �ϰ� �ϱ� ���� PQCS�� IOCP�� �ϰ��� �����ش�.
	//�ڱⰡ �ع����� ������� �߻��� �� ����.
	PQCS(100, sessionID, reinterpret_cast<LPOVERLAPPED>(PQCS_SIGN::RELEASE));

	return true;
}

bool LanServer::IsSessionRelease(ULONG ioCount)
{
	return (ioCount & Session::RELEASE_BIT_MASK) == Session::RELEASE_BIT_MASK;
}

Session* LanServer::AcquireSessionLicense(UINT64 sessionID)
{
	//���� �˻�
	Session* pSession = _sessionContainer.SearchSession(sessionID);
	if (pSession == nullptr)
	{
		return nullptr;
	}

	//SendPaqcket ��ƾ�� �ϱ� ���� IOCount �������Ѽ� �ش� ������ �������� �ʵ��� ���´�.
	ULONG ioCount = InterlockedIncrement(&pSession->_IOCount);

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
	if (pSession->_sessionID != sessionID)
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

void LanServer::ReleaseSessionLicense(Session* pSession)
{
	ULONG ioCountForCheckingRelease = pSession->DecreaseIOCount();
	if (ioCountForCheckingRelease == 0)
	{
		SessionRelease(pSession);
	}
}

void LanServer::HandleSendPakcetPQCS(INT64 sessionID)
{
	Session* pSession = _sessionContainer.SearchSession(sessionID);
	if (pSession == nullptr)
	{
		SystemLog::GetInstance()->WriteLogConsole(SystemLog::LOG_LEVEL_ERROR,
			L"[ERROR] sessionID: %d / Session not found\n", sessionID);
		__debugbreak();
	}

	pSession->RequestSend();
	ReleaseSessionLicense(pSession);
}

void LanServer::HandleReleasePQCS(INT64 sessionID)
{
	Session* pSession = _sessionContainer.SearchSession(sessionID);
	if (pSession == nullptr)
	{
		SystemLog::GetInstance()->WriteLogConsole(SystemLog::LOG_LEVEL_ERROR,
			L"[ERROR] sessionID: %d / Session not found\n", sessionID);
		__debugbreak();
	}

	_sessionContainer.DeleteSession(sessionID);

	if (pSession->_contentsID.has_value())
	{
		Contents* contents = _contentsHashTable[pSession->_contentsID.value()];
		contents->ELock();
		if (!contents->DeleteSessionID(sessionID))
			__debugbreak();
		contents->OnDisconnect(sessionID);
		contents->EUnlock();
	}
	else
	{
		OnRelease(sessionID);
	}
}

void LanServer::HandleTimerPQCS()
{
	OnTime();
}

void LanServer::HandleMoveContentsPQCS(DWORD transferred, INT64 sessionID)
{
	Session* pSession = _sessionContainer.SearchSession(sessionID);
	if (pSession == nullptr)
	{
		SystemLog::GetInstance()->WriteLogConsole(SystemLog::LOG_LEVEL_ERROR,
			L"[ERROR] sessionID: %d / Session not found\n", sessionID);
		__debugbreak();
	}
	INT32 destContentsID = static_cast<INT32>(transferred);
	Contents* pDestContents =_contentsHashTable[destContentsID];

	//������ �̹� �������� �ҼӵǾ� �ִ� ��� �װ����� ���ش�.
	if (pSession->_contentsID.has_value())
	{
		Contents* pCurrentContents =_contentsHashTable[pSession->_contentsID.value()];
		pCurrentContents->ELock();
		//�������� �ش� �������� �˸���.
		if (!pCurrentContents->NotifyLeaveContents(sessionID))
		{
			pCurrentContents->EUnlock();
			ReleaseSessionLicense(pSession);
			return;
		}
		pCurrentContents->EUnlock();
	}

	//������ �־��� ������ ���̵�� �Ҽӽ�Ų��.
	pSession->EnterContents(destContentsID);

	pDestContents->ELock();
	//�Ҽ� �������� �ش� �������� �˸���.
	if (!pDestContents->NotifyEnterContents(sessionID))
	{
		pDestContents->EUnlock();
		ReleaseSessionLicense(pSession);
		return;
	}
	pDestContents->EUnlock();

	//������ �̵� ���� ���� �Ϸ� ������ ó���Ǹ� �޽����� ���ǵǱ� ������
	//������ �̵��� ��ģ �� ���⼭ �񵿱� Recv�� �ɵ��� �Ѵ�.
	pSession->RequestRecv();

	pSession->ResetAsyncRecvBlockFlag();

	ReleaseSessionLicense(pSession);
}

void LanServer::HandleFrameLogicPQCS(DWORD transferred, INT64 sessionID)
{
	INT32 contentsID = static_cast<INT32>(transferred);
	INT64 inputTime = static_cast<INT64>(sessionID);
	Contents* contents = _contentsHashTable[contentsID];
	INT64 lateTime = std::chrono::duration_cast<std::chrono::milliseconds>(
		std::chrono::steady_clock::now().time_since_epoch()).count()
		- inputTime;
	contents->ELock();
	contents->OnUpdate(lateTime);
	contents->EUnlock();
}

bool LanServer::SendPacket(UINT64 sessionID, Message* pMessage, bool usePQCS, bool disconnect)
{
	pMessage->AddRefCount();

	Session* pSession = AcquireSessionLicense(sessionID);
	if (pSession == nullptr)
	{
		pMessage->SubRefCount();
		return false;
	}

	pMessage->WriteHeader<Lan>(_packetCode);

	//SendQueue�� ����ȭ ������ ������ �ֱ�
	std::optional<LONG> optionalEnqueueRet = pSession->_sendQ.Enqueue(pMessage);

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

	//WSASend ��û�ϱ�
	if (_sendIntervalTime == 0)
	{
		if (usePQCS)
		{
			int transferred = pMessage->GetDataSize() + pMessage->GetHeaderSize<Lan>();
			if (!PostQueuedCompletionStatus(_hIOCP, transferred, sessionID, reinterpret_cast<LPOVERLAPPED>(PQCS_SIGN::SEND_PACKET)))
			{
				SystemLog::GetInstance()->WriteLog(
					L"[ERROR]", SystemLog::LOG_LEVEL_ERROR, L"[Error] sessionID: %lld / PQCS Failed\n",
					sessionID);
			}
		}
		else
		{
			pSession->RequestSend();
			ReleaseSessionLicense(pSession);
		}
	}
	else
	{
		ReleaseSessionLicense(pSession);
	}

	if (disconnect)
	{
		//���⼭ ���� WSASend�� �Ϸ� ������ ���� IOCount�� 0�� �ǵ��� flag�� ������ �񵿱� Recv�� ���� ���ϰ� �Ѵ�.
		pSession->SetAsyncRecvBlockFlag();
		CancelIoEx((HANDLE)pSession->_socket, (LPOVERLAPPED)&pSession->_recvOverlapped);
	}

	return true;
}

void LanServer::RequestMoveContents(UINT64 sessionID, INT32 destContentsID)
{
	//���⼭ �ø� IOCount�� PQCS �Ϸ� ó������ ������.
	Session* pSession = AcquireSessionLicense(sessionID);
	if (pSession == nullptr)
	{
		SystemLog::GetInstance()->WriteLog(L"ERROR", SystemLog::LOG_LEVEL_ERROR, L"pSession is nullptr in RequestMoveContents");
		return;
	}

	//������ �̵� ���� ��, �ش� ������ �񵿱� Recv�� ���� �ʵ��� �ؼ� �޽��� ó���� �ϴ� ��Ȳ�� ���´�.
	pSession->SetAsyncRecvBlockFlag();
	
	PQCS(destContentsID, sessionID, reinterpret_cast<LPOVERLAPPED>(PQCS_SIGN::MOVE_CONTENTS));
}

HANDLE LanServer::WorkerThreadStarter()
{
	auto threadFunc = [](void* pThis) -> unsigned int
		{
			LanServer* pLanServer = static_cast<LanServer*>(pThis);
			pLanServer->WorkerThread();
			return 0;
		};

	return (HANDLE)_beginthreadex(NULL, 0, threadFunc, this, 0, nullptr);
}

HANDLE LanServer::AcceptThreadStarter()
{
	auto threadFunc = [](void* pThis) -> unsigned int
		{
			LanServer* pLanServer = static_cast<LanServer*>(pThis);
			pLanServer->AcceptThread();
			return 0;
		};

	return (HANDLE)_beginthreadex(NULL, 0, threadFunc, this, 0, nullptr);
}

HANDLE LanServer::DebugInfoThreadStarter()
{
	auto threadFunc = [](void* pThis) -> unsigned int
		{
			LanServer* pLanServer = static_cast<LanServer*>(pThis);
			pLanServer->DebugInfoThread();
			return 0;
		};

	return (HANDLE)_beginthreadex(NULL, 0, threadFunc, this, 0, nullptr);
}

HANDLE LanServer::SendPostThreadStarter()
{
	auto threadFunc = [](void* pThis) -> unsigned int
		{
			LanServer* pLanServer = static_cast<LanServer*>(pThis);
			pLanServer->SendPostThread();
			return 0;
		};

	return (HANDLE)_beginthreadex(NULL, 0, threadFunc, this, 0, nullptr);
}

HANDLE LanServer::TimerThreadStarter()
{
	auto threadFunc = [](void* pThis) -> unsigned int
		{
			LanServer* pLanServer = static_cast<LanServer*>(pThis);
			pLanServer->TimerThread();
			return 0;
		};

	return (HANDLE)_beginthreadex(NULL, 0, threadFunc, this, 0, nullptr);
}

void LanServer::WorkerThread()
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

		switch (reinterpret_cast<INT64>(pOverlapped))
		{
		case PQCS_SIGN::SEND_PACKET:
			HandleSendPakcetPQCS(sessionID);
			continue;
		case PQCS_SIGN::RELEASE:
			HandleReleasePQCS(sessionID);
			continue;
		case PQCS_SIGN::TIMER:
			HandleTimerPQCS();
			continue;
		case PQCS_SIGN::MOVE_CONTENTS:
			HandleMoveContentsPQCS(transferred, sessionID);
			continue;
		case PQCS_SIGN::FRAME_LOGIC:
			HandleFrameLogicPQCS(transferred, sessionID);
			continue;
		}

		// ���� �˻�
		pSession = _sessionContainer.SearchSession(sessionID);
		if (pSession == nullptr)
		{
			SystemLog::GetInstance()->WriteLogConsole(SystemLog::LOG_LEVEL_ERROR,
				L"[ERROR] sessionID: %d / Session not found\n", sessionID);
			__debugbreak();
		}

		//��밡 fin�� ���������� ���� ���
		if (transferred == 0)
		{
			if (pOverlapped->_operation == IOOperation::SEND && retOfGQCS == true)
			{
				SystemLog::GetInstance()->WriteLogConsole(SystemLog::LOG_LEVEL_DEBUG,
					L"[DEBUG] sessionID: %d / Send Transferred == 0\n", pSession->_sessionID);
			}
			if (pOverlapped->_operation == IOOperation::RECV)
			{
				pSession->InvalidateSession();
			}
		}

		//////////////////////////////////
		//		IO �Ϸ� ó��
		//////////////////////////////////
		if (pSession->_isValid == true)
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
		if (IOCountRet == 0)
		{
			SessionRelease(pSession);
		}

	}

	return;
}

void LanServer::AcceptThread()
{
	while (_isAcceptThreadRun)
	{
		SOCKADDR_IN clientAddr;
		memset(&clientAddr, 0, sizeof(SOCKADDR_IN));
		int addrSize = sizeof(SOCKADDR_IN);

		SOCKET clientSock = accept(_listenSocket, (SOCKADDR*)&clientAddr, &addrSize);
		//���� ���Ḧ ���� _listenSocket�� ���� ���
		if (clientSock == INVALID_SOCKET)
		{
			break;
		}

		//MaxSession�� �ѱ�� ������ ���� �ʴ´�.
		if (_numOfMaxSession <= _sessionContainer.GetSessionNum())
		{
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
				L"sessionID : %d ConnectSocketToIOCP() fail\n", pSession->_sessionID);
		}

		OnAccept(pSession->_sessionID);
		InterlockedIncrement64(&_TPS._acceptTPS);

		//����Ʈ �������� �����ϸ� ������ �ش� �������� �Ҽӽ�Ų��.
		if (_defaultContentID.has_value())
		{
			RequestMoveContents(pSession->_sessionID, _defaultContentID.value());
		}

		//�񵿱� ���ú� �ɾ�α�
		if (!pSession->GetAsyncRecvBlockFlag())
		{
			bool retOfReqRecv = pSession->RequestRecv();
		}
		pSession->DecreaseIOCount();
		//���� Recv �����ϸ� ���⼭ �������ش�.
		if (pSession->IsPossibleToDisconnect())
		{
			SessionRelease(pSession);
		}
	}
}

void LanServer::DebugInfoThread()
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

void LanServer::SendPostThread()
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
			UINT64 sessionID = _sessionContainer._sessionArray[i]._sessionID;
			Session* pSession = AcquireSessionLicense(sessionID);
			if (pSession == nullptr)
				continue;

			pSession->RequestSend();

			ReleaseSessionLicense(pSession);
		}
	}
}

void LanServer::TimerThread()
{
	// �����ӿ� �� ���� PQCS ���ֱ�
	// �䱸 ���� : �����尡 ��� ���� �� ��. �� �ñ׳��� �� �� �Ͼ�� �ñ׳� �ְ� �ٽ� �ڷ�������.
	// OnUpdate�� �и� �������� �ִٸ� �󸶳� �зȴ����� ��������� ��.

	const int ONE_SECOND_TASK_ID = -1;
	auto now = std::chrono::steady_clock::now();
	//1�ʿ� �� ���� OnTime ȣ��ǵ��� ���� �ִ´�.
	FrameTask oneSecondTask{ now + std::chrono::milliseconds(1000), ONE_SECOND_TASK_ID, 1000 };

	AcquireSRWLockExclusive(&_frameTimePriorityQueueLock);
	_frameTimePriorityQueue.push(oneSecondTask);
	ReleaseSRWLockExclusive(&_frameTimePriorityQueueLock);

	while (_isTimerThreadRun)
	{
		std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
		AcquireSRWLockExclusive(&_frameTimePriorityQueueLock);
		auto nextTask = _frameTimePriorityQueue.top();
		ReleaseSRWLockExclusive(&_frameTimePriorityQueueLock);


		if (now >= nextTask._executionTime) 
		{
			// �۾� ����
			AcquireSRWLockExclusive(&_frameTimePriorityQueueLock);
			_frameTimePriorityQueue.pop();

			if (nextTask._contentID == ONE_SECOND_TASK_ID)
			{
				PQCS(NULL, NULL, reinterpret_cast<LPOVERLAPPED>(PQCS_SIGN::TIMER));
			}
			else
			{
				//IOCP�� CompletionStatus�� ���ش�.
				PQCS(static_cast<DWORD>(nextTask._contentID), 
					std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count(),
					reinterpret_cast<LPOVERLAPPED>(PQCS_SIGN::FRAME_LOGIC));
			}

			// ���� ���� �ð� ��� �� �ٽ� ť�� ����
			FrameTask newTask((now + std::chrono::milliseconds(nextTask._intervalMs)), nextTask._contentID, nextTask._intervalMs);
			_frameTimePriorityQueue.push(newTask);
			ReleaseSRWLockExclusive(&_frameTimePriorityQueueLock);
		}
		else 
		{
			// ���� �۾����� ���
			INT64 sleepDuration = std::chrono::duration_cast<std::chrono::milliseconds>(nextTask._executionTime - now).count();
			std::this_thread::sleep_for(std::chrono::milliseconds(sleepDuration));
		}
	}
}

void LanServer::PQCS(DWORD transferred, ULONG_PTR completionKey, LPOVERLAPPED overlapped)
{
	PostQueuedCompletionStatus(_hIOCP, transferred, completionKey, overlapped);
}

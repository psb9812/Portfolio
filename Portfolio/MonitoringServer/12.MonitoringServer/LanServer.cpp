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
	//		config 파일에서 Parser로 값 읽어오기
	///////////////////////////////////////////////////
	Parser parser;
	if (!parser.LoadFile(configFileName))
	{
		logger->WriteLogConsole(SystemLog::LOG_LEVEL_ERROR, L"[에러] Config 파일 로드 실패 : %d\n", GetLastError());
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

	//시스템 로그 설정
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
		logger->WriteLogConsole(SystemLog::LOG_LEVEL_ERROR, L"[에러] 알 수 없는 로그 레벨 : %d\n", GetLastError());
		return false;
	}
	logger->SetLogDirectory(L"Log_Directory");


	///////////////////////////////////////////////////
	//		네트워크 설정 시작
	///////////////////////////////////////////////////
	WSADATA wsa;
	int ret = WSAStartup(MAKEWORD(2, 2), &wsa);
	if (ret != 0)
	{
		logger->WriteLogConsole(SystemLog::LOG_LEVEL_ERROR, L"[에러] WSAStartup()함수 실패 : %d\n", GetLastError());
		return false;
	}

	//리슨 소켓 생성
	_listenSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (_listenSocket == INVALID_SOCKET)
	{
		logger->WriteLogConsole(SystemLog::LOG_LEVEL_ERROR, L"[에러] socket() 함수 실패 : %d\n", GetLastError());
	}


	//서버 주소 구조체 세팅
	SOCKADDR_IN serverAddr;
	memset(&serverAddr, 0, sizeof(SOCKADDR_IN));
	serverAddr.sin_family = AF_INET;
	//InetPton(AF_INET, serverIP, &serverAddr.sin_addr);
	serverAddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
	serverAddr.sin_port = htons(_bindPort);

	//리슨 소켓에 bind 하기
	ret = bind(_listenSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr));
	if (ret != 0)
	{
		logger->WriteLogConsole(SystemLog::LOG_LEVEL_ERROR, L"[에러] bind()함수 실패 : %d\n", GetLastError());
		return false;
	}

	//리슨 소켓 listen 상태로 만들기
	ret = listen(_listenSocket, SOMAXCONN_HINT(1000));
	if (ret != 0)
	{
		logger->WriteLogConsole(SystemLog::LOG_LEVEL_ERROR, L"[에러] listen()함수 실패 : %d\n", GetLastError());
		return false;
	}

	//RST로 강제종료 할 수 있도록 링거 옵션을 설정한다.
	linger linger;
	linger.l_linger = 0;
	linger.l_onoff = true;
	setsockopt(_listenSocket, SOL_SOCKET, SO_LINGER, (char*)&linger, sizeof(linger));

	//비동기 IO로 Send를 수행하도록 L4의 Send버퍼를 0으로 만든다.
	int sendBufSize = 0;
	setsockopt(_listenSocket, SOL_SOCKET, SO_SNDBUF, (const char*)&sendBufSize, sizeof(sendBufSize));

	//인자로 받은 nagle 옵션을 설정한다.
	bool nagleOption = !_nagleOption;	//네이글을 끄는게 TRUE 이므로 의도에 맞게 변환.
	setsockopt(_listenSocket, IPPROTO_TCP, TCP_NODELAY, (char*)&nagleOption, sizeof(nagleOption));

	_sessionContainer.InitContainer(_numOfMaxSession);

	//IOCP 생성
	_hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, _IOCP_ConcurrentThreadNum);
	if (_hIOCP == NULL)
	{
		logger->WriteLogConsole(SystemLog::LOG_LEVEL_ERROR, L"[Error] CreateIoCompletionPort() 생성 실패 : %d\n", GetLastError());
		return false;
	}

	//accept 스레드 생성
	_hAcceptThread = AcceptThreadStarter();

	//worker 스레드 생성
	for (int i = 0; i < _IOCP_WorkerThreadNum; i++)
	{
		HANDLE hWorkerThread = WorkerThreadStarter();
		_workerThreadVector.push_back(hWorkerThread);
	}

	//DebugInfo 스레드 생성
	_hDebugInfoThread = DebugInfoThreadStarter();

	//TimerThread 생성
	_hTimerThread = TimerThreadStarter();

	//모아보내겠다는 설정 값을 줬다면 SendPostThread 생성
	if (_sendIntervalTime != 0)
	{
		_hSendPostThread = SendPostThreadStarter();
	}

	logger->WriteLogConsole(
		SystemLog::LOG_LEVEL_SYSTEM, L"[System] 네트워크 라이브러리 스타트 성공...\nBindIP: %s, BindPort: %d\nWorkerThreadNum : %d / ConcurrentThreadNum : %d\n",
		_bindIP, _bindPort, _IOCP_WorkerThreadNum, _IOCP_ConcurrentThreadNum);
	return true;
}

void LanServer::Stop()
{
	_isAcceptThreadRun = false;
	_isDebugInfoThreadRun = false;
	_isSendPostThreadRun = false;
	_isTimerThreadRun = false;
		
	//Accept 스레드 정리
	closesocket(_listenSocket);
	WaitForSingleObject(_hAcceptThread, INFINITE);
	wprintf(L"# Accept 스레드 정리 완료...\n");

	//접속 중인 세션 정리하기
	for (int i = 0; i < _numOfMaxSession; i++)
	{
		Session& session = _sessionContainer._sessionArray[i];
		//유효한 세션이면 연결을 끊고 _sessionContainer에서 Delete해준다.
		if (!session._isEmptyIndex)
		{
			Disconnect(session._sessionID);
		}
	}
	wprintf(L"# 접속 중인 세션 정리 완료...\n");

	//워커 스레드 정리하기
	for (int i = 0; i < _workerThreadVector.size(); i++)
	{
		PostQueuedCompletionStatus(_hIOCP, 0, 0, nullptr);
	}
	wprintf(L"# IOCP 워커 스레드 정리 진행 중...\n");
	WaitForMultipleObjects(static_cast<DWORD>(_workerThreadVector.size()), _workerThreadVector.data(), TRUE, INFINITE);
	wprintf(L"# IOCP 워커 스레드 정리 완료...\n");

	for (HANDLE threadHandle : _workerThreadVector)
	{
		CloseHandle(threadHandle);
	}

	WSACleanup();
	wprintf(L"# 네트워크 라이브러리 종료.\n");
}

void LanServer::Disconnect(UINT64 sessionID)
{
	Session* pSession = AcquireSessionLicense(sessionID);
	if (pSession == nullptr)
	{
		return;
	}

	pSession->InvalidateSession();
	//IOCount를 0으로 유도한다.
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
		int intervalMs = 1000 / contentsFPS; // FPS에 따라 호출 간격(ms) 계산
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
	//recvQ 정리
	int moveSize = pSession->_recvQ.MoveRear(transferred);

	//패킷을 프로토콜대로 해석해서 메시지 단위로 OnRecv로 컨텐츠에게 메시지 리시브를 알린다.
	while (1)
	{
		//헤더만 있거나 헤더도 없다면 반복문 탈출
		if (pSession->_recvQ.GetUseSize() <= sizeof(LanHeader))
			break;

		LanHeader header;
		pSession->_recvQ.Peek((char*)&header, sizeof(LanHeader));

		//터무니 없이 큰 len값은 연결을 끊는다.
		if (header.len >= 65535)
		{
			SystemLog::GetInstance()->WriteLog(L"ERROR", SystemLog::LOG_LEVEL_ERROR, L"header len is over65535");
			Disconnect(sessionID);
		}

		//헤더에 명시된 페이로드보다 읽을 데이터가 적으면 반복문 탈출
		if (header.len > pSession->_recvQ.GetUseSize() - sizeof(LanHeader))
		{
			break;
		}

		pSession->_recvQ.MoveFront(sizeof(LanHeader));

		//컨텐츠에 패킷을 전달해주기 위해 직렬화 버퍼 할당
		Message* pMessage = Message::Alloc();
		//레퍼런스 카운트 수동 증가
		pMessage->AddRefCount();
		//버퍼에 있는 값을 직렬화 버퍼에 넣기
		pSession->_recvQ.Dequeue(pMessage->GetPayloadPtr(), header.len);
		//pSession->RecvBufferPeek(pMessage->GetBufferPtr(), header.len);
		pMessage->MoveWritePos(header.len);

		auto opt_contentsID = pSession->_contentsID;
		if (opt_contentsID.has_value())
		{
			Contents* pContents = _contentsHashTable[opt_contentsID.value()];
			if (pContents->IsSerial())	//메시지 처리에 직렬화가 필요한 컨텐츠 (ex. 필드)
			{
				pContents->ELock();
				pContents->OnRecv(sessionID, pMessage);
				pContents->EUnlock();
			}
			else						//메시지 처리에 직렬화가 필요하지 않은 컨텐츠 (ex. 로비)
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

	// SendPacket_Disconnect 함수를 호출한 경우 다시 Recv를 걸지 않는다.
	if (pSession->GetAsyncRecvBlockFlag() == FALSE)
	{
		//다시 WSARecv 걸기
		bool retOfReqRecv = pSession->RequestRecv();
	}
}

void LanServer::SendCompleteProc(DWORD transferred, UINT64 sessionID, Session* pSession)
{
	int sentMessageCount = InterlockedExchange(&pSession->_sendingMessageCount, 0);
	pSession->SentMessageFree(sentMessageCount);

	pSession->CompleteSending();

	//모아서 보내지 않는 경우, 여기서 다시 보낸다
	//모아서 보내는 경우, 다음 간격에 보내 줄 것이므로 여기서 보내지 않는다.
	if (_sendIntervalTime == 0)
	{
			int retOfReqSend = pSession->RequestSend();
	}

	//OnSend(sessionID, transferred);
	InterlockedAdd64(&_TPS._sendTPS, sentMessageCount);
}

bool LanServer::SessionRelease(Session* pSession)
{
	//Release를 해도 되는지 체크
	if (!pSession->CheckCanRelease())
	{
		return false;
	}
	closesocket(pSession->_socket);
	pSession->_socket = INVALID_SOCKET;
	//직렬화 버퍼 풀에서 할당받은 직렬화 버퍼가 아직 제거되지 않은 경우 메모리 누수를 막기위해 여기서 풀에 반환해준다.
	int sentMessageCount = InterlockedExchange(&pSession->_sendingMessageCount, 0);

	pSession->SentMessageFree(sentMessageCount);

	//SendQ 비워주기
	while (true)
	{
		auto pMessageOpt = pSession->_sendQ.Dequeue();
		if (pMessageOpt.has_value())
			pMessageOpt.value()->SubRefCount();
		else
			break;
	}

	__int64 sessionID = pSession->_sessionID;
	//다른 워커 스레드가 OnRelease를 호출 하게 하기 위해 PQCS로 IOCP에 일감을 던져준다.
	//자기가 해버리면 데드락이 발생할 수 있음.
	PQCS(100, sessionID, reinterpret_cast<LPOVERLAPPED>(PQCS_SIGN::RELEASE));

	return true;
}

bool LanServer::IsSessionRelease(ULONG ioCount)
{
	return (ioCount & Session::RELEASE_BIT_MASK) == Session::RELEASE_BIT_MASK;
}

Session* LanServer::AcquireSessionLicense(UINT64 sessionID)
{
	//세션 검색
	Session* pSession = _sessionContainer.SearchSession(sessionID);
	if (pSession == nullptr)
	{
		return nullptr;
	}

	//SendPaqcket 루틴을 하기 전에 IOCount 증가시켜서 해당 세션이 삭제되지 않도록 막는다.
	ULONG ioCount = InterlockedIncrement(&pSession->_IOCount);

	//IOCount의 상위 1비트가 릴리즈 플래그이므로 이 비트를 검사해서 세션에 대한 작업을 진행할지를 결정한다.
	if (IsSessionRelease(ioCount))
	{
		ULONG ioCountForCheckingRelease = pSession->DecreaseIOCount();
		if (ioCountForCheckingRelease == 0)
		{
			SessionRelease(pSession);
		}
		return nullptr;
	}

	//검색해온 pSession이 함수를 들어온 시점의 세션이 맞는지 검사
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

	//기존에 이미 컨텐츠에 소속되어 있는 경우 그곳에서 빼준다.
	if (pSession->_contentsID.has_value())
	{
		Contents* pCurrentContents =_contentsHashTable[pSession->_contentsID.value()];
		pCurrentContents->ELock();
		//떠났음을 해당 컨텐츠로 알린다.
		if (!pCurrentContents->NotifyLeaveContents(sessionID))
		{
			pCurrentContents->EUnlock();
			ReleaseSessionLicense(pSession);
			return;
		}
		pCurrentContents->EUnlock();
	}

	//세션을 주어진 컨텐츠 아이디로 소속시킨다.
	pSession->EnterContents(destContentsID);

	pDestContents->ELock();
	//소속 시켰음을 해당 컨텐츠로 알린다.
	if (!pDestContents->NotifyEnterContents(sessionID))
	{
		pDestContents->EUnlock();
		ReleaseSessionLicense(pSession);
		return;
	}
	pDestContents->EUnlock();

	//컨텐츠 이동 간에 수신 완료 통지가 처리되면 메시지가 유실되기 때문에
	//컨텐츠 이동을 마친 후 여기서 비동기 Recv를 걸도록 한다.
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

	//SendQueue에 직렬화 버퍼의 포인터 넣기
	std::optional<LONG> optionalEnqueueRet = pSession->_sendQ.Enqueue(pMessage);

	if (!optionalEnqueueRet.has_value())
	{
		//SendQueue가 가득 찬 경우
		// 1. 상대가 수신을 안 해서 상대의 수신 버퍼가 가득 차버려서 송신 완료 통지가 안오면 가득 찰 수 있음 (비정상)
		// 2. 유저가 한 곳에 너무 몰린 상황처럼 보낼게 너무 몰리면 가득 찰 수가 있다.
		// 따라서 그런 경우 세션을 끊어서 해결한다.
		SystemLog::GetInstance()->WriteLog(
			L"[ERROR]", SystemLog::LOG_LEVEL_ERROR, L"[Error] sessionID: %lld / Full Of Send Queue\n",
			sessionID
		);
		Disconnect(sessionID);

		ReleaseSessionLicense(pSession);
		pMessage->SubRefCount();
		return false;
	}

	//WSASend 요청하기
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
		//여기서 보낸 WSASend의 완료 통지가 오면 IOCount가 0이 되도록 flag를 세워서 비동기 Recv를 걸지 못하게 한다.
		pSession->SetAsyncRecvBlockFlag();
		CancelIoEx((HANDLE)pSession->_socket, (LPOVERLAPPED)&pSession->_recvOverlapped);
	}

	return true;
}

void LanServer::RequestMoveContents(UINT64 sessionID, INT32 destContentsID)
{
	//여기서 올린 IOCount는 PQCS 완료 처리에서 내린다.
	Session* pSession = AcquireSessionLicense(sessionID);
	if (pSession == nullptr)
	{
		SystemLog::GetInstance()->WriteLog(L"ERROR", SystemLog::LOG_LEVEL_ERROR, L"pSession is nullptr in RequestMoveContents");
		return;
	}

	//세션이 이동 중일 때, 해당 세션의 비동기 Recv를 걸지 않도록 해서 메시지 처리를 하는 상황을 막는다.
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
		//			예외 처리
		//////////////////////////////////
		//스레드를 의도적으로 종료시키려 할 때(서버 종료 시) 0, 0, 0을 Completion Queue에 넣는다.
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

		// 세션 검색
		pSession = _sessionContainer.SearchSession(sessionID);
		if (pSession == nullptr)
		{
			SystemLog::GetInstance()->WriteLogConsole(SystemLog::LOG_LEVEL_ERROR,
				L"[ERROR] sessionID: %d / Session not found\n", sessionID);
			__debugbreak();
		}

		//상대가 fin을 정상적으로 보낸 경우
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
		//		IO 완료 처리
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
				SystemLog::GetInstance()->WriteLog(L"ERROR", SystemLog::LOG_LEVEL_ERROR, L"알 수 없는 operation 완료 통지");
				break;
			}
		}

		//////////////////////////////////////////
		//    IOCount 감소 및 세션 삭제처리    //
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
		//서버 종료를 위해 _listenSocket을 닫은 경우
		if (clientSock == INVALID_SOCKET)
		{
			break;
		}

		//MaxSession를 넘기면 연결을 받지 않는다.
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

		//소켓과 IOCP 연결
		if (pSession->ConnectSocketToIOCP(_hIOCP) == false)
		{
			SystemLog::GetInstance()->WriteLogConsole(SystemLog::LOG_LEVEL_ERROR,
				L"sessionID : %d ConnectSocketToIOCP() fail\n", pSession->_sessionID);
		}

		OnAccept(pSession->_sessionID);
		InterlockedIncrement64(&_TPS._acceptTPS);

		//디폴트 컨텐츠가 존재하면 세션을 해당 컨텐츠로 소속시킨다.
		if (_defaultContentID.has_value())
		{
			RequestMoveContents(pSession->_sessionID, _defaultContentID.value());
		}

		//비동기 리시브 걸어두기
		if (!pSession->GetAsyncRecvBlockFlag())
		{
			bool retOfReqRecv = pSession->RequestRecv();
		}
		pSession->DecreaseIOCount();
		//최초 Recv 실패하면 여기서 삭제해준다.
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
		//TPS 값들을 백업
		InterlockedExchange64(&_prevTPS._acceptTPS, _TPS._acceptTPS);
		InterlockedExchange64(&_prevTPS._recvTPS, _TPS._recvTPS);
		InterlockedExchange64(&_prevTPS._sendTPS, _TPS._sendTPS);

		//accpet는 total값 집계하기
		_totalAccept += _TPS._acceptTPS;

		//1초 지났으니 0으로 만들기
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
		// _sendIntervalTime을 지났는지 검사
		DWORD deltaTime = timeGetTime() - _sendTick;

		if (deltaTime < static_cast<DWORD>(_sendIntervalTime))
		{
			Sleep(_sendIntervalTime - deltaTime);
		}
		_sendTick += _sendIntervalTime;
		_sendThreadTPS++;

		//전체 세션 돌면서 모았던 SendPacket들을 Send 하기
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
	// 프레임에 한 번씩 PQCS 쏴주기
	// 요구 사항 : 스레드가 계속 돌면 안 됨. 딱 시그널을 줄 때 일어나서 시그널 주고 다시 자러가야함.
	// OnUpdate에 밀린 프레임이 있다면 얼마나 밀렸는지를 제공해줘야 함.

	const int ONE_SECOND_TASK_ID = -1;
	auto now = std::chrono::steady_clock::now();
	//1초에 한 번씩 OnTime 호출되도록 따로 넣는다.
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
			// 작업 실행
			AcquireSRWLockExclusive(&_frameTimePriorityQueueLock);
			_frameTimePriorityQueue.pop();

			if (nextTask._contentID == ONE_SECOND_TASK_ID)
			{
				PQCS(NULL, NULL, reinterpret_cast<LPOVERLAPPED>(PQCS_SIGN::TIMER));
			}
			else
			{
				//IOCP에 CompletionStatus를 쏴준다.
				PQCS(static_cast<DWORD>(nextTask._contentID), 
					std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count(),
					reinterpret_cast<LPOVERLAPPED>(PQCS_SIGN::FRAME_LOGIC));
			}

			// 다음 실행 시각 계산 후 다시 큐에 삽입
			FrameTask newTask((now + std::chrono::milliseconds(nextTask._intervalMs)), nextTask._contentID, nextTask._intervalMs);
			_frameTimePriorityQueue.push(newTask);
			ReleaseSRWLockExclusive(&_frameTimePriorityQueueLock);
		}
		else 
		{
			// 다음 작업까지 대기
			INT64 sleepDuration = std::chrono::duration_cast<std::chrono::milliseconds>(nextTask._executionTime - now).count();
			std::this_thread::sleep_for(std::chrono::milliseconds(sleepDuration));
		}
	}
}

void LanServer::PQCS(DWORD transferred, ULONG_PTR completionKey, LPOVERLAPPED overlapped)
{
	PostQueuedCompletionStatus(_hIOCP, transferred, completionKey, overlapped);
}

#pragma comment(lib, "ws2_32")
#pragma comment(lib, "Winmm.lib")
#include "NetServer.h"
#include <WS2tcpip.h>
#include <synchapi.h>
#include "PacketProtocol.h"
#include "SystemLog.h"
#include "Message.h"
#include "Session.h"
#include "Parser.h"

MemoryLog mLog;

NetServer::NetServer()
	:_totalAccept(0), _hAcceptThread(INVALID_HANDLE_VALUE), _hDebugInfoThread(INVALID_HANDLE_VALUE), _hSendPostThread(INVALID_HANDLE_VALUE),
	_isAcceptThreadRun(true), _isWorkerThreadRun(true), _isDebugInfoThreadRun(true), _isSendPostThreadRun(true),
	_numOfMaxSession(0), _numOfMaxUser(0), _nagleOption(false), _IOCP_WorkerThreadNum(0), _IOCP_ConcurrentThreadNum(0),
	_packetCode(0), _encodeStaticKey(0), _timeoutDisconnectForSession(0), _timeoutDisconnectForUser(0), _opt_defaultContentsID(std::nullopt)
{
}

NetServer::~NetServer()
{
}

bool NetServer::Start(const WCHAR* configFileName)
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
	parser.GetValue(L"PACKET_CODE", &_packetCode);
	parser.GetValue(L"PACKET_KEY", &_encodeStaticKey);
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

void NetServer::Stop()
{
	_isAcceptThreadRun = false;
	_isDebugInfoThreadRun = false;
	_isSendPostThreadRun = false;
		
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

void NetServer::Disconnect(UINT64 sessionID)
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

char* NetServer::GetHeaderPtr(Message* pMessage)
{
	return pMessage->GetHeaderPtr();
}

bool NetServer::AddContents(INT32 key, const Contents* pContents)
{
	if (pContents == nullptr)
		return false;

	//컨텐츠 자료구조에 삽입한다. 중복 삽입이라면 false 리턴
	auto result = _contentsHashTable.insert({ key, const_cast<Contents*>(pContents) });
	if (result.second == false)
		return false;

	return true;
}

void NetServer::SetDefaultContents(INT32 contentsID)
{
	_opt_defaultContentsID = contentsID;
}

void NetServer::RecvCompleteProc(DWORD transferred, UINT64 sessionID, Session* pSession)
{
	//recvQ 정리
	int moveSize = pSession->_recvQ.MoveRear(transferred);

	//패킷을 프로토콜대로 해석해서 메시지 단위로 OnRecv로 컨텐츠에게 메시지 리시브를 알린다.
	while (1)
	{
		//헤더만 있거나 헤더도 없다면 반복문 탈출
		if (pSession->_recvQ.GetUseSize() <= sizeof(NetHeader))
			break;

		NetHeader header;
		pSession->_recvQ.Peek((char*)&header, sizeof(NetHeader));

		//헤더의 코드 검사해서 어처구니 없는 값이면 연결 끊기
		if (header.code != _packetCode)
		{
			pSession->InvalidateSession();
			break;
		}
		//터무니 없이 큰 len값은 연결을 끊는다.
		if (header.len >= 65535)
		{
			SystemLog::GetInstance()->WriteLog(L"ERROR", SystemLog::LOG_LEVEL_ERROR, L"header len is over 65535");
			Disconnect(sessionID);
		}

		//헤더에 명시된 페이로드보다 읽을 데이터가 적으면 반복문 탈출
		if (header.len > pSession->_recvQ.GetUseSize() - sizeof(NetHeader))
		{
			break;
		}

		pSession->_recvQ.MoveFront(sizeof(NetHeader));

		//컨텐츠에 패킷을 전달해주기 위해 직렬화 버퍼 할당
		Message* pMessage = Message::Alloc();
		//레퍼런스 카운트 수동 증가
		pMessage->AddRefCount();
		//인코딩 되어있음을 명시함.
		pMessage->SetIsEncode(true);
		//버퍼에 있는 값을 직렬화 버퍼에 넣기
		int retDeq = pSession->_recvQ.Dequeue(pMessage->GetPayloadPtr(), header.len);
		if (retDeq == 0)
		{
			SystemLog::GetInstance()->WriteLog(L"ERROR", SystemLog::LOG_LEVEL_ERROR, L"RecvRingBuffer Dequeue Fail : sessionID : %lld", sessionID);
			pSession->InvalidateSession();
			CancelIoEx((HANDLE)pSession->_socket, NULL);
			pMessage->SubRefCount();
			break;
		}
		pMessage->MoveWritePos(header.len);
		//체크섬도 디코딩의 대상이므로 직렬화 버퍼에 헤더부분에 직접 넣는다.
		NetHeader* pMessageHeader = reinterpret_cast<NetHeader*>(GetHeaderPtr(pMessage));
		pMessageHeader->checkSum = header.checkSum;

		//메시지 복호화
		bool decodeOK = pMessage->Decode(header.randKey, _encodeStaticKey);
		if (!decodeOK)
		{
			//디코드 결과 체크섬 값과 일치하지 않으면 조작된 메시지로 간주하고 연결을 끊는다.
			SystemLog::GetInstance()->WriteLog(L"ERROR", SystemLog::LOG_LEVEL_ERROR, L"Decode Fail : sessionID : %lld", sessionID);
			pSession->InvalidateSession();
			CancelIoEx((HANDLE)pSession->_socket, NULL);
			pMessage->SubRefCount();
			break;
		}

		LONG contentsID = pSession->_contentsID;
		if (contentsID != Session::NON_CONTENTS)
		{
			pSession->_recvMessageQueue.Enqueue(pMessage);
			//WaitOnAddress로 자고 있는 컨텐츠 스레드 깨우기
			AwakeContentsThread(contentsID);
		}
		else
		{
			OnRecv(sessionID, pMessage);
		}

		InterlockedIncrement64(&_TPS._recvTPS);

		if (!(pSession->_isValid))
		{
			break;
		}
	}

	// SendPacket_Disconnect 함수를 호출한 경우 다시 Recv를 걸지 않는다.
	if (pSession->GetAsyncRecvBlockFlag() == FALSE)
	{
		pSession->RequestRecv();
	}
}

void NetServer::SendCompleteProc(DWORD transferred, UINT64 sessionID, Session* pSession)
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

bool NetServer::SessionRelease(Session* pSession)
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
		auto opt_pMessage = pSession->_sendQ.Dequeue();
		if (opt_pMessage.has_value())
			opt_pMessage.value()->SubRefCount();
		else
			break;
	}

	//recvMessageQueue 비워주기
	while (true)
	{
		auto opt_pMessage = pSession->_recvMessageQueue.Dequeue();
		if (opt_pMessage.has_value())
			opt_pMessage.value()->SubRefCount();
		else
			break;
	}

	__int64 sessionID = pSession->_sessionID;

	//다른 워커 스레드가 OnRelease를 호출 하게 하기 위해 PQCS로 IOCP에 일감을 던져준다.
	//자기가 해버리면 데드락이 발생할 수 있음.
	PQCS(100, sessionID, reinterpret_cast<LPOVERLAPPED>(PQCS_SIGN::RELEASE));

	return true;
}

bool NetServer::IsSessionRelease(ULONG ioCount)
{
	return (ioCount & Session::RELEASE_BIT_MASK) == Session::RELEASE_BIT_MASK;
}

void NetServer::AwakeContentsThread(LONG contentsID)
{
	LONG ret = InterlockedIncrement(&_contentsHashTable[contentsID]->_eventCounter);
	//이벤트가 0 -> 1로 변했다면 스레드를 깨운다.
	if (ret == 1)
	{
		WakeByAddressSingle(&_contentsHashTable[contentsID]->_eventCounter);
	}
}

void NetServer::PQCSSendPost(INT64 sessionID)
{
	if (!PostQueuedCompletionStatus(_hIOCP, INT32_MAX, sessionID, reinterpret_cast<LPOVERLAPPED>(PQCS_SIGN::SEND_PACKET)))
	{
		SystemLog::GetInstance()->WriteLog(
			L"[ERROR]", SystemLog::LOG_LEVEL_ERROR, L"[Error] sessionID: %lld / PQCS Failed\n",
			sessionID);
	}
}

Session* NetServer::AcquireSessionLicense(UINT64 sessionID)
{
	//세션 검색
	Session* pSession = _sessionContainer.SearchSession(sessionID);
	if (pSession == nullptr)
	{
		return nullptr;
	}

	//IOCount 증가시켜서 해당 세션이 삭제되지 않도록 막는다.
	ULONG ioCount = InterlockedIncrement(&pSession->_IOCount);

	//이미 Release 처리가 되기로 결정이 났다면
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

void NetServer::ReleaseSessionLicense(Session* pSession)
{
	ULONG ioCountForCheckingRelease = pSession->DecreaseIOCount();
	if (ioCountForCheckingRelease == 0)
	{
		SessionRelease(pSession);
	}
}

void NetServer::HandleSendPakcetPQCS(INT64 sessionID)
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

void NetServer::HandleReleasePQCS(INT64 sessionID)
{
	Session* pSession = _sessionContainer.SearchSession(sessionID);
	if (pSession == nullptr)
	{
		SystemLog::GetInstance()->WriteLogConsole(SystemLog::LOG_LEVEL_ERROR,
			L"[ERROR] sessionID: %d / Session not found\n", sessionID);
		__debugbreak();
	}

	LONG contentsID = pSession->_contentsID;

	//Session Container에서 해당 세션을 제거한다.
	_sessionContainer.DeleteSession(sessionID);

	if (pSession->_contentsID != Session::NON_CONTENTS)
	{
		//해당 컨텐츠에게 세션이 끊겼음을 메시지 큐에 삽입해서 전파한다.
		Contents* contents = _contentsHashTable[contentsID];
		contents->NotifyDisconnectSession(sessionID);
	}
	else
	{
		OnRelease(sessionID);
	}
}


bool NetServer::SendPacket(UINT64 sessionID, Message* pMessage, bool usePQCS, bool disconnect, bool onlyEnqueue)
{
	pMessage->AddRefCount();

	Session* pSession = AcquireSessionLicense(sessionID);
	if (pSession == nullptr)
	{
		pMessage->SubRefCount();
		return false;
	}

	// 패킷 인코딩 
	// (하나의 직렬화 버퍼에 여러 스레드가 Send를 하는 경우는 현재 필요하지 않으므로 고려하지 않는다. 추후 그러한 기능이 필요한 경우 구현)
	if (!pMessage->GetIsEncode())
	{
		pMessage->WriteHeader<Net>(_packetCode);
		pMessage->Encode(_encodeStaticKey);
	}

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
		wprintf(L"send Queue Full\n");
		Disconnect(sessionID);

		ReleaseSessionLicense(pSession);
		pMessage->SubRefCount();
		return false;
	}

	//WSASend 요청하기
	if (_sendIntervalTime == 0 && !onlyEnqueue)
	{
		if (usePQCS)
		{
			int transferred = pMessage->GetDataSize() + pMessage->GetHeaderSize<Net>();
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
		DWORD transferred = INT32_MAX;
		unsigned __int64 sessionID = INT64_MAX;
		OverlappedEx* pOverlapped = nullptr;
		Session* pSession = nullptr;

		retOfGQCS = GetQueuedCompletionStatus(_hIOCP, &transferred, &sessionID, (LPOVERLAPPED*)&pOverlapped, INFINITE);

		//스레드를 의도적으로 종료시키려 할 때(서버 종료 시) 0, 0, 0을 Completion Queue에 넣는다.
		if (transferred == 0 && sessionID == 0 && pOverlapped == nullptr)
		{
			break;
		}

		switch (reinterpret_cast<int>(pOverlapped))
		{
		case PQCS_SIGN::SEND_PACKET:
			HandleSendPakcetPQCS(sessionID);
			continue;
		case PQCS_SIGN::RELEASE:
			HandleReleasePQCS(sessionID);
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

void NetServer::AcceptThread()
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

		//ip, port 정보 기록
		WCHAR clientIPStr[16] = { 0, };
		InetNtop(AF_INET, &clientAddr.sin_addr, clientIPStr, 16);
		int clientPort = ntohs(clientAddr.sin_port);

		if (OnConnectionRequest(clientIPStr, clientPort) == false)
		{
			closesocket(clientSock);
			continue;
		}

		//세션 재활용
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
		if (_opt_defaultContentsID.has_value())
		{
			pSession->EnterContents(_opt_defaultContentsID.value());
			//컨텐츠 스레드에게 접속함을 통지
			_contentsHashTable[_opt_defaultContentsID.value()]->NotifyEnterContents(pSession->_sessionID);
		}

		//WSARecv 걸어두기
		if (!pSession->GetAsyncRecvBlockFlag())
		{
			bool retOfReqRecv = pSession->RequestRecv();
		}

		//재활용할 때 삭제 방지를 위해 증가 시켜둔 IOCount를 여기서 감소시킨다.
		pSession->DecreaseIOCount();
		//최초 Recv 실패하면 여기서 삭제해준다.
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

		OnTime();
	}

	return;
}

void NetServer::SendPostThread()
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

void NetServer::PQCS(DWORD transferred, ULONG_PTR completionKey, LPOVERLAPPED overlapped)
{
	PostQueuedCompletionStatus(_hIOCP, transferred, completionKey, overlapped);
}

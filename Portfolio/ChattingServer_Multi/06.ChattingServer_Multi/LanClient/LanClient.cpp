#include "pch.h"
#include "LanClient.h"
#include "ClientSession.h"
#include <tchar.h>
#include "SystemLog.h"
#include "Message.h"

LanClient::LanClient()
{
}

LanClient::~LanClient()
{
}

bool LanClient::InitializeNetwork(const WCHAR* ip, short port, int numOfWorkerThread, int numOfConcurrentThread, bool nagle)
{
	SystemLog* logger = SystemLog::GetInstance();

	wcscpy_s(_IP, 16, ip);
	_port = port;
	_nagle = nagle;
	_numOfConcurrentThread = numOfConcurrentThread;
	_numOfWorkerThread = numOfWorkerThread;


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

	//클라 소켓 생성
	SOCKET _socket = socket(AF_INET, SOCK_STREAM, 0);
	if (_socket == INVALID_SOCKET)
	{
		logger->WriteLogConsole(SystemLog::LOG_LEVEL_ERROR, L"[에러] socket() 함수 실패 : %d\n", GetLastError());
		return false;
	}

	//클라 세션 생성
	_session = new ClientSession(_socket);

	//RST로 강제종료 할 수 있도록 링거 옵션을 설정한다.
	linger linger;
	linger.l_linger = 0;
	linger.l_onoff = true;
	setsockopt(_session->_socket, SOL_SOCKET, SO_LINGER, (char*)&linger, sizeof(linger));

	//비동기 IO로 Send를 수행하도록 L4의 Send버퍼를 0으로 만든다.
	int sendBufSize = 0;
	setsockopt(_session->_socket, SOL_SOCKET, SO_SNDBUF, (const char*)&sendBufSize, sizeof(sendBufSize));

	//인자로 받은 nagle 옵션을 설정한다.
	bool nagleOption = !_nagle;	//네이글을 끄는게 TRUE 이므로 의도에 맞게 변환.
	setsockopt(_session->_socket, IPPROTO_TCP, TCP_NODELAY, (char*)&nagleOption, sizeof(nagleOption));

	//주소 구조체 세팅
	SOCKADDR_IN serverAddr;
	memset(&serverAddr, 0, sizeof(SOCKADDR_IN));
	serverAddr.sin_family = AF_INET;
	InetPton(AF_INET, _IP, &serverAddr.sin_addr);
	serverAddr.sin_port = htons(_port);

	//connect 보내기
	int connectRet = connect(_session->_socket, (SOCKADDR*)&serverAddr, sizeof(serverAddr));
	if (connectRet == SOCKET_ERROR)
	{
		logger->WriteLogConsole(SystemLog::LOG_LEVEL_ERROR, L"[에러] connect() 함수 실패 : %d\n", GetLastError());
		return false;
	}


	//IOCP 생성
	_hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, _numOfConcurrentThread);
	if (_hIOCP == NULL)
	{
		logger->WriteLogConsole(SystemLog::LOG_LEVEL_ERROR, L"[Error] CreateIoCompletionPort() 생성 실패 : %d\n", GetLastError());
		return false;
	}

	//클라 소켓을 IOCP에 등록
	CreateIoCompletionPort((HANDLE)_session->_socket, _hIOCP, (ULONG_PTR)nullptr, 0);

	//네트워크 스레드 생성
	for (int i = 0; i < _numOfWorkerThread; i++)
	{
		HANDLE hWorkerThread = (HANDLE)_beginthreadex(NULL, 0, NetworkThread, this, 0, nullptr);
		if (hWorkerThread == 0)
		{
			logger->WriteLogConsole(SystemLog::LOG_LEVEL_ERROR, L"[Error] _beginthreadex() 실패 : %d\n", GetLastError());
			return false;
		}
		_hWorkerThreads.push_back(hWorkerThread);
	}

	//바로 비동기 Recv를 걸어둔다.
	RequestRecv();

	OnInit();

	return true;
}

void LanClient::TerminateNetwork()
{

	//워커 스레드 종료 시그널 보내기
	for (int i = 0; i < _numOfWorkerThread; i++)
	{
		PostQueuedCompletionStatus(_hIOCP, 0, 0, 0);
	}
	//종료 될 때까지 대기
	WaitForMultipleObjects(_numOfWorkerThread, _hWorkerThreads.data(), TRUE, INFINITE);

	//핸들 정리
	for (int i = 0; i < _numOfWorkerThread; i++)
	{
		CloseHandle(_hWorkerThreads[i]);
	}
	CloseHandle(_hIOCP);

	if (_session->_connect == true)
	{
		HandleRelease();
	}

	WSACleanup();

	OnTerminate();
}

bool LanClient::Disconnect()
{
	CancelIoEx((HANDLE)_session->_socket, NULL);
	_session->_connect = false;

	return false;
}

bool LanClient::SendPacket(Message* pMessage)
{
	pMessage->AddRefCount();

	//연결 체크
	if (_session->_connect == false)
	{
		pMessage->SubRefCount();
		return false;
	}

	//메시지 헤더 세팅
	pMessage->WriteHeader<Lan>(NULL);

	//메시지 송신 큐에 인큐
	auto ret = _session->_sendQ.Enqueue(pMessage);
	if (!ret.has_value())
	{
		pMessage->SubRefCount();
		Disconnect();
		return false;
	}

	//송신 요청
	RequestSend();
	return true;
}

unsigned int __stdcall LanClient::NetworkThread(LPVOID pParam)
{
	LanClient* pLanClient = reinterpret_cast<LanClient*>(pParam);

	while (true)
	{
		DWORD transferred = 0;
		UINT64 completionKey = 0;
		OverlappedEx* pOverlapped = nullptr;

		int GQCSRet = GetQueuedCompletionStatus(pLanClient->_hIOCP, &transferred, &completionKey, (LPOVERLAPPED*)&pOverlapped, INFINITE);


		//스레드를 의도적으로 종료시키려 할 때(서버 종료 시) 0, 0, 0을 Completion Queue에 넣는다.
		if (transferred == 0 && completionKey == 0 && pOverlapped == nullptr)
			break;
		if (reinterpret_cast<int>(pOverlapped) == RELEASE_SIGN_PQCS)
		{
			pLanClient->HandleRelease();
			continue;
		}

		if (transferred == 0 && pOverlapped != nullptr)
		{
			DWORD errcode = GetLastError();
			if (pOverlapped->_operation == IOOperation::SEND)
				SystemLog::GetInstance()->WriteLog(L"ERROR", SystemLog::LOG_LEVEL_ERROR, L"0 바이트 송신, errcode: %d", errcode);
		}

		if (pOverlapped->_operation == IOOperation::SEND)
		{
			pLanClient->HandleSendComplete(transferred);
		}
		else
		{
			pLanClient->HandleRecvComplete(transferred);
		}

		pLanClient->DecreseIOCount();
	}

	SystemLog::GetInstance()->WriteLogConsole(SystemLog::LOG_LEVEL_ERROR, L"NetworkThread 종료 / threadID : %d", GetCurrentThreadId());

	return 0;
}

void LanClient::HandleRelease()
{
	closesocket(_session->_socket);
	delete _session;

	OnDisconnectServer();
}

bool LanClient::HandleSendComplete(DWORD sendBytes)
{
	//SendingMessage 정리
	for (int i = 0; i < _session->_sendingMessageCount; i++)
	{
		_session->_sendingMessageArray[i]->SubRefCount();
	}

	_session->_sendingMessageCount = 0;

	OnSend(sendBytes);
	InterlockedExchange(&_session->_isSending, 0);

	//보낼 것이 있을 수 있으니 다시 송신 요청해본다.
	RequestSend();

	return true;
}

bool LanClient::HandleRecvComplete(DWORD recvBytes)
{
	//recvQ 정리
	int moveSize = _session->_recvQ.MoveRear(recvBytes);

	//패킷을 프로토콜대로 해석해서 메시지 단위로 OnRecv로 컨텐츠에게 메시지 리시브를 알린다.
	while (1)
	{
		//헤더만 있거나 헤더도 없다면 반복문 탈출
		if (_session->_recvQ.GetUseSize() <= sizeof(LanHeader))
			break;

		LanHeader header;
		_session->_recvQ.Peek((char*)&header, sizeof(LanHeader));

		//터무니 없이 큰 len값은 연결을 끊는다.
		if (header.len >= 65535)
		{
			SystemLog::GetInstance()->WriteLog(L"ERROR", SystemLog::LOG_LEVEL_ERROR, L"header len is over65535");
			Disconnect();
			break;
		}

		//헤더에 명시된 페이로드보다 읽을 데이터가 적으면 반복문 탈출
		if (header.len > _session->_recvQ.GetUseSize() - sizeof(LanHeader))
			break;

		_session->_recvQ.MoveFront(sizeof(LanHeader));

		//컨텐츠에 패킷을 전달해주기 위해 직렬화 버퍼 할당
		Message* pMessage = Message::Alloc();
		//레퍼런스 카운트 수동 증가
		pMessage->AddRefCount();
		//버퍼에 있는 값을 직렬화 버퍼에 넣기
		_session->_recvQ.Dequeue(pMessage->GetPayloadPtr(), header.len);
		pMessage->MoveWritePos(header.len);

		OnRecv(pMessage);

		if (_session->_connect == false)
			break;
	}
	//다시 WSARecv 걸기
	RequestRecv();
	return true;
}

bool LanClient::RequestRecv()
{
	DWORD flag = 0;
	DWORD recvNumBytes = 0;
	int retOfWSARecv;

	if (_session->_connect == false)
		return false;

	memset(&_session->_recvOverlapped, 0, sizeof(OVERLAPPED));
	//Overlapped I/O를 위해 각 정보를 셋팅해 준다.
	WSABUF wsaBuf[2] = { 0, };
	int freeSize = _session->_recvQ.GetFreeSize();
	int dirEnqueSize = _session->_recvQ.DirectEnqueueSize();
	int spareSize = freeSize - dirEnqueSize;

	int numOfUsedWSABuf = 2;
	wsaBuf[0].buf = _session->_recvQ.GetRearBufferPtr();
	wsaBuf[0].len = dirEnqueSize;
	wsaBuf[1].buf = _session->_recvQ.GetBeginBufferPtr();
	wsaBuf[1].len = spareSize;

	InterlockedIncrement(&_session->_IOCount);
	retOfWSARecv = WSARecv(_session->_socket, wsaBuf, numOfUsedWSABuf, &recvNumBytes, &flag, (LPWSAOVERLAPPED) & (_session->_recvOverlapped), NULL);

	//socket_error이면 client socket이 끊어진 것으로 처리한다.
	if (retOfWSARecv == SOCKET_ERROR)
	{
		int errCode = GetLastError();
		if (errCode == WSA_IO_PENDING)
		{
			//비동기 IO
		}
		else
		{
			InterlockedDecrement(&_session->_IOCount);
			if (errCode != 10054)
			{
				SystemLog::GetInstance()->WriteLogConsole(SystemLog::LOG_LEVEL_ERROR, L"[Error] WSARecv() 함수 실패 : % d\n", errCode);
			}
			return false;
		}
	}
	return true;
}

bool LanClient::RequestSend()
{
	DWORD flag = 0;
	int retOfWSASend;
	int sendQSize = 0;
	SystemLog* log = SystemLog::GetInstance();

	while (true)
	{
		if (_session->_connect == FALSE)
			return false;
		//사이즈 체크를 먼저 한다.
		if (_session->_sendQ.GetSize() <= 0)
			return false;

		if (InterlockedExchange(&_session->_isSending, TRUE) == TRUE)
			return true;

		sendQSize = _session->_sendQ.GetSize();
		if (sendQSize <= 0)
			InterlockedExchange(&_session->_isSending, FALSE);
		else
			break;
	}

	//WSABUF 세팅
	WSABUF wsaBufs[ClientSession::MAX_SEND_BUF_ONCE] = { 0, };

	ULONG numOfSendMessage = 0;
	for (int i = 0; i < _countof(wsaBufs); i++)
	{
		auto retOptional = _session->_sendQ.Dequeue();
		if (!retOptional.has_value())
			break;
		Message* pMessage = retOptional.value();
		wsaBufs[i].buf = GetMessageHeaderPtr(pMessage);
		wsaBufs[i].len = pMessage->GetDataSize() + pMessage->GetHeaderSize<Lan>();
		_session->_sendingMessageArray[i] = pMessage;
		numOfSendMessage++;
	}

	memset(&_session->_sendOverlapped, 0, sizeof(OVERLAPPED));

	ULONG ioCnt = InterlockedIncrement(&_session->_IOCount);
	_session->_sendingMessageCount= numOfSendMessage;

	retOfWSASend = WSASend(_session->_socket, wsaBufs, numOfSendMessage, NULL, flag, (LPWSAOVERLAPPED)&_session->_sendOverlapped, NULL);
	if (retOfWSASend == SOCKET_ERROR)
	{
		int errCode = GetLastError();
		if (errCode == WSA_IO_PENDING)
		{
			//g_sendIoPendingCnt++;
		}
		else
		{
			InterlockedDecrement(&_session->_IOCount);
			InterlockedExchange(&_session->_isSending, FALSE);
			if (errCode != 10054)
			{
				log->WriteLogConsole(SystemLog::LOG_LEVEL_ERROR, L"[Error]WSASend() 함수 실패 : % d\n", errCode);
			}
			return false;
		}
	}
	return true;
}

void LanClient::IncreseIOCount()
{
	InterlockedIncrement(&_session->_IOCount);
}

void LanClient::DecreseIOCount()
{
	LONG ret = InterlockedDecrement(&_session->_IOCount);
	if (ret == 0)
	{
		//IOCount가 0이면 끊어야 하므로 PQCS를 쏴준다.
		PostQueuedCompletionStatus(_hIOCP, INT32_MAX, 0, reinterpret_cast<LPOVERLAPPED>(RELEASE_SIGN_PQCS));
	}
}

char* LanClient::GetMessageHeaderPtr(Message* pMessage)
{
	//Lan 메시지는 헤더 크기가 2바이트이기 때문에 뒤로 2칸 가면 헤더의 포인터이다.
	return pMessage->GetPayloadPtr() - sizeof(LanHeader);
}

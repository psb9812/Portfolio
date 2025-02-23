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
	//		��Ʈ��ũ ���� ����
	///////////////////////////////////////////////////
	WSADATA wsa;
	int ret = WSAStartup(MAKEWORD(2, 2), &wsa);
	if (ret != 0)
	{
		logger->WriteLogConsole(SystemLog::LOG_LEVEL_ERROR, L"[����] WSAStartup()�Լ� ���� : %d\n", GetLastError());
		return false;
	}

	//Ŭ�� ���� ����
	SOCKET _socket = socket(AF_INET, SOCK_STREAM, 0);
	if (_socket == INVALID_SOCKET)
	{
		logger->WriteLogConsole(SystemLog::LOG_LEVEL_ERROR, L"[����] socket() �Լ� ���� : %d\n", GetLastError());
		return false;
	}

	//Ŭ�� ���� ����
	_session = new ClientSession(_socket);

	//RST�� �������� �� �� �ֵ��� ���� �ɼ��� �����Ѵ�.
	linger linger;
	linger.l_linger = 0;
	linger.l_onoff = true;
	setsockopt(_session->_socket, SOL_SOCKET, SO_LINGER, (char*)&linger, sizeof(linger));

	//�񵿱� IO�� Send�� �����ϵ��� L4�� Send���۸� 0���� �����.
	int sendBufSize = 0;
	setsockopt(_session->_socket, SOL_SOCKET, SO_SNDBUF, (const char*)&sendBufSize, sizeof(sendBufSize));

	//���ڷ� ���� nagle �ɼ��� �����Ѵ�.
	bool nagleOption = !_nagle;	//���̱��� ���°� TRUE �̹Ƿ� �ǵ��� �°� ��ȯ.
	setsockopt(_session->_socket, IPPROTO_TCP, TCP_NODELAY, (char*)&nagleOption, sizeof(nagleOption));

	//�ּ� ����ü ����
	SOCKADDR_IN serverAddr;
	memset(&serverAddr, 0, sizeof(SOCKADDR_IN));
	serverAddr.sin_family = AF_INET;
	InetPton(AF_INET, _IP, &serverAddr.sin_addr);
	serverAddr.sin_port = htons(_port);

	//connect ������
	int connectRet = connect(_session->_socket, (SOCKADDR*)&serverAddr, sizeof(serverAddr));
	if (connectRet == SOCKET_ERROR)
	{
		logger->WriteLogConsole(SystemLog::LOG_LEVEL_ERROR, L"[����] connect() �Լ� ���� : %d\n", GetLastError());
		return false;
	}


	//IOCP ����
	_hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, _numOfConcurrentThread);
	if (_hIOCP == NULL)
	{
		logger->WriteLogConsole(SystemLog::LOG_LEVEL_ERROR, L"[Error] CreateIoCompletionPort() ���� ���� : %d\n", GetLastError());
		return false;
	}

	//Ŭ�� ������ IOCP�� ���
	CreateIoCompletionPort((HANDLE)_session->_socket, _hIOCP, (ULONG_PTR)nullptr, 0);

	//��Ʈ��ũ ������ ����
	for (int i = 0; i < _numOfWorkerThread; i++)
	{
		HANDLE hWorkerThread = (HANDLE)_beginthreadex(NULL, 0, NetworkThread, this, 0, nullptr);
		if (hWorkerThread == 0)
		{
			logger->WriteLogConsole(SystemLog::LOG_LEVEL_ERROR, L"[Error] _beginthreadex() ���� : %d\n", GetLastError());
			return false;
		}
		_hWorkerThreads.push_back(hWorkerThread);
	}

	//�ٷ� �񵿱� Recv�� �ɾ�д�.
	RequestRecv();

	OnInit();

	return true;
}

void LanClient::TerminateNetwork()
{

	//��Ŀ ������ ���� �ñ׳� ������
	for (int i = 0; i < _numOfWorkerThread; i++)
	{
		PostQueuedCompletionStatus(_hIOCP, 0, 0, 0);
	}
	//���� �� ������ ���
	WaitForMultipleObjects(_numOfWorkerThread, _hWorkerThreads.data(), TRUE, INFINITE);

	//�ڵ� ����
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

	//���� üũ
	if (_session->_connect == false)
	{
		pMessage->SubRefCount();
		return false;
	}

	//�޽��� ��� ����
	pMessage->WriteHeader<Lan>(NULL);

	//�޽��� �۽� ť�� ��ť
	auto ret = _session->_sendQ.Enqueue(pMessage);
	if (!ret.has_value())
	{
		pMessage->SubRefCount();
		Disconnect();
		return false;
	}

	//�۽� ��û
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


		//�����带 �ǵ������� �����Ű�� �� ��(���� ���� ��) 0, 0, 0�� Completion Queue�� �ִ´�.
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
				SystemLog::GetInstance()->WriteLog(L"ERROR", SystemLog::LOG_LEVEL_ERROR, L"0 ����Ʈ �۽�, errcode: %d", errcode);
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

	SystemLog::GetInstance()->WriteLogConsole(SystemLog::LOG_LEVEL_ERROR, L"NetworkThread ���� / threadID : %d", GetCurrentThreadId());

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
	//SendingMessage ����
	for (int i = 0; i < _session->_sendingMessageCount; i++)
	{
		_session->_sendingMessageArray[i]->SubRefCount();
	}

	_session->_sendingMessageCount = 0;

	OnSend(sendBytes);
	InterlockedExchange(&_session->_isSending, 0);

	//���� ���� ���� �� ������ �ٽ� �۽� ��û�غ���.
	RequestSend();

	return true;
}

bool LanClient::HandleRecvComplete(DWORD recvBytes)
{
	//recvQ ����
	int moveSize = _session->_recvQ.MoveRear(recvBytes);

	//��Ŷ�� �������ݴ�� �ؼ��ؼ� �޽��� ������ OnRecv�� ���������� �޽��� ���ú긦 �˸���.
	while (1)
	{
		//����� �ְų� ����� ���ٸ� �ݺ��� Ż��
		if (_session->_recvQ.GetUseSize() <= sizeof(LanHeader))
			break;

		LanHeader header;
		_session->_recvQ.Peek((char*)&header, sizeof(LanHeader));

		//�͹��� ���� ū len���� ������ ���´�.
		if (header.len >= 65535)
		{
			SystemLog::GetInstance()->WriteLog(L"ERROR", SystemLog::LOG_LEVEL_ERROR, L"header len is over65535");
			Disconnect();
			break;
		}

		//����� ��õ� ���̷ε庸�� ���� �����Ͱ� ������ �ݺ��� Ż��
		if (header.len > _session->_recvQ.GetUseSize() - sizeof(LanHeader))
			break;

		_session->_recvQ.MoveFront(sizeof(LanHeader));

		//�������� ��Ŷ�� �������ֱ� ���� ����ȭ ���� �Ҵ�
		Message* pMessage = Message::Alloc();
		//���۷��� ī��Ʈ ���� ����
		pMessage->AddRefCount();
		//���ۿ� �ִ� ���� ����ȭ ���ۿ� �ֱ�
		_session->_recvQ.Dequeue(pMessage->GetPayloadPtr(), header.len);
		pMessage->MoveWritePos(header.len);

		OnRecv(pMessage);

		if (_session->_connect == false)
			break;
	}
	//�ٽ� WSARecv �ɱ�
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
	//Overlapped I/O�� ���� �� ������ ������ �ش�.
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

	//socket_error�̸� client socket�� ������ ������ ó���Ѵ�.
	if (retOfWSARecv == SOCKET_ERROR)
	{
		int errCode = GetLastError();
		if (errCode == WSA_IO_PENDING)
		{
			//�񵿱� IO
		}
		else
		{
			InterlockedDecrement(&_session->_IOCount);
			if (errCode != 10054)
			{
				SystemLog::GetInstance()->WriteLogConsole(SystemLog::LOG_LEVEL_ERROR, L"[Error] WSARecv() �Լ� ���� : % d\n", errCode);
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
		//������ üũ�� ���� �Ѵ�.
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

	//WSABUF ����
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
				log->WriteLogConsole(SystemLog::LOG_LEVEL_ERROR, L"[Error]WSASend() �Լ� ���� : % d\n", errCode);
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
		//IOCount�� 0�̸� ����� �ϹǷ� PQCS�� ���ش�.
		PostQueuedCompletionStatus(_hIOCP, INT32_MAX, 0, reinterpret_cast<LPOVERLAPPED>(RELEASE_SIGN_PQCS));
	}
}

char* LanClient::GetMessageHeaderPtr(Message* pMessage)
{
	//Lan �޽����� ��� ũ�Ⱑ 2����Ʈ�̱� ������ �ڷ� 2ĭ ���� ����� �������̴�.
	return pMessage->GetPayloadPtr() - sizeof(LanHeader);
}

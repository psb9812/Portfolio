#include <WS2tcpip.h>
#include "Session.h"
#include "SystemLog.h"
#include "Message.h"



Session::Session()
	:_sessionID(0), _socket(INVALID_SOCKET), _IOCount(0), _isValid(FALSE),
	_isEmptyIndex(TRUE), _isSending(FALSE), _clientPort(0)
{
	_recvOverlapped._operation = IOOperation::RECV;
	_sendOverlapped._operation = IOOperation::SEND;
}

Session::Session(__int64 ID, SOCKET clientSock, IN_ADDR clientIP, int clientPort)
	:_sessionID(ID), _socket(clientSock), _clientPort(clientPort), _IOCount(0),
	_isValid(FALSE), _isEmptyIndex(TRUE), _isSending(FALSE)
{
	_recvOverlapped._operation = IOOperation::RECV;
	_sendOverlapped._operation = IOOperation::SEND;

	InetNtop(AF_INET, &clientIP, _clientIP, IP_LEN);
}

Session::~Session()
{
	//closesocket(_socket);
}

void Session::LaundrySession(__int64 sessionID, SOCKET clientSock, IN_ADDR clientIP, int clientPort)
{
	_sessionID = sessionID;
	_socket = clientSock;

	if (_sendQ.GetSize() > 0)
	{
		__debugbreak();
	}

	//ť�� ���� ����
	_recvQ.ClearBuffer();

	// �ǵ������� �ϳ� ������Ų��. 
	// ���⼭ ������Ų IOCount�� Accept�� RequestRecv ���Ŀ� ���ҽ�Ų��.
	InterlockedIncrement(&_IOCount);

	//������ �÷��׸� IOCount�� ���� ��Ų �ڿ� ������ ������ ���ɼ��� �ִ� ƴ�� ���� �ʴ´�.
	TurnOffReleaseFlag();

	InetNtop(AF_INET, &clientIP, _clientIP, IP_LEN);
	_clientPort = clientPort;

	_isValid = TRUE;
	_isSending = FALSE;
	_isEmptyIndex = FALSE;
}

void Session::CloseSocket()
{
	closesocket(_socket);
	_socket = INVALID_SOCKET;
}

void Session::CancelIO()
{
	//send, recv Overlapped�� ���� ��Ų��.
	CancelIoEx((HANDLE)_socket, NULL);
}

LONG Session::SetSendingMessageCount(int value)
{
	LONG ret = InterlockedExchange(&_sendingMessageCount, value);
	return ret;
}

void Session::ReleaseSession()
{
	InterlockedExchange(&_isEmptyIndex, TRUE);
}

int Session::RecvBufferEnqueue(char* pSrc, int size)
{
	return _recvQ.Enqueue(pSrc, size);
}

std::optional<LONG> Session::SendBufferEnqueue(Message* pMessage)
{
	return _sendQ.Enqueue(pMessage);
}

int Session::RecvBufferDequeue(char* pSrc, int size)
{
	return _recvQ.Dequeue(pSrc, size);
}

Message* Session::SendBufferDequeue()
{
	auto retData = _sendQ.Dequeue();

	if (retData.has_value())
	{
		return retData.value();
	}
	else
	{
		return nullptr;
	}
}

int Session::RecvBufferPeek(char* pSrc, int size)
{
	return _recvQ.Peek(pSrc, size);
}

int Session::RecvBufferMoveRear(int size)
{
	return _recvQ.MoveRear(size);
}

int Session::RecvBufferMoveFront(int size)
{
	return _recvQ.MoveFront(size);
}

int Session::GetIOBufferUseSize(IOOperation operation)
{
	int useSize = 0;
	switch (operation)
	{
	case IOOperation::SEND:
		useSize = _sendQ.GetSize();
		break;
	case IOOperation::RECV:
		useSize = _recvQ.GetUseSize();
		break;
	default:
		break;
	}

	return useSize;
}

void Session::SentMessageFree(UINT size)
{
	for (UINT i = 0; i < size; i++)
	{
		_sendingMessageArray[i]->SubRefCount();
	}
}

bool Session::RequestRecv()
{
	DWORD flag = 0;
	DWORD recvNumBytes = 0;
	int retOfWSARecv;
	int numOfUsedWSABuf = 0;

	if (_isValid == FALSE)
	{
		return false;
	}

	memset(&_recvOverlapped, 0, sizeof(OVERLAPPED));
	//Overlapped I/O�� ���� �� ������ ������ �ش�.
	WSABUF wsaBuf[2] = { 0, };
	int freeSize = _recvQ.GetFreeSize();
	int dirEnqueSize = _recvQ.DirectEnqueueSize();
	int spareSize = freeSize - dirEnqueSize;

	if (freeSize == dirEnqueSize)  //WSABuf �� ���� ������ ��Ȳ
	{
		numOfUsedWSABuf = 1;
		wsaBuf[0].buf = _recvQ.GetRearBufferPtr();
		wsaBuf[0].len = dirEnqueSize;
	}
	else //�������� ���� ���� Free������ �������� WSABuf �� ���� �ʿ��� ��Ȳ
	{
		numOfUsedWSABuf = 2;
		wsaBuf[0].buf = _recvQ.GetRearBufferPtr();
		wsaBuf[0].len = dirEnqueSize;
		wsaBuf[1].buf = _recvQ.GetBeginBufferPtr();
		wsaBuf[1].len = spareSize;
	}

	InterlockedIncrement(&_IOCount);
	retOfWSARecv = WSARecv(_socket, wsaBuf,
		numOfUsedWSABuf, &recvNumBytes, &flag, (LPWSAOVERLAPPED) &(_recvOverlapped), NULL);

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
			InterlockedDecrement(&_IOCount);
			InterlockedExchange(&_isValid, FALSE);
			if (GetLastError() != 10054)
			{
				SystemLog::GetInstance()->WriteLogConsole(
					SystemLog::LOG_LEVEL_ERROR, L"[Error] sessionID: %d / WSARecv() �Լ� ���� : % d\n",
					_sessionID, GetLastError());
			}
			return false;
		}
	}

	return true;
}



int Session::RequestSend()
{
	DWORD flag = 0;
	int retOfWSASend;
	int sendQSize = 0;
	SystemLog* log = SystemLog::GetInstance();

	while (true)
	{
		if (_isValid == FALSE)
			return RS_INVALID_SESSION;
		//������ üũ�� ���� �Ѵ�.
		if (_sendQ.GetSize() <= 0)
			return RS_ZERO_USESIZE;

		if (InterlockedExchange(&_isSending, TRUE) == TRUE)
			return RS_IO_SENDING;

		sendQSize = _sendQ.GetSize();
		if (sendQSize <= 0)
			InterlockedExchange(&_isSending, FALSE);
		else
			break;
	}

	//WSABUF ����
	WSABUF wsaBufs[MAX_SEND_BUF_ONCE] = { 0, };
	
	DWORD numOfSendMessage = 0;
	for(int i = 0; i < _countof(wsaBufs); i++)
	{
		auto retOptional = _sendQ.Dequeue();
		if (!retOptional.has_value())
		{
			break;
		}
		Message* pMessage = retOptional.value();
		wsaBufs[i].buf = GetHeaderPtr(pMessage);
		wsaBufs[i].len = pMessage->GetDataSize() + pMessage->GetHeaderSize<Net>();
		_sendingMessageArray[i] = pMessage;
		numOfSendMessage++;
	}

	memset(&_sendOverlapped, 0, sizeof(OVERLAPPED));

	InterlockedIncrement(&_IOCount);
	InterlockedExchange(&_sendingMessageCount, numOfSendMessage);

	retOfWSASend = WSASend(_socket, wsaBufs, numOfSendMessage, NULL, flag, (LPWSAOVERLAPPED)&_sendOverlapped, NULL);

	if (retOfWSASend == SOCKET_ERROR)
	{
		int errCode = GetLastError();
		if (errCode == WSA_IO_PENDING)
		{
			//g_sendIoPendingCnt++;
		}
		else
		{
			InterlockedDecrement(&_IOCount);
			InterlockedExchange(&_isValid, FALSE);
			InterlockedExchange(&_isSending, FALSE);
			if (errCode != 10054)
			{
				log->WriteLogConsole(
					SystemLog::LOG_LEVEL_ERROR, L"[Error] SessionID : %lld / WSASend() �Լ� ���� : % d\n",
					_sessionID, GetLastError()
				);
			}
			return RS_WSASEND_ERROR;
		}
	}
	return RS_SUCCESS;
}


int Session::IncreaseIOCount()
{
	int ret = InterlockedIncrement(&_IOCount);
	return ret;
}

int Session::DecreaseIOCount()
{
	int ret = InterlockedDecrement(&_IOCount);
	return ret;
}

bool Session::IsPossibleToDisconnect()
{
	bool ret = (_IOCount == 0) && (_isValid == false);

	return ret;
}

bool Session::ConnectSocketToIOCP(HANDLE hIOCP)
{

	HANDLE hRet = CreateIoCompletionPort((HANDLE)_socket, hIOCP, _sessionID, 0);

	return (hRet != NULL) ? true : false;
}

__int64 Session::GetSessionID() const
{
	return _sessionID;
}

void Session::CompleteSending()
{
	InterlockedExchange(&_isSending, FALSE);
}

bool Session::IsValid()
{
	return _isValid;
}

bool Session::IsEmptyIndex()
{
	return _isEmptyIndex;
}

void Session::InvalidateSession()
{
	InterlockedExchange(&_isValid, FALSE);
}

char* Session::GetHeaderPtr(Message* pMessage)
{
	return pMessage->GetHeaderPtr();
}

void Session::ClearSendQ()
{
	_sendQ.Clear();
}

bool Session::CheckCanRelease()
{
	//int exchangeValue = 0 | RELEASE_BIT_MASK;	//RELEASE_BIT_MASK�� �ֻ��� ��Ʈ�� 1�� ���̴�.
	//IOCount�� �ֻ��� ��Ʈ�� 1�̸� ������ ���̶�� ����.
	if (InterlockedCompareExchange(&_IOCount, RELEASE_BIT_MASK, 0) == 0)
	{
		return true;
	}
	else
	{
		return false;
	}
}

void Session::TurnOffReleaseFlag()
{
	InterlockedAnd((LONG*) & _IOCount, ~RELEASE_BIT_MASK);
}



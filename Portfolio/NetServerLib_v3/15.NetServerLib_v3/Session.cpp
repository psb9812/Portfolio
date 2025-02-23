#include <WS2tcpip.h>
#include "Session.h"
#include "SystemLog.h"
#include "Message.h"



Session::Session()
	:_sessionID(0), _socket(INVALID_SOCKET), _IOCount(0), _isValid(FALSE),
	_isEmptyIndex(TRUE), _isSending(FALSE), _clientPort(0), _recvQ(4000)
{
	_recvOverlapped._operation = IOOperation::RECV;
	_sendOverlapped._operation = IOOperation::SEND;
}

Session::Session(__int64 ID, SOCKET clientSock, IN_ADDR clientIP, int clientPort)
	:_sessionID(ID), _socket(clientSock), _clientPort(clientPort), _IOCount(0),
	_isValid(FALSE), _isEmptyIndex(TRUE), _isSending(FALSE), _recvQ(4000)
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
		__debugbreak();
	if (_recvMessageQueue.GetSize() > 0)
		__debugbreak();
	//큐의 내용 비우기
	_recvQ.ClearBuffer();

	// 여기서 증가시킨 IOCount는 Accept의 RequestRecv 이후에 감소시킨다.
	//릴리즈 플래그를 IOCount를 증가 시킨 뒤에 내려서 삭제될 가능성이 있는 틈을 주지 않는다.
	InterlockedIncrement(&_IOCount);
	TurnOffReleaseFlag();

	InetNtop(AF_INET, &clientIP, _clientIP, IP_LEN);
	_clientPort = clientPort;

	_isValid = TRUE;
	_isSending = FALSE;
	_isEmptyIndex = FALSE;
	_isBlockAsyncRecv = FALSE;
	_contentsID = NON_CONTENTS;
}

void Session::SentMessageFree(UINT size)
{
	for (UINT i = 0; i < size; i++)
		_sendingMessageArray[i]->SubRefCount();
}

bool Session::RequestRecv()
{
	DWORD flag = 0;
	DWORD recvNumBytes = 0;
	int retOfWSARecv;
	int numOfUsedWSABuf = 0;

	if (_isValid == FALSE)
		return false;

	memset(&_recvOverlapped, 0, sizeof(OVERLAPPED));
	//Overlapped I/O를 위해 각 정보를 셋팅해 준다.
	WSABUF wsaBuf[2] = { 0, };
	int freeSize = _recvQ.GetFreeSize();
	int dirEnqueSize = _recvQ.DirectEnqueueSize();
	int spareSize = freeSize - dirEnqueSize;

	numOfUsedWSABuf = 2;
	wsaBuf[0].buf = _recvQ.GetRearBufferPtr();
	wsaBuf[0].len = dirEnqueSize;
	wsaBuf[1].buf = _recvQ.GetBeginBufferPtr();
	wsaBuf[1].len = spareSize;

	InterlockedIncrement(&_IOCount);
	retOfWSARecv = WSARecv(_socket, wsaBuf, numOfUsedWSABuf, &recvNumBytes, &flag, (LPWSAOVERLAPPED) & (_recvOverlapped), NULL);

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
			InterlockedDecrement(&_IOCount);
			InterlockedExchange(&_isValid, FALSE);
			if (errCode != 10054)
			{
				SystemLog::GetInstance()->WriteLogConsole(SystemLog::LOG_LEVEL_ERROR, L"[Error] sessionID: %d / WSARecv() 함수 실패 : % d\n",_sessionID, errCode);
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
		//사이즈 체크를 먼저 한다.
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

	//WSABUF 세팅
	WSABUF wsaBufs[MAX_SEND_BUF_ONCE] = { 0, };

	ULONG numOfSendMessage = 0;
	for (int i = 0; i < _countof(wsaBufs); i++)
	{
		auto retOptional = _sendQ.Dequeue();
		if (!retOptional.has_value())
			break;
		Message* pMessage = retOptional.value();
		wsaBufs[i].buf = GetHeaderPtr(pMessage);
		wsaBufs[i].len = pMessage->GetDataSize() + pMessage->GetHeaderSize<Net>();
		_sendingMessageArray[i] = pMessage;
		numOfSendMessage++;
	}

	memset(&_sendOverlapped, 0, sizeof(OVERLAPPED));

	ULONG ioCnt = InterlockedIncrement(&_IOCount);
	InterlockedExchange(&_sendingMessageCount, numOfSendMessage);
	//mLog.WriteMemoryLog(SendMessagePerSendPost, _sessionID, numOfSendMessage);
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
					SystemLog::LOG_LEVEL_ERROR, L"[Error] SessionID : %d / WSASend() 함수 실패 : % d\n",
					_sessionID, GetLastError()
				);
			}
			return RS_WSASEND_ERROR;
		}
	}
	return RS_SUCCESS;
}

bool Session::ConnectSocketToIOCP(HANDLE hIOCP)
{
	HANDLE hRet = CreateIoCompletionPort((HANDLE)_socket, hIOCP, _sessionID, 0);

	return (hRet != NULL) ? true : false;
}

char* Session::GetHeaderPtr(Message* pMessage)
{
	return pMessage->GetHeaderPtr();
}

bool Session::CheckCanRelease()
{
	int exchangeValue = 0 | RELEASE_BIT_MASK;	//0x80000000은 최상위 비트가 1인 수이다.
	//IOCount의 최상위 비트가 1이면 삭제될 것이라는 것임.
	if (InterlockedCompareExchange(&_IOCount, exchangeValue, 0) == 0)
		return true;
	else
		return false;
}

void Session::LeaveContents()
{
	InterlockedExchange(&_contentsID, NON_CONTENTS);
}
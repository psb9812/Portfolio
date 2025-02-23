#pragma once
#include <WinSock2.h>
#include "Tools/LockFreeQueue.h"
#include "Tools/LockFreeStack.h"
#include "Tools/RingBuffer.h"
#include "Contents.h"

#define	IP_LEN		16

//Request Send�� ��Ȳ�� ���� ��
#define	RS_IO_SENDING		1
#define RS_INVALID_SESSION	2
#define RS_ZERO_USESIZE		3
#define	RS_WSASEND_ERROR	4
#define	RS_SUCCESS			5

class Message;

enum class IOOperation
{
	SEND,
	RECV
};

struct OverlappedEx
{
	OVERLAPPED		_overlapped;
	IOOperation		_operation;
};

struct Session
{
	static constexpr int RELEASE_BIT_MASK = 0x80000000;
	static constexpr int MAX_SEND_BUF_ONCE = 60;		//�� ���� Send ��û���� ���� �� �ִ� ����ȭ ������ ����

	__int64					_sessionID;
	SOCKET					_socket;
	LONG					_isSending = FALSE;
	LONG					_isRecving = FALSE;
	OverlappedEx			_recvOverlapped;
	OverlappedEx			_sendOverlapped;
	RingBuffer				_recvQ;
	LockFreeQueue<Message*>	_sendQ;
	Message*				_sendingMessageArray[MAX_SEND_BUF_ONCE];
	ULONG					_IOCount;
	LONG					_sendingMessageCount = 0;
	WCHAR					_clientIP[IP_LEN] = { 0, };
	int						_clientPort;
	LONG					_isValid = TRUE;					//���� ������ ��ȿ�� �÷���
	LONG					_isEmptyIndex = TRUE;				//_sessionArray���� ����ִ� �ε��������� ���� �÷���
	LONG					_isBlockAsyncRecv = FALSE;			//�񵿱� ���ú긦 ���� �÷���
	std::optional<INT32>	_contentsID;						//�Ҽ��� contentID

public:
	Session();
	Session(__int64 ID, SOCKET clientSock, IN_ADDR clientIP, int clientPort);
	~Session();

	// ���ο� ������ �Ҵ� ���� �� ���� ��Ź �ϴ� �Լ�
	void LaundrySession(__int64 sessionID, SOCKET clientSock, IN_ADDR clientIP, int clientPort);
	// Send �Ϸ�� �޽����� ���� ī��Ʈ�� ���ҽ�Ű�� �Լ�
	void SentMessageFree(UINT size);

	// IO��û �Լ�
	bool RequestRecv();
	int RequestSend();

	//IOCount ���� �Լ�
	inline int IncreaseIOCount()
	{
		int ret = InterlockedIncrement(&_IOCount);
		return ret;
	}
	inline int	DecreaseIOCount()
	{
		int ret = InterlockedDecrement(&_IOCount);
		return ret;
	}

	//���� �������� üũ �ϴ� �Լ�
	inline bool IsPossibleToDisconnect()
	{
		bool ret = (_IOCount == 0) && (_isValid == false);

		return ret;
	}

	// IOCP�� ������ ������ �����ϴ� �Լ�
	bool ConnectSocketToIOCP(HANDLE hIOCP);

	inline void CompleteSending() { InterlockedExchange(&_isSending, FALSE); }
	inline void CompleteRecving() { InterlockedExchange(&_isRecving, FALSE); }

	inline void InvalidateSession() { InterlockedExchange(&_isValid, FALSE); }

	bool CheckCanRelease();
	inline void TurnOffReleaseFlag() { InterlockedAnd((LONG*)&_IOCount, ~RELEASE_BIT_MASK); }

	inline void SetAsyncRecvBlockFlag() { InterlockedExchange(&_isBlockAsyncRecv, TRUE); }
	inline void ResetAsyncRecvBlockFlag() { InterlockedExchange(&_isBlockAsyncRecv, FALSE); }
	inline LONG GetAsyncRecvBlockFlag() const { return _isBlockAsyncRecv; }

	inline void EnterContents(INT32 contentID) { _contentsID = contentID; }
	bool LeaveContents();

	char* GetHeaderPtr(Message* pMessage);
};
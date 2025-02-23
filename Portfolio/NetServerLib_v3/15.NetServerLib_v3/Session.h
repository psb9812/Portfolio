#pragma once
#include <WinSock2.h>
#include "LockFreeQueue.h"
#include "LockFreeStack.h"
#include "RingBuffer.h"
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
	RingBuffer				_recvQ;				// WSARecv�� ���� ������ ���� ���� ť
	LockFreeQueue<Message*>	_sendQ;				// WSASend�� �޽��� ���� ť
	LockFreeQueue<Message*> _recvMessageQueue;	// ���� �������� ���� �޽��� ���� ť
	LONG					_contentsID;		// �Ҽ��� contentID, ���� NON_CONTENTS�̶�� ���ķ� ���
	static constexpr int NON_CONTENTS = -1;								// �������� �ҼӵǾ� ���� �ʴٴ� �ǹ��� ��.

	static constexpr int RELEASE_BIT_MASK = 0x80000000;					// IOCount ���� Release �÷��׸� ������ �� ����ϴ� ��Ʈ �÷���
	static constexpr int MAX_SEND_BUF_ONCE = 300;						// �� ���� Send ��û���� ���� �� �ִ� ����ȭ ������ ����

	__int64					_sessionID;									// ���� ID (���� 16 : index �� / ���� 48��Ʈ : ID��)
	SOCKET					_socket;									// ����
	LONG					_isSending = FALSE;							// Send 1ȸ ���ѿ� �÷��� ����
	OverlappedEx			_recvOverlapped;							// recv Overlapped ����ü
	OverlappedEx			_sendOverlapped;							// send Overlapped ����ü
	RingBuffer				_recvQ;										// recv ������
	LockFreeQueue<Message*>	_sendQ;										// send �� ���� ť
	Message*				_sendingMessageArray[MAX_SEND_BUF_ONCE];	// WSASend - Send �Ϸ� ���� ���̿� ���� ����ȭ ���۸� �����ϴ� �迭
	ULONG					_IOCount;									// ���� ������ ���۷��� ī��Ʈ
	LONG					_sendingMessageCount;						// ���� �޽��� ���� (�Ϸ� �������� �� ������ŭ ����ȭ ���۸� �����Ѵ�)
	WCHAR					_clientIP[IP_LEN] = { 0, };					// Ŭ���̾�Ʈ�� IP
	int						_clientPort;								// Ŭ���̾�Ʈ�� ��Ʈ
	LONG					_isValid = TRUE;							// ���� ������ ��ȿ�� �÷���
	LONG					_isEmptyIndex = TRUE;						// _sessionArray���� ����ִ� �ε��������� ���� �÷���
	LONG					_isBlockAsyncRecv = FALSE;					// �񵿱� ���ú긦 ���� �÷���
																		   
	LONG					_contentsID;								// �Ҽ��� contentID
	LockFreeQueue<Message*> _recvMessageQueue;							// ������ �����忡�� ������ �޽����� ����ִ� ť.

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

	inline void InvalidateSession() { InterlockedExchange(&_isValid, FALSE); }

	bool CheckCanRelease();
	inline void TurnOffReleaseFlag() { InterlockedAnd((LONG*)&_IOCount, ~RELEASE_BIT_MASK); }

	inline void SetAsyncRecvBlockFlag() { InterlockedExchange(&_isBlockAsyncRecv, TRUE); }
	inline void ResetAsyncRecvBlockFlag() { InterlockedExchange(&_isBlockAsyncRecv, FALSE); }
	inline LONG GetAsyncRecvBlockFlag() const { return _isBlockAsyncRecv; }

	inline void EnterContents(INT32 contentID) { InterlockedExchange(&_contentsID, contentID); }
	void LeaveContents();

	char* GetHeaderPtr(Message* pMessage);
};
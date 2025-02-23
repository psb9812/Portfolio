#pragma once
#include <WinSock2.h>
#include "LockFreeQueue.h"
#include "LockFreeStack.h"
#include "RingBuffer.h"

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

class Session
{
public:
	static constexpr int RELEASE_BIT_MASK = 0x80000000;
private:
	static constexpr int MAX_SEND_BUF_ONCE = 50;		//�� ���� Send ��û���� ���� �� �ִ� ����ȭ ������ ����

	__int64					_sessionID;
	SOCKET					_socket;

	LONG					_isSending = FALSE;

	OverlappedEx			_recvOverlapped;
	OverlappedEx			_sendOverlapped;

	ULONG					_IOCount;

	RingBuffer				_recvQ;

	LockFreeQueue<Message*>	_sendQ;

	LONG					_isValid = TRUE;			//���� ������ ��ȿ�� �÷���

	Message*				_sendingMessageArray[MAX_SEND_BUF_ONCE];

	LONG					_isEmptyIndex = TRUE;		//_sessionArray���� ����ִ� �ε��������� ���� �÷���

	WCHAR					_clientIP[IP_LEN] = { 0, };
	int						_clientPort;

	LONG					_sendingMessageCount;


public:
	Session();
	Session(__int64 ID, SOCKET clientSock, IN_ADDR clientIP, int clientPort);
	~Session();

	// ���ο� ������ �Ҵ� ���� �� ���� ��Ź �ϴ� �Լ�
	void LaundrySession(__int64 sessionID, SOCKET clientSock, IN_ADDR clientIP, int clientPort);

	// ���� ����
	void ReleaseSession();

	// ���� �ݱ�
	void CloseSocket();

	// overlapped ����ü��� ���� ���̴� IO�� ĵ���ϱ�
	void CancelIO();

	LONG SetSendingMessageCount(int value);

	int RecvBufferEnqueue(char* pSrc, int size);
	std::optional<LONG> SendBufferEnqueue(Message* pMessage);

	int RecvBufferDequeue(char* pSrc, int size);
	Message* SendBufferDequeue();

	int RecvBufferPeek(char* pSrc, int size);
	int RecvBufferMoveRear(int size);
	int RecvBufferMoveFront(int size);
	int GetIOBufferUseSize(IOOperation operation);

	// Send �Ϸ�� �޽����� ���� ī��Ʈ�� ���ҽ�Ű�� �Լ�
	void SentMessageFree(UINT size);

	// IO��û �Լ�
	bool RequestRecv();
	int RequestSend();

	//IOCount ���� �Լ�
	int IncreaseIOCount();
	int	DecreaseIOCount();

	//���� �������� üũ �ϴ� �Լ�
	bool IsPossibleToDisconnect();

	// IOCP�� ������ ������ �����ϴ� �Լ�
	bool ConnectSocketToIOCP(HANDLE hIOCP);

	__int64 GetSessionID() const;

	//������ Send �۾��� �Ϸ������� ��Ÿ���� �Լ�
	// �� �Լ��� ȣ��Ǹ� Send�� ������ ��Ȳ�̴�.
	void CompleteSending();

	bool IsValid();
	bool IsEmptyIndex();
	void InvalidateSession();

	char* GetHeaderPtr(Message* pMessage);

	void ClearSendQ();

	bool CheckCanRelease();

	void TurnOffReleaseFlag();
};



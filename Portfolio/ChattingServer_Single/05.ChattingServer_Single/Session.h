#pragma once
#include <WinSock2.h>
#include "LockFreeQueue.h"
#include "LockFreeStack.h"
#include "RingBuffer.h"

#define	IP_LEN		16

//Request Send의 상황별 리턴 값
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
	static constexpr int MAX_SEND_BUF_ONCE = 50;		//한 번의 Send 요청으로 보낼 수 있는 직렬화 버퍼의 개수

	__int64					_sessionID;
	SOCKET					_socket;

	LONG					_isSending = FALSE;

	OverlappedEx			_recvOverlapped;
	OverlappedEx			_sendOverlapped;

	ULONG					_IOCount;

	RingBuffer				_recvQ;

	LockFreeQueue<Message*>	_sendQ;

	LONG					_isValid = TRUE;			//현재 세션의 유효성 플래그

	Message*				_sendingMessageArray[MAX_SEND_BUF_ONCE];

	LONG					_isEmptyIndex = TRUE;		//_sessionArray에서 비어있는 인덱스인지에 관한 플래그

	WCHAR					_clientIP[IP_LEN] = { 0, };
	int						_clientPort;

	LONG					_sendingMessageCount;


public:
	Session();
	Session(__int64 ID, SOCKET clientSock, IN_ADDR clientIP, int clientPort);
	~Session();

	// 새로운 세션을 할당 받을 때 세션 세탁 하는 함수
	void LaundrySession(__int64 sessionID, SOCKET clientSock, IN_ADDR clientIP, int clientPort);

	// 세션 삭제
	void ReleaseSession();

	// 소켓 닫기
	void CloseSocket();

	// overlapped 구조체들로 진행 중이던 IO를 캔슬하기
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

	// Send 완료된 메시지의 참조 카운트를 감소시키는 함수
	void SentMessageFree(UINT size);

	// IO요청 함수
	bool RequestRecv();
	int RequestSend();

	//IOCount 조작 함수
	int IncreaseIOCount();
	int	DecreaseIOCount();

	//삭제 가능한지 체크 하는 함수
	bool IsPossibleToDisconnect();

	// IOCP와 세션의 소켓을 연결하는 함수
	bool ConnectSocketToIOCP(HANDLE hIOCP);

	__int64 GetSessionID() const;

	//기존의 Send 작업이 완료했음을 나타내는 함수
	// 이 함수가 호출되면 Send가 가능한 상황이다.
	void CompleteSending();

	bool IsValid();
	bool IsEmptyIndex();
	void InvalidateSession();

	char* GetHeaderPtr(Message* pMessage);

	void ClearSendQ();

	bool CheckCanRelease();

	void TurnOffReleaseFlag();
};



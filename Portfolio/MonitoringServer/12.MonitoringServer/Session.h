#pragma once
#include <WinSock2.h>
#include "Tools/LockFreeQueue.h"
#include "Tools/LockFreeStack.h"
#include "Tools/RingBuffer.h"
#include "Contents.h"

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

struct Session
{
	static constexpr int RELEASE_BIT_MASK = 0x80000000;
	static constexpr int MAX_SEND_BUF_ONCE = 60;		//한 번의 Send 요청으로 보낼 수 있는 직렬화 버퍼의 개수

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
	LONG					_isValid = TRUE;					//현재 세션의 유효성 플래그
	LONG					_isEmptyIndex = TRUE;				//_sessionArray에서 비어있는 인덱스인지에 관한 플래그
	LONG					_isBlockAsyncRecv = FALSE;			//비동기 리시브를 막는 플래그
	std::optional<INT32>	_contentsID;						//소속한 contentID

public:
	Session();
	Session(__int64 ID, SOCKET clientSock, IN_ADDR clientIP, int clientPort);
	~Session();

	// 새로운 세션을 할당 받을 때 세션 세탁 하는 함수
	void LaundrySession(__int64 sessionID, SOCKET clientSock, IN_ADDR clientIP, int clientPort);
	// Send 완료된 메시지의 참조 카운트를 감소시키는 함수
	void SentMessageFree(UINT size);

	// IO요청 함수
	bool RequestRecv();
	int RequestSend();

	//IOCount 조작 함수
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

	//삭제 가능한지 체크 하는 함수
	inline bool IsPossibleToDisconnect()
	{
		bool ret = (_IOCount == 0) && (_isValid == false);

		return ret;
	}

	// IOCP와 세션의 소켓을 연결하는 함수
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
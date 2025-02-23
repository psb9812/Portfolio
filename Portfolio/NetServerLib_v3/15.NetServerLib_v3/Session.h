#pragma once
#include <WinSock2.h>
#include "LockFreeQueue.h"
#include "LockFreeStack.h"
#include "RingBuffer.h"
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
	RingBuffer				_recvQ;				// WSARecv로 받은 데이터 보관 원형 큐
	LockFreeQueue<Message*>	_sendQ;				// WSASend할 메시지 보관 큐
	LockFreeQueue<Message*> _recvMessageQueue;	// 직렬 컨텐츠의 세션 메시지 보관 큐
	LONG					_contentsID;		// 소속한 contentID, 값이 NON_CONTENTS이라면 병렬로 취급
	static constexpr int NON_CONTENTS = -1;								// 컨텐츠에 소속되어 있지 않다는 의미의 값.

	static constexpr int RELEASE_BIT_MASK = 0x80000000;					// IOCount 값에 Release 플래그를 삽입할 때 사용하는 비트 플래그
	static constexpr int MAX_SEND_BUF_ONCE = 300;						// 한 번의 Send 요청으로 보낼 수 있는 직렬화 버퍼의 개수

	__int64					_sessionID;									// 세션 ID (상위 16 : index 값 / 하위 48비트 : ID값)
	SOCKET					_socket;									// 소켓
	LONG					_isSending = FALSE;							// Send 1회 제한용 플래그 변수
	OverlappedEx			_recvOverlapped;							// recv Overlapped 구조체
	OverlappedEx			_sendOverlapped;							// send Overlapped 구조체
	RingBuffer				_recvQ;										// recv 링버퍼
	LockFreeQueue<Message*>	_sendQ;										// send 락 프리 큐
	Message*				_sendingMessageArray[MAX_SEND_BUF_ONCE];	// WSASend - Send 완료 통지 사이에 사용된 직렬화 버퍼를 저장하는 배열
	ULONG					_IOCount;									// 수명 관리용 레퍼런스 카운트
	LONG					_sendingMessageCount;						// 보낸 메시지 개수 (완료 통지에서 이 개수만큼 직렬화 버퍼를 정리한다)
	WCHAR					_clientIP[IP_LEN] = { 0, };					// 클라이언트의 IP
	int						_clientPort;								// 클라이언트의 포트
	LONG					_isValid = TRUE;							// 현재 세션의 유효성 플래그
	LONG					_isEmptyIndex = TRUE;						// _sessionArray에서 비어있는 인덱스인지에 관한 플래그
	LONG					_isBlockAsyncRecv = FALSE;					// 비동기 리시브를 막는 플래그
																		   
	LONG					_contentsID;								// 소속한 contentID
	LockFreeQueue<Message*> _recvMessageQueue;							// 컨텐츠 스레드에서 꺼내는 메시지가 담겨있는 큐.

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
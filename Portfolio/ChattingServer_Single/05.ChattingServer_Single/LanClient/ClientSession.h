#pragma once
#include "../LockFreeQueue.h"
#include "../Session.h"
#include "../Message.h"

struct ClientSession
{
	static constexpr int MAX_SEND_BUF_ONCE = 60;

	LONG _IOCount;
	bool _connect;
	SOCKET _socket;

	LONG _isSending;

	OverlappedEx _recvOverlapped;
	OverlappedEx _sendOverlapped;

	RingBuffer				_recvQ;
	LockFreeQueue<Message*> _sendQ;
	Message* _sendingMessageArray[MAX_SEND_BUF_ONCE];

	LONG _sendingMessageCount;

	ClientSession(SOCKET sock)
		:_IOCount(0), _connect(true), _socket(sock), _isSending(0), _sendingMessageCount(0)
	{
		//Overlapped 구조체 초기화
		_recvOverlapped._operation = IOOperation::RECV;
		_sendOverlapped._operation = IOOperation::SEND;
		memset(&_recvOverlapped, 0, sizeof(_recvOverlapped._overlapped));
		memset(&_sendOverlapped, 0, sizeof(_sendOverlapped._overlapped));
	}
};
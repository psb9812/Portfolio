#pragma once
#include "RingBuffer.h"
#include "WinSock2.h"

struct Session
{
	DWORD sessionID;
	SOCKET socket;
	SOCKADDR_IN sockaddr;
	RingBuffer sendRingBuffer;
	RingBuffer recvRingBuffer;

	DWORD action;
	BYTE direction;
	short x;
	short y;

	char HP;
};
#pragma once
#include "Session.h"
#include <Windows.h>
#include "LockFreeStack.h"

class SessionContainer
{
public:
	Session*				_sessionArray;				//���� ������ �迭
	LockFreeStack<USHORT>	_insertIndexStack;
	unsigned int			_currentUserNum;

	const int				ID_BITWIDE = 48;	//���� ID�� ID ������ ��Ʈ ��

public:
	~SessionContainer();

	void		InitContainer(int numOfMaxUser);

	Session*	SearchSession(__int64 sessionID);
	Session*	InsertSession(__int64 ID, SOCKET clientSock, IN_ADDR clientIP, int clientPort);
	void		DeleteSession(__int64 sessionID);

	inline int	GetSessionNum()	const  { return _currentUserNum; }

private:
	__int64		MakeSessionID(__int64 ID, USHORT insertIdx);
};

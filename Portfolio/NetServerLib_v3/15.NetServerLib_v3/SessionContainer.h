#pragma once
#include "Session.h"
#include <Windows.h>
#include "LockFreeStack.h"

class SessionContainer
{
public:
	Session* _sessionArray;				//���� ������ �迭
	LockFreeStack<USHORT>	_insertIndexStack;
	LONG					_currentUserNum;
	LONG					_maxUserNum;


	const int				ID_BITWIDE = 48;	//���� ID�� ID ������ ��Ʈ ��

public:
	SessionContainer();
	~SessionContainer();

	void		InitContainer(int numOfMaxUser);

	Session*	SearchSession(INT64 sessionID);
	Session*	InsertSession(INT64 ID, SOCKET clientSock, IN_ADDR clientIP, int clientPort);
	void		DeleteSession(INT64 sessionID);

	inline int	GetSessionNum()	const { return _currentUserNum; }

private:
	INT64		MakeSessionID(INT64 ID, USHORT insertIdx);
};

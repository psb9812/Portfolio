#pragma once

#include "Windows.h"

class User
{
private:
	INT64		_sessionID;
	INT64		_userID;
	UINT64		_lastRecvTime;		//�������� ������ �ð� for Ÿ�Ӿƿ�

	static inline INT64 _userIDCounter = 0;

public:
	User();
	void Init(INT64 sessionID);
	void UpdateLastRecvTime();
	UINT64 GetLastRecvTime() const;
};


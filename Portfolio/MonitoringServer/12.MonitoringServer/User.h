#pragma once
#include "IUser.h"
#include "Windows.h"

struct User : public IUser
{
public:
	User(INT64 sessionID);
	~User();

	INT64 _sessionID;
	INT64 _userID;

	inline static INT64 s_userIDGen = 0;
};
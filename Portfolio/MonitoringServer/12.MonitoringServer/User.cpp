#include "User.h"

User::User(INT64 sessionID)
	:_sessionID(sessionID), _userID(s_userIDGen++)
{
}

User::~User()
{
}

#pragma once
#include "IUser.h"
#include <cstdint>

class User : public IUser
{
private:
	INT64		_sessionID;
	INT64		_userID;

	UINT64		_lastRecvTime;		//마지막로 수신한 시간 for 타임아웃

	//Data from login request
	INT64		_accountNo;
	bool		_isLogin;			//로그인 여부

	static inline INT64 _userIDCounter = 0;
public:
	User();
	void Init(INT64 sessionID);

	void UpdateLastRecvTime();

	__forceinline void SetAccountNo(UINT64 accountNo)
	{
		_accountNo = accountNo;
	}
	__forceinline void Login()
	{
		_isLogin = true;
	}
	__forceinline void Logout()
	{
		_isLogin = false;
	}

	__forceinline INT64 GetSessionID() const
	{
		return _sessionID;
	}

	__forceinline INT64 GetAccountNo() const
	{
		return _accountNo;
	}
	__forceinline bool GetLogin() const
	{
		return _isLogin;
	}
	__forceinline UINT64 GetLastRecvTime() const
	{
		return _lastRecvTime;
	}
};


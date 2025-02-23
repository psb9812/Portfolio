#pragma once
#include "Sector.h"
#include "Windows.h"

class User
{
private:
	INT64		_sessionID;
	INT64		_userID;
	UINT64		_lastRecvTime;		//마지막로 수신한 시간 for 타임아웃
	INT64		_accountNo;
	WCHAR		_ID[20];
	WCHAR		_nickName[20];

	SectorPos	_curSector;			//현재 섹터 좌표
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
	__forceinline void SetID(WCHAR* id)
	{
		wcscpy_s(_ID, id);
	}
	__forceinline void SetNickName(WCHAR* nickName)
	{
		wcscpy_s(_nickName, nickName);
	}
	__forceinline void SetSectorX(WORD sectorX)
	{
		_curSector._x = sectorX;
	}
	__forceinline void SetSectorY(WORD sectorY)
	{
		_curSector._y = sectorY;
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
	__forceinline SectorPos GetCurSector() const
	{
		return _curSector;
	}
	__forceinline WCHAR* GetIDPtr() const
	{
		return (WCHAR*)_ID;
	}
	__forceinline WCHAR* GetNickNamePtr() const
	{
		return (WCHAR*)_nickName;
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


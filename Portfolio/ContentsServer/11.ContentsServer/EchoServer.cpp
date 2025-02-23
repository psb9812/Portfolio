#include "pch.h"
#include "EchoServer.h"
#include "LoginContents.h"
#include "EchoContents.h"
#include "User.h"
#include "Message.h"

EchoServer::EchoServer()
    :_userPool(0, true)
{
	InitializeSRWLock(&_userContainerLock);

	//컨텐츠 등록
    AddContents(static_cast<INT32>(ContentsID::Login), new LoginContents(this, 10, static_cast<INT32>(ContentsID::Login)));
    AddContents(static_cast<INT32>(ContentsID::Echo), new EchoContents(this, 30, static_cast<INT32>(ContentsID::Echo)));

	//세션이 접속하면 기본값으로 소속되는 컨텐츠 설정
    SetDefaultContents(static_cast<INT32>(ContentsID::Login));
}

EchoServer::~EchoServer()
{
}

bool EchoServer::Start(const WCHAR* configFileName)
{
	NetServer::Start(configFileName);
	_isServerAlive = true;

	wprintf(L"# EchoServer 시작.\n");
	return true;
}

void EchoServer::Stop()
{
	NetServer::Stop();
	_isServerAlive = false;

	wprintf(L"# EchoServer 종료.\n");
}


bool EchoServer::ServerControl()
{
	static bool controlMode = false;

	//--------------------------------------------------------
	//L : 컨트롤 Lock  /  U : 컨트롤 Unlock  /  Q : 서버 종료
	//--------------------------------------------------------
	if (_kbhit())
	{
		WCHAR controlKey = _getwch();

		//키보드 제어 허용
		if (L'u' == controlKey || L'U' == controlKey)
		{
			controlMode = true;
			//관련 키 도움말 출력
			wprintf(L"--------------------------------------------------\n");
			wprintf(L"# Control Mode : Press Q - Quit\n");
			wprintf(L"# Control Mode : Press L - Key Lock\n");
			wprintf(L"# Control Mode : Press 0 - Set log level debug\n");
			wprintf(L"# Control Mode : Press 1 - Set log level error\n");
			wprintf(L"# Control Mode : Press 2 - Set log level system\n");
			wprintf(L"--------------------------------------------------\n");
		}
		if ((L'l' == controlKey || L'L' == controlKey) && controlMode)
		{
			wprintf(L"Control Lock..! Press U - Control Unlock\n");
			controlMode = false;
		}
		if ((L'q' == controlKey || L'Q' == controlKey) && controlMode)
		{
			Stop();
			return false;
		}
		if (L'0' == controlKey && controlMode)
		{
			SystemLog::GetInstance()->SetLogLevel(SystemLog::LOG_LEVEL_DEBUG);
			wprintf(L"Current Log Level : LOG_LEVEL_DEBUG \n");
		}
		if (L'1' == controlKey && controlMode)
		{
			SystemLog::GetInstance()->SetLogLevel(SystemLog::LOG_LEVEL_ERROR);
			wprintf(L"Current Log Level : LOG_LEVEL_ERROR \n");
		}
		if (L'2' == controlKey && controlMode)
		{
			SystemLog::GetInstance()->SetLogLevel(SystemLog::LOG_LEVEL_SYSTEM);
			wprintf(L"Current Log Level : LOG_LEVEL_SYSTEM \n");
		}
		if ((L'w' == controlKey || L'W' == controlKey) && controlMode)
		{
			//ProfileManager::ProfileDataOutText(L"ChatServer_Single_Proflie.txt");
		}
		if ((L'r' == controlKey || L'R' == controlKey) && controlMode)
		{
			//ProfileManager::ProfileReset();
		}
	}
	return true;
}

bool EchoServer::InsertUser(INT64 sessionID, User* pUser)
{
	AcquireSRWLockExclusive(&_userContainerLock);
	auto insertRet = _userContainer.insert({ sessionID, pUser });
	if (!insertRet.second)
	{
		SystemLog::GetInstance()->WriteLog(L"ERROR", SystemLog::LOG_LEVEL_ERROR, L"중복 유저 삽입 시도!");
		ReleaseSRWLockExclusive(&_userContainerLock);
		return false;
	}
	ReleaseSRWLockExclusive(&_userContainerLock);
	return true;
}

bool EchoServer::DeleteUser(INT64 sessionID)
{
	AcquireSRWLockExclusive(&_userContainerLock);
	size_t eraseNum = _userContainer.erase(sessionID);
	if (eraseNum == 0)
	{
		SystemLog::GetInstance()->WriteLog(L"ERROR", SystemLog::LOG_LEVEL_ERROR, L"존재하지 않는 유저 삭제 시도");
		ReleaseSRWLockExclusive(&_userContainerLock);
		return false;
	}
	ReleaseSRWLockExclusive(&_userContainerLock);
	return true;
}

void EchoServer::FreeUserMemory(User* pUser)
{
	_userPool.Free(pUser);
}

bool EchoServer::OnConnectionRequest(const WCHAR* connectIP, INT32 connectPort)
{
    return true;
}

void EchoServer::OnAccept(INT64 sessionID)
{
    User* pUser = _userPool.Alloc();
    pUser->Init(sessionID);

	InsertUser(sessionID, pUser);
}

void EchoServer::OnRelease(INT64 sessionID)
{
}

void EchoServer::OnRecv(INT64 sessionID, Message* pMessage)
{
}

void EchoServer::OnSend(INT64 sessionID, INT32 sendSize)
{
}

void EchoServer::OnTime()
{
	//모든 User를 순회하며 타임 아웃 여부를 체크하고 타임 아웃이면 연결을 끊는다.
	/*
	for (auto& userPair : _userContainer)
	{
		User* pUser = userPair.second;
		UINT64 timeOutValue = pUser->GetLogin() ? _timeoutDisconnectForUser : _timeoutDisconnectForSession;
		ULONGLONG deltaTime = GetTickCount64() - pUser->GetLastRecvTime();
		if (deltaTime > _timeoutDisconnectForUser)
		{
			//테스트 중에는 끊지 않는다.
			Disconnect(userPair.first);
		}
	}
	*/

	for (auto& contentsPair : _contentsHashTable)
	{
		Contents* pContents = contentsPair.second;
		InterlockedExchange(&pContents->_prevFrameCnt, pContents->_frameCnt);
		InterlockedExchange(&pContents->_frameCnt, 0);
	}

}

void EchoServer::OnError(INT32 errorCode, WCHAR* comment)
{
}

IUser* EchoServer::GetUser(INT64 sessionID)
{
	AcquireSRWLockShared(&_userContainerLock);
    IUser* pUser = _userContainer[sessionID];
	ReleaseSRWLockShared(&_userContainerLock);
	if (pUser == nullptr)
	{
		__debugbreak();
	}
    return pUser;
}

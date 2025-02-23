#pragma comment(lib, "Winmm.lib")

#include "pch.h"

#include "ChattingServer.h"
#include <Windows.h>
#include "SmartMessagePtr.h"
#include "SystemLog.h"
#include "Message.h"
//#define PROFILE
#include "Profiler.h"
#include "Parser.h"




ChattingServer::ChattingServer()
    :_userPool(0, true), _currentUserNum(0), _updateTPS(0), _prevUpdateTPS(0),
    _hTimerThread(INVALID_HANDLE_VALUE), _isTimerThreadRun(true)
{
    InitializeSRWLock(&_userContainerLock);
    timeBeginPeriod(1);

    for (int i = 0; i < MAX_SECTOR_Y; i++)
    {
        for (int j = 0; j < MAX_SECTOR_X; j++)
        {
            _sectors[i][j]._myPos._x = j;
            _sectors[i][j]._myPos._y = i;
        }
    }

    InitializeSRWLock(&_redisInitLock);
}

ChattingServer::~ChattingServer()
{
    for (auto element : _redisConnections)
    {
        delete element;
    }
    //핸들 정리
    CloseHandle(_hTimerThread);
}



bool ChattingServer::Start(const WCHAR* configFileName)
{
    bool isStart = NetServer::Start(configFileName);
    if (!isStart)
        return false;

    _hTimerThread = TimerThreadStarter();

    if (!LoadChatServerConfig())
    {
        wprintf(L"Fail to Load ChatConfig\n");
    }

    _redisConnections.reserve(_IOCP_WorkerThreadNum);
    _isServerAlive = true;

    SystemLog::GetInstance()->WriteLogConsole(
        SystemLog::LOG_LEVEL_SYSTEM, L"[System] 채팅 서버 스타트 성공...\n");

    return true;
}

void ChattingServer::Stop()
{
    //네트워크 부분부터 먼저 닫아준다.
    NetServer::Stop();

    _isTimerThreadRun = false;

    //접속 중인 모든 User 객체 소멸
    for (auto& userPair : _userContainer)
    {
        User* pUser = userPair.second;
        _userPool.Free(pUser);
    }

    //User 컨테이너 비우기
    _userContainer.clear();

    //섹터 자료구조도 비우기
    for (int i = 0; i < MAX_SECTOR_Y; i++)
    {
        for (int j = 0; j < MAX_SECTOR_X; j++)
        {
            _sectors[i][j].Clear();
        }
    }

    WaitForSingleObject(_hTimerThread, INFINITE);
    _isServerAlive = false;
    wprintf(L"# 컨텐츠 스레드 종료...\n");


    wprintf(L"# 서버 종료.\n");
}

LONG ChattingServer::GetUserPoolCapacity() const
{
    return _userPool.GetCapacityCount();
}

LONG ChattingServer::GetUserPoolSize() const
{
    return _userPool.GetUseCount();
}

LONG ChattingServer::GetUserCount() const
{
    return _currentUserNum;
}

INT64 ChattingServer::GetUpdateTPS() const
{
    return _prevUpdateTPS;
}

bool ChattingServer::GetServerAlive() const
{
    return _isServerAlive;
}

bool ChattingServer::OnConnectionRequest(const wchar_t* connectIP, int connectPort)
{
    return true;
}

void ChattingServer::OnAccept(__int64 sessionID)
{
    //User 풀에서 할당받기
    User* pUser = _userPool.Alloc();
    //_sessionID는 매개변수로 받은 값으로 초기화
    pUser->Init(sessionID);

    //User 객체 컨테이너에 삽입
    ELockUserContainer();
    _userContainer.insert({ sessionID, pUser });
    EUnlockUserContainer();

    InterlockedIncrement64(&_updateTPS);
}

void ChattingServer::OnRelease(__int64 sessionID)
{
    ELockUserContainer();
    // sessionID로 제거할 User 찾기
    User* pUser = _userContainer[sessionID];
    if (pUser == nullptr)
    {
        SystemLog::GetInstance()->WriteLog(L"[ERROR]", SystemLog::LOG_LEVEL_ERROR, L"OnRelease -> User not found\n");
        EUnlockUserContainer();
        return;
    }
    // User 컨테이너에서도 제거
    _userContainer.erase(sessionID);
    EUnlockUserContainer();

    SectorPos userSectorPos = pUser->GetCurSector();

    //User가 SectorMove 패킷을 받아서 Sector에 들어있는지 확인
    bool CheckAboutUserInSector = !(userSectorPos._x == MAX_SECTOR_X && userSectorPos._y == MAX_SECTOR_Y);
    // _Sectors 배열에서 해당 User 객체 찾아서 내부 User 자료구조에서 제거
    if (CheckAboutUserInSector)
    {
        Sector& userSector = _sectors[userSectorPos._y][userSectorPos._x];

        userSector.ELock();
        userSector.Remove(pUser);
        userSector.EUnlock();
    }


    //로그인이 되었던 유저가 접속 종료한 경우라면 현재 유저 수를 차감한다.
    if (pUser->GetLogin())
    {
        InterlockedDecrement(&_currentUserNum);
    }

    // User객체 풀에 반환

    _userPool.Free(pUser);
    InterlockedIncrement64(&_updateTPS);
}

void ChattingServer::OnRecv(__int64 sessionID, Message* pMessage)
{
    WORD type = -1;
    *pMessage >> type;

    switch (type)
    {
    case en_PACKET_CS_CHAT_REQ_LOGIN:
        PacketProc_Login(sessionID, pMessage);
        break;
    case en_PACKET_CS_CHAT_REQ_SECTOR_MOVE:
        PacketProc_SectorMove(sessionID, pMessage);
        break;
    case en_PACKET_CS_CHAT_REQ_MESSAGE:
        PacketProc_Message(sessionID, pMessage);
        break;
    case en_PACKET_CS_CHAT_REQ_HEARTBEAT:
        PacketProc_HeartBeat(sessionID, pMessage);
        break;
    default:
        Disconnect(sessionID);
        break;
    }
    //Recv 처리에서 생성한 메시지 레퍼런스 카운트 내리기
    pMessage->SubRefCount();
    InterlockedIncrement64(&_updateTPS);
}

void ChattingServer::OnSend(__int64 sessionID, int sendSize)
{
}

void ChattingServer::OnError(int errorCode, wchar_t* comment)
{

}

void ChattingServer::OnTime()
{
    SLockUserContainer();
    //모든 User를 순회하며 타임 아웃 여부를 체크하고 타임 아웃이면 연결을 끊는다.
    for (auto& userPair : _userContainer)
    {
        User* pUser = userPair.second;

        UINT64 timeOutValue = pUser->GetLogin() ? _timeoutDisconnectForUser : _timeoutDisconnectForSession;
        ULONGLONG deltaTime = GetTickCount64() - pUser->GetLastRecvTime();

        if (deltaTime > timeOutValue)
        {
            //테스트 중에는 끊지 않는다.
            //Disconnect(userPair.first);
        }
    }
    SUnlockUserContainer();
    InterlockedIncrement64(&_updateTPS);

    //Update TPS 측정 처리
    _prevUpdateTPS = _updateTPS;
    _updateTPS = 0;
}


void ChattingServer::TimerThreadProc()
{
    while (_isTimerThreadRun)
    {
        PQCS(1, 0, (LPOVERLAPPED)TIMER_SIGN_PQCS);
        Sleep(TIMER_INTERVAL);
    }
}

void ChattingServer::PacketProc_Login(INT64 sessionID, Message* pMessage)
{
    //직렬화 버퍼의 내용을 꺼내기
    INT64 accountNo = -1;
    WCHAR id[ID_LEN] = { 0, };
    WCHAR nickName[NICKNAME_LEN] = { 0, };
    char sessionKey[SESSIONKEY_LEN] = { 0, };

    BYTE loginStatus = 1;
    std::string sessionKeyFromRedis;

    if (pMessage->GetDataSize() != sizeof(accountNo) + sizeof(id) + sizeof(nickName) + sizeof(sessionKey))
    {
        Disconnect(sessionID);
        return;
    }

    *pMessage >> accountNo;
    pMessage->GetData((char*)id, ID_LEN * sizeof(WCHAR));
    pMessage->GetData((char*)nickName, NICKNAME_LEN * sizeof(WCHAR));
    pMessage->GetData(sessionKey, SESSIONKEY_LEN);

    SLockUserContainer();

    //세션ID로 User 검색
    User* pUser = _userContainer[sessionID];

    //이미 삭제가 진행되어서 해당 유저가 컨테이너에 없다면 그냥 함수를 리턴한다.
    if (pUser == nullptr)
    {
        SUnlockUserContainer();
        return;
    }

    SUnlockUserContainer();

    pUser->UpdateLastRecvTime();

    //User Max 치를 넘겼다면 로그인 실패로 처리한다.
    if (_numOfMaxUser <= _currentUserNum)
    {
        loginStatus = 0;
    }

    /*
    //////////////////////////
    //      Redis 통신      //
    //////////////////////////
    if (loginStatus == 1)
    {
        //Redis 초기화
        if (_pRedisConnection == nullptr)
        {
            char tokenServerIP[IP_LEN] = { 0, };
            ConvertWcharToChar(_tokenServerIP, tokenServerIP, IP_LEN);

            AcquireSRWLockExclusive(&_redisInitLock);
            _pRedisConnection = new cpp_redis::client();
            _pRedisConnection->connect(tokenServerIP, _tokenServerPort);
            if (!_pRedisConnection->is_connected())
            {
                SystemLog::GetInstance()->WriteLog(L"[ERROR]", SystemLog::LOG_LEVEL_ERROR, L"Redis Connection Fail");
                __debugbreak();
            }
            _redisConnections.push_back(_pRedisConnection);
            ReleaseSRWLockExclusive(&_redisInitLock);
        }

        // 조회
        _pRedisConnection->get(std::to_string(accountNo), [&sessionKeyFromRedis](cpp_redis::reply& reply) {
            if (reply.is_string())
            {
                sessionKeyFromRedis = reply.as_string();
            }
            });
        _pRedisConnection->sync_commit();

        // 항목 지우기
        _pRedisConnection->del({ std::to_string(accountNo) });
        _pRedisConnection->sync_commit();

        //1 : 성공 / 0 : 실패
        loginStatus = (std::string(sessionKey, 64) == sessionKeyFromRedis) ? 1 : 0;
    }

    */
    pUser->SetAccountNo(accountNo);

    //해당 세션에게 로그인 응답 패킷 보내기
    Message* pLoginResponseMsg = Message::Alloc();
    MakePacket_LoginResponse(pLoginResponseMsg, loginStatus, pUser->GetAccountNo());

    if (loginStatus)
    {
        pUser->Login();
        pUser->SetID(id);
        pUser->SetNickName(nickName);
        InterlockedIncrement(&_currentUserNum);
        SendPacket(sessionID, pLoginResponseMsg);
    }
    else
    {
        SendPacket_Disconnect(sessionID, pLoginResponseMsg);
    }
}

void ChattingServer::PacketProc_SectorMove(INT64 sessionID, Message* pMessage)
{
    //직렬화 버퍼의 내용 꺼내기(AccountNo, SectorX, SectorY)
    INT64 accountNo = -1;
    WORD sectorX = MAX_SECTOR_X;
    WORD sectorY = MAX_SECTOR_Y;

    //악의적인 공격 방어
    if (pMessage->GetDataSize() != sizeof(accountNo) + sizeof(sectorX) + sizeof(sectorY))
    {
        Disconnect(sessionID);
        return;
    }

    *pMessage >> accountNo >> sectorX >> sectorY;

    SLockUserContainer();
    //세션ID로 User 검색
    User* pUser = _userContainer[sessionID];
    if (pUser == nullptr)
    {
        SUnlockUserContainer();
        return;
    }
    SUnlockUserContainer();

    pUser->UpdateLastRecvTime();

    //동일 accountNo이 아니라면 결함이기 때문에 __debugbreak()를 한다.
    if (accountNo != pUser->GetAccountNo())
    {
        //__debugbreak();
        Disconnect(sessionID);
        return;
    }

    //유효하지 않은 SectorX, SectorY가?
    if (0 > sectorX && sectorX >= MAX_SECTOR_X ||
        0 > sectorY && sectorY >= MAX_SECTOR_Y)
    {
        Disconnect(sessionID);
        return;
    }

    SectorPos oldUserSectorPos = pUser->GetCurSector();
    Sector& oldUserSector = _sectors[oldUserSectorPos._y][oldUserSectorPos._x];
    //유저 객체의 _curSector 정보를 받은 SectorX와 SectorY로 갱신
    pUser->SetSectorX(sectorX);
    pUser->SetSectorY(sectorY);
    SectorPos curUserSectorPos = pUser->GetCurSector();
    Sector& curUserSector = _sectors[curUserSectorPos._y][curUserSectorPos._x];
    
    //블락되는 코드를 최대한 하단에 두기 위해 여기서 먼저 보낸다
    Message* pSectorMoveResponseMsg = Message::Alloc();
    MakePacket_SectorMoveResponse(pSectorMoveResponseMsg, pUser->GetAccountNo(), curUserSectorPos._x, curUserSectorPos._y);
    SendPacket(sessionID, pSectorMoveResponseMsg, true);


    // x,y의 최대치는 최초 User 객체가 생성되었을 때만 가지는 값이다.
    // 이 때는 _sectors 배열에 반영되지 않았으므로 erase를 하지 않는다.
    bool isFristAllocToSector = oldUserSectorPos._x == MAX_SECTOR_X && oldUserSectorPos._y == MAX_SECTOR_Y;
    if (isFristAllocToSector)
    {
        //최초 섹터에 들어가는 경우 해당 섹터만 락을 걸고 넣어준다.
        curUserSector.ELock();
        curUserSector.PushBack(pUser);
        curUserSector.EUnlock();
    }
    else
    {
        //블락 가능 지점
        MoveSector(oldUserSectorPos, curUserSectorPos, oldUserSector, curUserSector, pUser);
    }
}

void ChattingServer::PacketProc_Message(INT64 sessionID, Message* pMessage)
{
    //직렬화 버퍼의 내용 꺼내기(AccountNo, MessageLen, Message)
    INT64 accountNo = -1;
    WORD messageLen = 0;
    WCHAR message[MAX_MESSAGE_LEN];

    if (pMessage->GetDataSize() < sizeof(accountNo) + sizeof(messageLen))
    {
        Disconnect(sessionID);
        return;
    }

    *pMessage >> accountNo >> messageLen;
    int retNum = pMessage->GetData((char*)message, messageLen);

    if (retNum != messageLen)
    {
        //__debugbreak();
        Disconnect(sessionID);
        return;
    }

    SLockUserContainer();
    //세션ID로 User 검색
    User* pUser = _userContainer[sessionID];
    if (pUser == nullptr)
    {
        SUnlockUserContainer();
        return;
    }
    SUnlockUserContainer();

    pUser->UpdateLastRecvTime();

    //동일 accountNo이 아니라면 결함이기 때문에 __debugbreak()를 한다.
    if (accountNo != pUser->GetAccountNo())
    {
        //__debugbreak();
        Disconnect(sessionID);
        return;
    }

    //해당 User의 주변 섹터 구하기
    SectorAround around;
    pUser->GetCurSector().GetSectorAround(around);


    //모든 주변 섹터의 User에게 채팅 보내기 응답 메시지를 송신한다.
    Message* pMessageResponseMsg = Message::Alloc();
    MakePacket_MessageResponse(pMessageResponseMsg, pUser->GetAccountNo(),
        pUser->GetIDPtr(), pUser->GetNickNamePtr(), messageLen, message);

    SendPacket_Around(pMessageResponseMsg, around);
}

void ChattingServer::PacketProc_HeartBeat(INT64 sessionID, Message* pMessage)
{
    SLockUserContainer();
    //세션ID로 User 검색
    User* pUser = _userContainer[sessionID];
    if (pUser == nullptr)
    {
        SUnlockUserContainer();
        return;
    }
    SUnlockUserContainer();
    //User 객체의 lastRecvTime 정보를 현재 시간으로 갱신
    pUser->UpdateLastRecvTime();
}

void ChattingServer::MakePacket_LoginResponse(Message* pMessage, BYTE status, INT64 accountNo)
{
    *pMessage << (WORD)en_PACKET_CS_CHAT_RES_LOGIN << status << accountNo;
}

void ChattingServer::MakePacket_SectorMoveResponse(Message* pMessage, INT64 accountNo,
    WORD sectorX, WORD sectorY)
{
    *pMessage << (WORD)en_PACKET_CS_CHAT_RES_SECTOR_MOVE << accountNo << sectorX << sectorY;
}

void ChattingServer::MakePacket_MessageResponse(Message* pMessage, INT64 accountNo,
    WCHAR* id, WCHAR* nickName, WORD msgLen, WCHAR* message)
{
    *pMessage << (WORD)en_PACKET_CS_CHAT_RES_MESSAGE << accountNo;
    pMessage->PutData((char*)id, ID_LEN * sizeof(WCHAR));
    pMessage->PutData((char*)nickName, NICKNAME_LEN * sizeof(WCHAR));
    *pMessage << msgLen;
    pMessage->PutData((char*)message, msgLen * sizeof(WCHAR));
}

bool ChattingServer::SendPacket_Around(Message* pMessage, SectorAround& sectorAround)
{
    
    // 직렬화 버퍼가 여러 세션에게 보내는 동안 Free되지 않도록 레퍼런스 카운트를 여기서 증가 시킨다.
    pMessage->AddRefCount();

    SLockAround(sectorAround);
    for (int i = 0; i < sectorAround.count; i++)
    {
        for (auto e : _sectors[sectorAround.around[i]._y][sectorAround.around[i]._x]._users)
        {
            bool checkSend = SendPacket(e->GetSessionID(), pMessage, true);
        }
    }
    SUnlockAround(sectorAround);

    pMessage->SubRefCount();

    return true;
}

void ChattingServer::ELockUserContainer()
{
    AcquireSRWLockExclusive(&_userContainerLock);
}

void ChattingServer::EUnlockUserContainer()
{
    ReleaseSRWLockExclusive(&_userContainerLock);
}

void ChattingServer::SLockUserContainer()
{
    AcquireSRWLockShared(&_userContainerLock);
}

void ChattingServer::SUnlockUserContainer()
{
    ReleaseSRWLockShared(&_userContainerLock);
}

void ChattingServer::MoveSector(SectorPos oldSectorPos, SectorPos curSectorPos, Sector& oldSector, Sector& curSector, User* pUser)
{
    if (oldSectorPos._x < curSectorPos._x && oldSectorPos._y == curSectorPos._y)
	{
		// 오른쪽 이동 상황
        oldSector.ELock();
        curSector.ELock();

        oldSector.Remove(pUser);
		curSector.PushBack(pUser);

		curSector.EUnlock();
		oldSector.EUnlock();
	}
	else if (oldSectorPos._x < curSectorPos._x && oldSectorPos._y < curSectorPos._y)
	{
		//오른쪽 아래 이동 상황
        oldSector.ELock();
        curSector.ELock();

        oldSector.Remove(pUser);
        curSector.PushBack(pUser);

		curSector.EUnlock();
		oldSector.EUnlock();
	}
	else if (oldSectorPos._x == curSectorPos._x && oldSectorPos._y < curSectorPos._y)
	{
		// 아래쪽 이동 상황

        oldSector.ELock();
        curSector.ELock();

        oldSector.Remove(pUser);
        curSector.PushBack(pUser);

		curSector.EUnlock();
		oldSector.EUnlock();
	}
	else if (oldSectorPos._x > curSectorPos._x && oldSectorPos._y < curSectorPos._y)
	{
		// 왼쪽 아래 이동 상황
        oldSector.ELock();
        curSector.ELock();

        oldSector.Remove(pUser);
        curSector.PushBack(pUser);

		curSector.EUnlock();
		oldSector.EUnlock();
	}
	else if (oldSectorPos._x > curSectorPos._x && oldSectorPos._y == curSectorPos._y)
	{
		// 왼쪽 이동 상황
        curSector.ELock();
        oldSector.ELock();

        oldSector.Remove(pUser);
        curSector.PushBack(pUser);

		oldSector.EUnlock();
		curSector.EUnlock();
	}
	else if (oldSectorPos._x > curSectorPos._x && oldSectorPos._y > curSectorPos._y)
	{
		// 왼쪽 위 이동 상황
        curSector.ELock();
        oldSector.ELock();

        oldSector.Remove(pUser);
        curSector.PushBack(pUser);

		oldSector.EUnlock();
		curSector.EUnlock();
	}
	else if (oldSectorPos._x == curSectorPos._x && oldSectorPos._y > curSectorPos._y)
	{
		// 위쪽 이동 상황
        curSector.ELock();
        oldSector.ELock();

        oldSector.Remove(pUser);
        curSector.PushBack(pUser);

		oldSector.EUnlock();
		curSector.EUnlock();
	}
	else if (oldSectorPos._x < curSectorPos._x && oldSectorPos._y > curSectorPos._y)
	{
		// 오른쪽 위 이동 상황
        curSector.ELock();
        oldSector.ELock();

        oldSector.Remove(pUser);
        curSector.PushBack(pUser);

		oldSector.EUnlock();
		curSector.EUnlock();
	}
}

void ChattingServer::ELockAround(SectorAround& sectorAround)
{
    //왼쪽->오른쪽 순서대로 ELock을 걸어준다.(데드락 방지)
    for (int i = 0; i < sectorAround.count; i++)
    {
        _sectors[sectorAround.around[i]._y][sectorAround.around[i]._x].ELock();
    }
}

void ChattingServer::EUnlockAround(SectorAround& sectorAround)
{
    //락을 잡은 역순으로 해제해준다.
    for (int i = sectorAround.count - 1; i >= 0; i--)
    {
        _sectors[sectorAround.around[i]._y][sectorAround.around[i]._x].EUnlock();
    }
}

bool ChattingServer::LoadChatServerConfig()
{
    Parser parser;

    if (!parser.LoadFile(L"ChatServer.cnf"))
    {
        return false;
    }

    parser.GetValue(L"TOKEN_SERVER_IP", _tokenServerIP, IP_LEN);
    parser.GetValue(L"TOKEN_SERVER_PORT", (int*)&_tokenServerPort);

    return true;
}



HANDLE ChattingServer::TimerThreadStarter()
{
    auto threadFunc = [](void* pThis)->unsigned int
        {
            ChattingServer* server = static_cast<ChattingServer*>(pThis);
            server->TimerThreadProc();
            return 0;
        };

    return (HANDLE)_beginthreadex(NULL, 0, threadFunc, this, 0, NULL);
}

void ChattingServer::ConvertWcharToChar(const WCHAR* wideStr, char* charStr, size_t charStrSize)
{
    int bytesWritten = WideCharToMultiByte(CP_UTF8, 0, wideStr, -1, charStr, static_cast<int>(charStrSize), nullptr, nullptr);

    if (bytesWritten == 0)
    {
        wprintf(L"Conversion failed with error: %d\n", GetLastError());
    }
}

void ChattingServer::ConvertCharToWchar(char* charStr, WCHAR* wideStr, size_t wideStrSize)
{
    int bytesWritten = MultiByteToWideChar(CP_UTF8, 0, charStr, -1, wideStr, static_cast<int>(wideStrSize));

    if (bytesWritten == 0)
    {
        wprintf(L"Conversion failed with error: %d\n", GetLastError());
    }
}
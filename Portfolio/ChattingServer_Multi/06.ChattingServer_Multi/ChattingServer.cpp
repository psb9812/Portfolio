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
    //�ڵ� ����
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
        SystemLog::LOG_LEVEL_SYSTEM, L"[System] ä�� ���� ��ŸƮ ����...\n");

    return true;
}

void ChattingServer::Stop()
{
    //��Ʈ��ũ �κк��� ���� �ݾ��ش�.
    NetServer::Stop();

    _isTimerThreadRun = false;

    //���� ���� ��� User ��ü �Ҹ�
    for (auto& userPair : _userContainer)
    {
        User* pUser = userPair.second;
        _userPool.Free(pUser);
    }

    //User �����̳� ����
    _userContainer.clear();

    //���� �ڷᱸ���� ����
    for (int i = 0; i < MAX_SECTOR_Y; i++)
    {
        for (int j = 0; j < MAX_SECTOR_X; j++)
        {
            _sectors[i][j].Clear();
        }
    }

    WaitForSingleObject(_hTimerThread, INFINITE);
    _isServerAlive = false;
    wprintf(L"# ������ ������ ����...\n");


    wprintf(L"# ���� ����.\n");
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
    //User Ǯ���� �Ҵ�ޱ�
    User* pUser = _userPool.Alloc();
    //_sessionID�� �Ű������� ���� ������ �ʱ�ȭ
    pUser->Init(sessionID);

    //User ��ü �����̳ʿ� ����
    ELockUserContainer();
    _userContainer.insert({ sessionID, pUser });
    EUnlockUserContainer();

    InterlockedIncrement64(&_updateTPS);
}

void ChattingServer::OnRelease(__int64 sessionID)
{
    ELockUserContainer();
    // sessionID�� ������ User ã��
    User* pUser = _userContainer[sessionID];
    if (pUser == nullptr)
    {
        SystemLog::GetInstance()->WriteLog(L"[ERROR]", SystemLog::LOG_LEVEL_ERROR, L"OnRelease -> User not found\n");
        EUnlockUserContainer();
        return;
    }
    // User �����̳ʿ����� ����
    _userContainer.erase(sessionID);
    EUnlockUserContainer();

    SectorPos userSectorPos = pUser->GetCurSector();

    //User�� SectorMove ��Ŷ�� �޾Ƽ� Sector�� ����ִ��� Ȯ��
    bool CheckAboutUserInSector = !(userSectorPos._x == MAX_SECTOR_X && userSectorPos._y == MAX_SECTOR_Y);
    // _Sectors �迭���� �ش� User ��ü ã�Ƽ� ���� User �ڷᱸ������ ����
    if (CheckAboutUserInSector)
    {
        Sector& userSector = _sectors[userSectorPos._y][userSectorPos._x];

        userSector.ELock();
        userSector.Remove(pUser);
        userSector.EUnlock();
    }


    //�α����� �Ǿ��� ������ ���� ������ ����� ���� ���� ���� �����Ѵ�.
    if (pUser->GetLogin())
    {
        InterlockedDecrement(&_currentUserNum);
    }

    // User��ü Ǯ�� ��ȯ

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
    //Recv ó������ ������ �޽��� ���۷��� ī��Ʈ ������
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
    //��� User�� ��ȸ�ϸ� Ÿ�� �ƿ� ���θ� üũ�ϰ� Ÿ�� �ƿ��̸� ������ ���´�.
    for (auto& userPair : _userContainer)
    {
        User* pUser = userPair.second;

        UINT64 timeOutValue = pUser->GetLogin() ? _timeoutDisconnectForUser : _timeoutDisconnectForSession;
        ULONGLONG deltaTime = GetTickCount64() - pUser->GetLastRecvTime();

        if (deltaTime > timeOutValue)
        {
            //�׽�Ʈ �߿��� ���� �ʴ´�.
            //Disconnect(userPair.first);
        }
    }
    SUnlockUserContainer();
    InterlockedIncrement64(&_updateTPS);

    //Update TPS ���� ó��
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
    //����ȭ ������ ������ ������
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

    //����ID�� User �˻�
    User* pUser = _userContainer[sessionID];

    //�̹� ������ ����Ǿ �ش� ������ �����̳ʿ� ���ٸ� �׳� �Լ��� �����Ѵ�.
    if (pUser == nullptr)
    {
        SUnlockUserContainer();
        return;
    }

    SUnlockUserContainer();

    pUser->UpdateLastRecvTime();

    //User Max ġ�� �Ѱ�ٸ� �α��� ���з� ó���Ѵ�.
    if (_numOfMaxUser <= _currentUserNum)
    {
        loginStatus = 0;
    }

    /*
    //////////////////////////
    //      Redis ���      //
    //////////////////////////
    if (loginStatus == 1)
    {
        //Redis �ʱ�ȭ
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

        // ��ȸ
        _pRedisConnection->get(std::to_string(accountNo), [&sessionKeyFromRedis](cpp_redis::reply& reply) {
            if (reply.is_string())
            {
                sessionKeyFromRedis = reply.as_string();
            }
            });
        _pRedisConnection->sync_commit();

        // �׸� �����
        _pRedisConnection->del({ std::to_string(accountNo) });
        _pRedisConnection->sync_commit();

        //1 : ���� / 0 : ����
        loginStatus = (std::string(sessionKey, 64) == sessionKeyFromRedis) ? 1 : 0;
    }

    */
    pUser->SetAccountNo(accountNo);

    //�ش� ���ǿ��� �α��� ���� ��Ŷ ������
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
    //����ȭ ������ ���� ������(AccountNo, SectorX, SectorY)
    INT64 accountNo = -1;
    WORD sectorX = MAX_SECTOR_X;
    WORD sectorY = MAX_SECTOR_Y;

    //�������� ���� ���
    if (pMessage->GetDataSize() != sizeof(accountNo) + sizeof(sectorX) + sizeof(sectorY))
    {
        Disconnect(sessionID);
        return;
    }

    *pMessage >> accountNo >> sectorX >> sectorY;

    SLockUserContainer();
    //����ID�� User �˻�
    User* pUser = _userContainer[sessionID];
    if (pUser == nullptr)
    {
        SUnlockUserContainer();
        return;
    }
    SUnlockUserContainer();

    pUser->UpdateLastRecvTime();

    //���� accountNo�� �ƴ϶�� �����̱� ������ __debugbreak()�� �Ѵ�.
    if (accountNo != pUser->GetAccountNo())
    {
        //__debugbreak();
        Disconnect(sessionID);
        return;
    }

    //��ȿ���� ���� SectorX, SectorY��?
    if (0 > sectorX && sectorX >= MAX_SECTOR_X ||
        0 > sectorY && sectorY >= MAX_SECTOR_Y)
    {
        Disconnect(sessionID);
        return;
    }

    SectorPos oldUserSectorPos = pUser->GetCurSector();
    Sector& oldUserSector = _sectors[oldUserSectorPos._y][oldUserSectorPos._x];
    //���� ��ü�� _curSector ������ ���� SectorX�� SectorY�� ����
    pUser->SetSectorX(sectorX);
    pUser->SetSectorY(sectorY);
    SectorPos curUserSectorPos = pUser->GetCurSector();
    Sector& curUserSector = _sectors[curUserSectorPos._y][curUserSectorPos._x];
    
    //����Ǵ� �ڵ带 �ִ��� �ϴܿ� �α� ���� ���⼭ ���� ������
    Message* pSectorMoveResponseMsg = Message::Alloc();
    MakePacket_SectorMoveResponse(pSectorMoveResponseMsg, pUser->GetAccountNo(), curUserSectorPos._x, curUserSectorPos._y);
    SendPacket(sessionID, pSectorMoveResponseMsg, true);


    // x,y�� �ִ�ġ�� ���� User ��ü�� �����Ǿ��� ���� ������ ���̴�.
    // �� ���� _sectors �迭�� �ݿ����� �ʾ����Ƿ� erase�� ���� �ʴ´�.
    bool isFristAllocToSector = oldUserSectorPos._x == MAX_SECTOR_X && oldUserSectorPos._y == MAX_SECTOR_Y;
    if (isFristAllocToSector)
    {
        //���� ���Ϳ� ���� ��� �ش� ���͸� ���� �ɰ� �־��ش�.
        curUserSector.ELock();
        curUserSector.PushBack(pUser);
        curUserSector.EUnlock();
    }
    else
    {
        //��� ���� ����
        MoveSector(oldUserSectorPos, curUserSectorPos, oldUserSector, curUserSector, pUser);
    }
}

void ChattingServer::PacketProc_Message(INT64 sessionID, Message* pMessage)
{
    //����ȭ ������ ���� ������(AccountNo, MessageLen, Message)
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
    //����ID�� User �˻�
    User* pUser = _userContainer[sessionID];
    if (pUser == nullptr)
    {
        SUnlockUserContainer();
        return;
    }
    SUnlockUserContainer();

    pUser->UpdateLastRecvTime();

    //���� accountNo�� �ƴ϶�� �����̱� ������ __debugbreak()�� �Ѵ�.
    if (accountNo != pUser->GetAccountNo())
    {
        //__debugbreak();
        Disconnect(sessionID);
        return;
    }

    //�ش� User�� �ֺ� ���� ���ϱ�
    SectorAround around;
    pUser->GetCurSector().GetSectorAround(around);


    //��� �ֺ� ������ User���� ä�� ������ ���� �޽����� �۽��Ѵ�.
    Message* pMessageResponseMsg = Message::Alloc();
    MakePacket_MessageResponse(pMessageResponseMsg, pUser->GetAccountNo(),
        pUser->GetIDPtr(), pUser->GetNickNamePtr(), messageLen, message);

    SendPacket_Around(pMessageResponseMsg, around);
}

void ChattingServer::PacketProc_HeartBeat(INT64 sessionID, Message* pMessage)
{
    SLockUserContainer();
    //����ID�� User �˻�
    User* pUser = _userContainer[sessionID];
    if (pUser == nullptr)
    {
        SUnlockUserContainer();
        return;
    }
    SUnlockUserContainer();
    //User ��ü�� lastRecvTime ������ ���� �ð����� ����
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
    
    // ����ȭ ���۰� ���� ���ǿ��� ������ ���� Free���� �ʵ��� ���۷��� ī��Ʈ�� ���⼭ ���� ��Ų��.
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
		// ������ �̵� ��Ȳ
        oldSector.ELock();
        curSector.ELock();

        oldSector.Remove(pUser);
		curSector.PushBack(pUser);

		curSector.EUnlock();
		oldSector.EUnlock();
	}
	else if (oldSectorPos._x < curSectorPos._x && oldSectorPos._y < curSectorPos._y)
	{
		//������ �Ʒ� �̵� ��Ȳ
        oldSector.ELock();
        curSector.ELock();

        oldSector.Remove(pUser);
        curSector.PushBack(pUser);

		curSector.EUnlock();
		oldSector.EUnlock();
	}
	else if (oldSectorPos._x == curSectorPos._x && oldSectorPos._y < curSectorPos._y)
	{
		// �Ʒ��� �̵� ��Ȳ

        oldSector.ELock();
        curSector.ELock();

        oldSector.Remove(pUser);
        curSector.PushBack(pUser);

		curSector.EUnlock();
		oldSector.EUnlock();
	}
	else if (oldSectorPos._x > curSectorPos._x && oldSectorPos._y < curSectorPos._y)
	{
		// ���� �Ʒ� �̵� ��Ȳ
        oldSector.ELock();
        curSector.ELock();

        oldSector.Remove(pUser);
        curSector.PushBack(pUser);

		curSector.EUnlock();
		oldSector.EUnlock();
	}
	else if (oldSectorPos._x > curSectorPos._x && oldSectorPos._y == curSectorPos._y)
	{
		// ���� �̵� ��Ȳ
        curSector.ELock();
        oldSector.ELock();

        oldSector.Remove(pUser);
        curSector.PushBack(pUser);

		oldSector.EUnlock();
		curSector.EUnlock();
	}
	else if (oldSectorPos._x > curSectorPos._x && oldSectorPos._y > curSectorPos._y)
	{
		// ���� �� �̵� ��Ȳ
        curSector.ELock();
        oldSector.ELock();

        oldSector.Remove(pUser);
        curSector.PushBack(pUser);

		oldSector.EUnlock();
		curSector.EUnlock();
	}
	else if (oldSectorPos._x == curSectorPos._x && oldSectorPos._y > curSectorPos._y)
	{
		// ���� �̵� ��Ȳ
        curSector.ELock();
        oldSector.ELock();

        oldSector.Remove(pUser);
        curSector.PushBack(pUser);

		oldSector.EUnlock();
		curSector.EUnlock();
	}
	else if (oldSectorPos._x < curSectorPos._x && oldSectorPos._y > curSectorPos._y)
	{
		// ������ �� �̵� ��Ȳ
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
    //����->������ ������� ELock�� �ɾ��ش�.(����� ����)
    for (int i = 0; i < sectorAround.count; i++)
    {
        _sectors[sectorAround.around[i]._y][sectorAround.around[i]._x].ELock();
    }
}

void ChattingServer::EUnlockAround(SectorAround& sectorAround)
{
    //���� ���� �������� �������ش�.
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
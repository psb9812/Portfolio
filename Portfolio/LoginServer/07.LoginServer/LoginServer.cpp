#include "pch.h"
#include "LoginServer.h"
#include "SystemLog.h"
#include "Message.h"
#include "CommonDefine.h"
#include "Parser.h"
#include "DBConnection.h"

LoginServer::LoginServer()
	:_currentUserNum(0), _userPool(0, true), _isTimerThreadRun(true),
    _hTimerThread(INVALID_HANDLE_VALUE),
    _loginAttemptCountPerSec(0), _loginWaitSessionNum(0), _loginCompleteCountPerSec(0),
    _prevLoginAttemptCountPerSec(0), _prevLoginCompleteCountPerSec(0)
{
    InitializeSRWLock(&_userContainerLock);
    InitializeSRWLock(&_DBInitLock);
    InitializeSRWLock(&_redisInitLock);
}

LoginServer::~LoginServer()
{
    //DB�� Redis �Ҵ� ����
    for (auto element : _DBConnections)
    {
        delete element;
    }
    for (auto element : _redisConnections)
    {
        delete element;
    }

    //�ڵ� ����
    CloseHandle(_hTimerThread);
}

bool LoginServer::Start(const WCHAR* configFileName)
{
    bool isStart = NetServer::Start(configFileName);
    if (!isStart)
        return false;

    _hTimerThread = TimerThreadStarter();

    if (!LoadLoginServerConfig())
    {
        wprintf(L"[ERROR] �α��� ���� Config Fail\n");
    }

    _DBConnections.reserve(_IOCP_WorkerThreadNum);
    _redisConnections.reserve(_IOCP_WorkerThreadNum);
    _isServerAlive = true;

    wprintf(L"DB IP : %s / DB Port : %d\nRedis IP : %s / Redis Port : %d\n",
        _DBServerIP, _DBServerPort, _tokenServerIP, _tokenServerPort);

    wprintf(L"[System] �α��� ���� ��ŸƮ ����...\n");

    return true;
}

void LoginServer::Stop()
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

    WaitForSingleObject(_hTimerThread, INFINITE);
    _isServerAlive = false;
    wprintf(L"# ������ ������ ����...\n");


    wprintf(L"# ���� ����.\n");
}

int LoginServer::GetUserPoolCapacity() const
{
    return _userPool.GetCapacityCount();
}

int LoginServer::GetUserPoolSize() const
{
    return _userPool.GetUseCount();
}

int LoginServer::GetUserCount() const
{
    return _userContainer.size();
}

INT64 LoginServer::GetLoginAttemptCountPerSec() const
{
    return _prevLoginAttemptCountPerSec;
}

INT64 LoginServer::GetLoginWaitSessionNum() const
{
    return _loginWaitSessionNum;
}

INT64 LoginServer::GetLoginCompleteCountPerSec() const
{
    return _prevLoginCompleteCountPerSec;
}

bool LoginServer::GetServerAlive() const
{
    return _isServerAlive;
}

bool LoginServer::OnConnectionRequest(const wchar_t* connectIP, int connectPort)
{
	return true;
}

void LoginServer::OnAccept(__int64 sessionID)
{
    //User Ǯ���� �Ҵ�ޱ�
    User* pUser = _userPool.Alloc();
    //_sessionID�� �Ű������� ���� ������ �ʱ�ȭ
    pUser->Init(sessionID);

    //User ��ü �����̳ʿ� ����
    ELockUserContainer();
    _userContainer.insert({ sessionID, pUser });
    EUnlockUserContainer();

    InterlockedIncrement(&_currentUserNum);
}

void LoginServer::OnRelease(__int64 sessionID)
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

    InterlockedDecrement(&_currentUserNum);

    // User��ü Ǯ�� ��ȯ
    _userPool.Free(pUser);
}

void LoginServer::OnRecv(__int64 sessionID, Message* pMessage)
{
    WORD type = -1;
    *pMessage >> type;

    switch (type)
    {
    case en_PACKET_CS_LOGIN_REQ_LOGIN:
        PacketProc_Login(sessionID, pMessage);
        break;
    default:
        Disconnect(sessionID);
        break;
    }
    //Recv ó������ ������ �޽��� ���۷��� ī��Ʈ ������
    pMessage->SubRefCount();
}

void LoginServer::OnSend(__int64 sessionID, int sendSize)
{
}

void LoginServer::OnError(int errorCode, wchar_t* comment)
{
}

void LoginServer::OnTime()
{
    //1�ʿ� �� �� Ÿ�� �ƿ� üũ
    SLockUserContainer();
    //��� User�� ��ȸ�ϸ� Ÿ�� �ƿ� ���θ� üũ�ϰ� Ÿ�� �ƿ��̸� ������ ���´�.
    for (auto& userPair : _userContainer)
    {
        User* pUser = userPair.second;

        UINT64 timeOutValue = _timeoutDisconnectForSession;
        ULONGLONG deltaTime = GetTickCount64() - pUser->GetLastRecvTime();

        if (deltaTime > timeOutValue)
        {
            //�׽�Ʈ �߿��� ���� �ʴ´�.
            //Disconnect(userPair.first);
        }
    }
    SUnlockUserContainer();

    //����͸� ���� ����
    InterlockedExchange64(&_prevLoginAttemptCountPerSec, _loginAttemptCountPerSec);
    InterlockedExchange64(&_loginAttemptCountPerSec, 0);
    InterlockedExchange64(&_prevLoginCompleteCountPerSec, _loginCompleteCountPerSec);
    InterlockedExchange64(&_loginCompleteCountPerSec, 0);
}

bool LoginServer::LoadLoginServerConfig()
{
    Parser parser;

    if (!parser.LoadFile(L"LoginServer.cnf"))
    {
        return false;
    }

    parser.GetValue(L"GAME_SERVER_IP", _gameServerIP, IP_LEN);
    parser.GetValue(L"GAME_SERVER_PORT", (int*) &_gameServerPort);

    parser.GetValue(L"CHAT_SERVER_IP", _chatServerIP, IP_LEN);
    parser.GetValue(L"CHAT_SERVER_PORT", (int*) &_chatServerPort);

    parser.GetValue(L"DB_SERVER_IP", _DBServerIP, IP_LEN);
    parser.GetValue(L"DB_SERVER_PORT", (int*) &_DBServerPort);

    parser.GetValue(L"TOKEN_SERVER_IP", _tokenServerIP, IP_LEN);
    parser.GetValue(L"TOKEN_SERVER_PORT", (int*) &_tokenServerPort);

    parser.GetValue(L"DB_USER_NAME", _DBUserName, DB_CONFIG_LEN);
    parser.GetValue(L"DB_PASSWORD", _DBPassword, DB_CONFIG_LEN);
    parser.GetValue(L"DB_SCHEMA_NAME", _DBSchemaName, DB_CONFIG_LEN);

    return true;
}

void LoginServer::ConvertWcharToChar(const WCHAR* wideStr, char* charStr, size_t charStrSize)
{
    int bytesWritten = WideCharToMultiByte(CP_UTF8, 0, wideStr, -1, charStr, static_cast<int>(charStrSize), nullptr, nullptr);

    if (bytesWritten == 0) 
    {
        wprintf(L"Conversion failed with error: %d\n", GetLastError());
    }
}

void LoginServer::ConvertCharToWchar(char* charStr, WCHAR* wideStr, size_t wideStrSize)
{
    int bytesWritten = MultiByteToWideChar(CP_UTF8, 0, charStr, -1, wideStr, static_cast<int>(wideStrSize));

    if (bytesWritten == 0)
    {
        wprintf(L"Conversion failed with error: %d\n", GetLastError());
    }
}

HANDLE LoginServer::TimerThreadStarter()
{
    auto threadFunc = [](void* pThis)->unsigned int
        {
            LoginServer* server = static_cast<LoginServer*>(pThis);
            server->TimerThreadProc();
            return 0;
        };

    return (HANDLE)_beginthreadex(NULL, 0, threadFunc, this, 0, NULL);
}

void LoginServer::TimerThreadProc()
{
    while (_isTimerThreadRun)
    {
        PQCS(1, 0, reinterpret_cast<LPOVERLAPPED>(TIMER_SIGN_PQCS));
        Sleep(TIMER_INTERVAL - 5);
    }
}

void LoginServer::PacketProc_Login(INT64 sessionID, Message* pMessage)
{
    if (pMessage->GetDataSize() < sizeof(INT64) + sizeof(char) * 64)
    {
        Disconnect(sessionID);
    }

    INT64 accountNo = -1;
    char sessionKey[64] = { 0, };

    WCHAR id[ID_LEN] = { 0, };
    WCHAR nickname[NICKNAME_LEN] = { 0, };

    //����ȭ ������ �� ��������
    *pMessage >> accountNo;
    pMessage->GetData(sessionKey, sizeof(sessionKey));

    User* pUser = _userContainer[sessionID];
    if (pUser == nullptr)
    {
        return;
    }

    pUser->UpdateLastRecvTime();

    InterlockedIncrement64(&_loginAttemptCountPerSec);
    InterlockedIncrement64(&_loginWaitSessionNum);

    char status = en_PACKET_CS_LOGIN_RES_LOGIN::dfLOGIN_STATUS_NONE;

    //TODO : DB �����ؼ� ��ȿ�� �α������� Ȯ��
    
    //TLS�� �ִ� DBConnection �ʱ�ȭ
    if (_pDBConnection == nullptr)
    {
        //WCHAR -> CHAR ��ȯ
        char DBServerIP[IP_LEN] = { 0, };
        char DBUserName[DB_CONFIG_LEN] = { 0, };
        char DBPassword[DB_CONFIG_LEN] = { 0, };
        char DBSchemaName[DB_CONFIG_LEN] = { 0, };
        ConvertWcharToChar(_DBServerIP, DBServerIP, sizeof(DBServerIP));
        ConvertWcharToChar(_DBUserName, DBUserName, sizeof(DBUserName));
        ConvertWcharToChar(_DBPassword, DBPassword, sizeof(DBPassword));
        ConvertWcharToChar(_DBSchemaName, DBSchemaName, sizeof(DBSchemaName));

        AcquireSRWLockExclusive(&_DBInitLock);
        _pDBConnection = new DBConnection(DBServerIP, DBUserName, DBPassword, DBSchemaName, _DBServerPort);
        _DBConnections.push_back(_pDBConnection);
        ReleaseSRWLockExclusive(&_DBInitLock);
    }

    // Ŭ���̾�Ʈ ��û ��Ŷ�� accountNo�� accountDB���� userid, usernick�� ��ȸ�Ѵ�.
    {
        std::string selectQuery = "SELECT `userid`, `usernick` FROM `account` WHERE `accountno` = " + std::to_string(accountNo) + ";";
        int query_stat = mysql_query(_pDBConnection->GetConnection(), selectQuery.c_str());
        if (query_stat != 0)
        {
            printf("Mysql query error : %s", mysql_error(_pDBConnection->GetConnection()));
            return;
        }

        MYSQL_RES* sql_result = mysql_store_result(_pDBConnection->GetConnection());

        MYSQL_ROW sql_row;
        if ((sql_row = mysql_fetch_row(sql_result)) != NULL)
        {
            ConvertCharToWchar(sql_row[0], id, sizeof(id));
            ConvertCharToWchar(sql_row[1], nickname, sizeof(nickname));
        }
        mysql_free_result(sql_result);
    }

    // Redis �ʱ�ȭ
    if (_pRedisConnection == nullptr)
    {
        char tokenServerIP[IP_LEN] = { 0, };
        ConvertWcharToChar(_tokenServerIP, tokenServerIP, IP_LEN);

        AcquireSRWLockExclusive(&_redisInitLock);
        _pRedisConnection = new cpp_redis::client();
        _pRedisConnection->connect(tokenServerIP, _tokenServerPort);
        _redisConnections.push_back(_pRedisConnection);
        ReleaseSRWLockExclusive(&_redisInitLock);
    }

    // �α��� ����
    status = en_PACKET_CS_LOGIN_RES_LOGIN::dfLOGIN_STATUS_OK;

    //Redis�� accountno - token ����
    std::string sessionKey_str(sessionKey, 64);
    _pRedisConnection->set(std::to_string(accountNo), sessionKey_str);
    _pRedisConnection->expire(std::to_string(accountNo), 10);
    _pRedisConnection->sync_commit();

    Message* pResLoginMessage = Message::Alloc();

    MakePacket_LoginResponse(
        pResLoginMessage, accountNo, status,
        id, nickname, 
        _gameServerIP, _gameServerPort,
        _chatServerIP, _chatServerPort
    );

    SendPacket(sessionID, pResLoginMessage, false);

    InterlockedDecrement64(&_loginWaitSessionNum);
    InterlockedIncrement64(&_loginCompleteCountPerSec);
}

void LoginServer::MakePacket_LoginResponse(Message* pMessage, INT64 accountNo, BYTE status, WCHAR* ID, WCHAR* nickname, WCHAR* gameServerIP, USHORT gameServerPort, WCHAR* chatServerIP, USHORT chatServerPort)
{
    *pMessage << (WORD)en_PACKET_CS_LOGIN_RES_LOGIN << accountNo << status;
    pMessage->PutData((char*)ID, ID_LEN * sizeof(WCHAR));
    pMessage->PutData((char*)nickname, NICKNAME_LEN * sizeof(WCHAR));
    pMessage->PutData((char*)gameServerIP, IP_LEN * sizeof(WCHAR));
    *pMessage << gameServerPort;
    pMessage->PutData((char*)chatServerIP, IP_LEN * sizeof(WCHAR));
    *pMessage << chatServerPort;
}

void LoginServer::ELockUserContainer()
{
    AcquireSRWLockExclusive(&_userContainerLock);
}

void LoginServer::EUnlockUserContainer()
{
    ReleaseSRWLockExclusive(&_userContainerLock);
}

void LoginServer::SLockUserContainer()
{
    AcquireSRWLockShared(&_userContainerLock);
}

void LoginServer::SUnlockUserContainer()
{
    ReleaseSRWLockShared(&_userContainerLock);
}

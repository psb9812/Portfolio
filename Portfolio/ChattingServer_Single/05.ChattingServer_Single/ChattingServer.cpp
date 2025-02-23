#pragma comment(lib, "Winmm.lib")

#include <process.h>
#include "ChattingServer.h"
#include <Windows.h>
#include "SmartMessagePtr.h"
#include "SystemLog.h"
#include "Message.h"
#include "../../CommonProtocol.h"



ChattingServer::ChattingServer()
    :_jobPool(10, true), _jobQueue(100000), _userPool(15000, false), _currentUserNum(0), _updateTPS(0), _prevUpdateTPS(0),
    _hLogicThread(INVALID_HANDLE_VALUE), _isLogicThreadRun(true),
    _hTimerThread(INVALID_HANDLE_VALUE), _isTimerThreadRun(true)
{
    _jobQueueEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
}

ChattingServer::~ChattingServer()
{
    //�ڵ� ����
    CloseHandle(_jobQueueEvent);
    CloseHandle(_hLogicThread);
    CloseHandle(_hTimerThread);
}



bool ChattingServer::Start()
{
    bool isStart = NetServer::Start();
    if (!isStart)
        return false;

    _hLogicThread = LogicThreadStarter();
    _hTimerThread = TimerThreadStarter();

    _isServerAlive = true;
    wprintf(L"[System] ä�� ���� ��ŸƮ ����...\n");

    return true;
}

void ChattingServer::Stop()
{
    //��Ʈ��ũ �κк��� ���� �ݾ��ش�.
    NetServer::Stop();  

    _isLogicThreadRun = false;
    _isTimerThreadRun = false;

    //���� �����带 �����Ű�� ���� Terminate Job�� Job Queue�� �־��ְ� �̺�Ʈ �ñ׳��� �����Ѵ�.
    Job* pJobForTerminate = _jobPool.Alloc();
    pJobForTerminate->Set(en_JobType::JOB_TERMINATE, NULL, NULL);
    _jobQueue.Enqueue(pJobForTerminate);
    SetEvent(_jobQueueEvent);

    WaitForSingleObject(_hLogicThread, INFINITE);
    WaitForSingleObject(_hTimerThread, INFINITE);
    wprintf(L"# ������ ������ ����...\n");

    _isServerAlive = false;

    wprintf(L"# ���� ����.\n");
}

LONG ChattingServer::GetJobQueueSize() const
{
    return _jobQueue.GetSize();
}

int ChattingServer::GetJobPoolSize() const
{
    return _jobPool.GetUseCount();
}

int ChattingServer::GetJobPoolCapacity() const
{
    return _jobPool.GetCapacityCount();
}

int ChattingServer::GetUserPoolCapacity() const
{
    return _userPool.GetCapacityCount();
}

int ChattingServer::GetUserPoolSize() const
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
    //Enqueue accept job
    Job* acceptJob = _jobPool.Alloc();
    acceptJob->Set(en_JobType::JOB_ACCEPT, sessionID, nullptr);
    std::optional<LONG> jobQueueOptional = _jobQueue.Enqueue(acceptJob);
    if (jobQueueOptional.has_value())
    {
        //Enqueue�ؼ� ������ size�� 0���� 1�� �� ��쿡�� �̺�Ʈ �ñ׳��� �����ϹǷ� Ŀ�� ��ȯ�� ���δ�.
        if (jobQueueOptional.value() == 1)
        {
            SetEvent(_jobQueueEvent);
        }
    }
    else
    {
        SystemLog::GetInstance()->WriteLog(L"ERROR", SystemLog::LOG_LEVEL_ERROR, L"��ť�� �� á��.");
        //��ť�� ���� á���Ƿ� ���������� ��Ȳ�̴�.
        //TODO: ������ ����ϰ� �����Ų��.
    }
}

void ChattingServer::OnRelease(__int64 sessionID)
{
    //Enqueue release job
    Job* releaseJob = _jobPool.Alloc();
    releaseJob->Set(en_JobType::JOB_RELEASE, sessionID, nullptr);
    std::optional<LONG> jobQueueOptional = _jobQueue.Enqueue(releaseJob);
    if (jobQueueOptional.has_value())
    {
        //Enqueue�ؼ� ������ size�� 0���� 1�� �� ��쿡�� �̺�Ʈ �ñ׳��� �����ϹǷ� Ŀ�� ��ȯ�� ���δ�.
        if (jobQueueOptional.value() == 1)
        {
            SetEvent(_jobQueueEvent);
        }
    }
    else
    {
        SystemLog::GetInstance()->WriteLog(L"ERROR", SystemLog::LOG_LEVEL_ERROR, L"��ť�� �� á��.");
        //��ť�� ���� á���Ƿ� ���������� ��Ȳ�̴�.
        //TODO: ������ ����ϰ� �����Ų��.
    }
}

void ChattingServer::OnRecv(__int64 sessionID, Message* pMessage)
{
    //Enqueue recv job
    Job* recvJob = _jobPool.Alloc();
    recvJob->Set(en_JobType::JOB_RECV, sessionID, pMessage);
    std::optional<LONG> jobQueueOptional = _jobQueue.Enqueue(recvJob);
    if (jobQueueOptional.has_value())
    {
        //Enqueue�ؼ� ������ size�� 0���� 1�� �� ��쿡�� �̺�Ʈ �ñ׳��� �����ϹǷ� Ŀ�� ��ȯ�� ���δ�.
        if (jobQueueOptional.value() == 1)
        {
            SetEvent(_jobQueueEvent);
        }
    }
    else
    {
        SystemLog::GetInstance()->WriteLog(L"ERROR", SystemLog::LOG_LEVEL_ERROR, L"��ť�� �� á��.");
        //��ť�� ���� á���Ƿ� ���������� ��Ȳ�̴�.
        //TODO: ������ ����ϰ� �����Ų��.
    }
}

void ChattingServer::OnSend(__int64 sessionID, int sendSize)
{
}

void ChattingServer::OnError(int errorCode, wchar_t* comment)
{

}

void ChattingServer::LogicThreadProc()
{
    while (_isLogicThreadRun)
    {
        WaitForSingleObject(_jobQueueEvent, INFINITE);

        //��ť�� �� ������ ��ť���ؼ� ó���Ѵ�.
        while (true)
        {
            std::optional<Job*> optionalOfDequeue;
            if (_jobQueue.GetSize() > 0)
            {
                optionalOfDequeue = _jobQueue.Dequeue();
            }
            if (!optionalOfDequeue.has_value())
            {
                break;
            }

            Job* pJob = optionalOfDequeue.value();

            switch (pJob->_type)
            {
            case en_JobType::JOB_ACCEPT:
                HandleAcceptJob(pJob->_sessionID);
                break;
            case en_JobType::JOB_RELEASE:
                HandleReleaseJob(pJob->_sessionID);
                break;
            case en_JobType::JOB_RECV:
                HandleRecvJob(pJob->_sessionID, pJob->_pMessage);
                break;
            case en_JobType::JOB_TIMER:
                HandleTimerJob();
                break;
            case en_JobType::JOB_TERMINATE:
                HandleTerminateJob();
                break;
            default:
                SystemLog::GetInstance()->WriteLog(L"ERROR", SystemLog::LOG_LEVEL_ERROR,
                    L"Unknown Job Type");
                break;
            }
            _updateTPS++;
            _jobPool.Free(pJob);
        }
    }
}

void ChattingServer::TimerThreadProc()
{
    while (_isTimerThreadRun)
    {
        //1�ʿ� �� ���� TimerJob�� Job Queue�� �־��ش�.
        Job* timerJob = _jobPool.Alloc();
        timerJob->Set(en_JobType::JOB_TIMER, NULL, nullptr);
        std::optional<LONG> jobQueueOptional = _jobQueue.Enqueue(timerJob);
        if (jobQueueOptional.has_value())
        {
            //Enqueue�ؼ� ������ size�� 0���� 1�� �� ��쿡�� �̺�Ʈ �ñ׳��� �����ϹǷ� Ŀ�� ��ȯ�� ���δ�.
            if (jobQueueOptional.value() == 1)
            {
                SetEvent(_jobQueueEvent);
            }
        }
        else
        {
            //��ť�� ���� á���Ƿ� ���������� ��Ȳ�̴�.
            //TODO: ������ ����ϰ� �����Ų��.
        }

        //Update TPS ���� ó��
        _prevUpdateTPS = _updateTPS;
        _updateTPS = 0;
        Sleep(TIMER_INTERVAL);
    }
}

void ChattingServer::HandleAcceptJob(__int64 sessionID)
{
    //User Ǯ���� �Ҵ�ޱ�
    User* pUser = _userPool.Alloc();
    //_sessionID�� �Ű������� ���� ������ �ʱ�ȭ
    pUser->Init(sessionID);
    //User ��ü �����̳ʿ� ����
    _userContainer.insert({ sessionID, pUser });
}

void ChattingServer::HandleReleaseJob(__int64 sessionID)
{
    // sessionID�� ������ User ã��
    User* pUser = _userContainer[sessionID];
    SectorPos userSector = pUser->GetCurSector();

    //User�� SectorMove ��Ŷ�� �޾Ƽ� Sector�� ����ִ��� Ȯ��
    bool CheckAboutUserInSector = !(userSector._x == MAX_SECTOR_X && userSector._y == MAX_SECTOR_Y);
    // _Sectors �迭���� �ش� User ��ü ã�Ƽ� ���� User unordered_map���� ����
    if(CheckAboutUserInSector)
    {
        _sectors[userSector._y][userSector._x]._users.remove(pUser);
    }
    // User �����̳ʿ����� ����
    _userContainer.erase(sessionID);

    if (pUser->GetLogin())
    {
        _currentUserNum--;
    }
    // User��ü Ǯ�� ��ȯ
    _userPool.Free(pUser);

}

void ChattingServer::HandleRecvJob(__int64 sessionID, Message* pMessage)
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
    case en_PACKET_CS_CHAT_REQ_USER_DISTRIBUTION:
        PacketProc_UserDistribution(sessionID);
        break;
    default:
        Disconnect(sessionID);
        break;
    }
    //Recv ó������ ������ �޽��� ���۷��� ī��Ʈ ������
    int ret = pMessage->SubRefCount();
    if (ret != -1)
    {
        wprintf(L"���� �޽��� ���� �� ��.\n");
        SystemLog::GetInstance()->WriteLog(L"ERROR", SystemLog::LOG_LEVEL_ERROR, L"���� �޽��� ���� �� ��");
    }
}

void ChattingServer::HandleTimerJob()
{
    //��� User�� ��ȸ�ϸ� Ÿ�� �ƿ� ���θ� üũ�ϰ� Ÿ�� �ƿ��̸� ������ ���´�.
    for (auto& userPair : _userContainer)
    {
        User* pUser = userPair.second;
        UINT64 timeOutValue = pUser->GetLogin() ? _timeoutDisconnectForUser : _timeoutDisconnectForSession;
        ULONGLONG deltaTime = GetTickCount64() - pUser->GetLastRecvTime();
        if (deltaTime > _timeoutDisconnectForUser)
        {
            //�׽�Ʈ �߿��� ���� �ʴ´�.
            //Disconnect(userPair.first);
        }
    }
}

void ChattingServer::HandleTerminateJob()
{
    //���� ���� ��� User ��ü �Ҹ�
    for (auto userPair : _userContainer)
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
            _sectors[i][j]._users.clear();
        }
    }
}

void ChattingServer::PacketProc_Login(INT64 sessionID, Message* pMessage)
{
    //����ȭ ������ ������ ������
    INT64 accountNo = -1;
    WCHAR id[ID_LEN] = { 0, };
    WCHAR nickName[NICKNAME_LEN] = { 0, };
    char sessionKey[SESSIONKEY_LEN] = { 0, };

    //�������� ���� ���
    if (pMessage->GetDataSize() != sizeof(accountNo) + sizeof(id) + sizeof(nickName) + sizeof(sessionKey))
    {
        Disconnect(sessionID);
        return;
    }

    *pMessage >> accountNo;
    pMessage->GetData((char*)id, ID_LEN * sizeof(WCHAR));
    pMessage->GetData((char*)nickName, NICKNAME_LEN * sizeof(WCHAR));
    pMessage->GetData(sessionKey, SESSIONKEY_LEN);

    //����ID�� User �˻�
    User* pUser = _userContainer[sessionID];
    pUser->UpdateLastRecvTime();

    //���� ��ū�� ���� �α��� ���������� ��ģ��.
    //���� �α��� ������ ������ �ʾ����Ƿ� ������ �α��� �������� ����

    BYTE loginStatus = 1; //1 : ���� / 0 : ����

    //User Max ġ�� �Ѱ�ٸ� �α��� ���з� ó���Ѵ�.
    if (_numOfMaxUser <= _currentUserNum)
    {
        loginStatus = 0;
    }
    
    if (loginStatus)
    {
        pUser->Login();
        pUser->SetAccountNo(accountNo);
        pUser->SetID(id);
        pUser->SetNickName(nickName);
        _currentUserNum++;
    }
    else
    {
        Disconnect(sessionID);
    }

    //�ش� ���ǿ��� �α��� ���� ��Ŷ ������
    Message* pLoginResponseMsg = Message::Alloc();
    MakePacket_LoginResponse(pLoginResponseMsg, loginStatus, pUser->GetAccountNo());
    SendPacket(sessionID, pLoginResponseMsg);
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

    //����ID�� User �˻�
    User* pUser = _userContainer[sessionID];
    pUser->UpdateLastRecvTime();

    if (accountNo != pUser->GetAccountNo())
    {
        //SystemLog::GetInstance()->WriteLog(L"ERROR", SystemLog::LOG_LEVEL_ERROR, L"accountNO ����ġ.");
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

    SectorPos oldUserSector = pUser->GetCurSector();
    // x,y�� �ִ�ġ�� ���� User ��ü�� �����Ǿ��� ���� ������ ���̴�.
    // �� ���� _sectors �迭�� �ݿ����� �ʾ����Ƿ� erase�� ���� �ʴ´�.
    bool isFristAllocToSector = oldUserSector._x == MAX_SECTOR_X && oldUserSector._y == MAX_SECTOR_Y;
    if (!(isFristAllocToSector))
    {
        //���� ��ü�� _curSector ������ _Sectors���� �ش� ��ǥ�� ���Ϳ� ���� unordered_map���� �ڽ� ����
        _sectors[oldUserSector._y][oldUserSector._x]._users.remove(pUser);
    }

    //���� ��ü�� _curSector ������ ���� SectorX�� SectorY�� ����
    pUser->SetSectorX(sectorX);
    pUser->SetSectorY(sectorY);
    SectorPos curUserSector = pUser->GetCurSector();

    //�ٽ� ���� _curSector ������ _Sectors���� �ش� ��ǥ�� ���Ϳ� ���� unordered_map���� �ڱ� ����
    _sectors[curUserSector._y][curUserSector._x]._users.push_back(pUser);

    //�̵� ��� ���� ��Ŷ�� �ش� ���ǿ��� ����
    Message* pSectorMoveResponseMsg = Message::Alloc();
    MakePacket_SectorMoveResponse(pSectorMoveResponseMsg, pUser->GetAccountNo(), curUserSector._x, curUserSector._y);
    SendPacket(sessionID, pSectorMoveResponseMsg);
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
    if (messageLen == 0)
        __debugbreak();
    int retNum = pMessage->GetData((char*)message, messageLen);

    if (retNum != messageLen)
    {
        //__debugbreak();
        Disconnect(sessionID);
        return;
    }

    //����ID�� User �˻�
    User* pUser = _userContainer[sessionID];
    pUser->UpdateLastRecvTime();

    if (accountNo != pUser->GetAccountNo())
    {
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
    //����ID�� User �˻�
    User* pUser = _userContainer[sessionID];
    //User ��ü�� lastRecvTime ������ ���� �ð����� ����
    pUser->UpdateLastRecvTime();
}

void ChattingServer::PacketProc_UserDistribution(INT64 sessionID)
{
    Message* pResMessage = Message::Alloc();

    *pResMessage << (WORD)en_PACKET_CS_CHAT_RES_USER_DISTRIBUTION;

    USHORT localSectors[25][25];
    // 50x50���� ���� �迭 ���
    for (int i = 0; i < 25; i++) {
        for (int j = 0; j < 25; j++) {
            USHORT sum = 0;
            sum += static_cast<USHORT>(_sectors[2 * i][2 * j]._users.size());         // �»�
            sum += static_cast<USHORT>(_sectors[2 * i + 1][2 * j]._users.size());     // ����
            sum += static_cast<USHORT>(_sectors[2 * i][2 * j + 1]._users.size());     // ���
            sum += static_cast<USHORT>(_sectors[2 * i + 1][2 * j + 1]._users.size()); // ����
            localSectors[i][j] = sum;
        }
    }

    for (int i = 0; i < 25; i++)
    {
        pResMessage->PutData((char*)localSectors[i], 25 * sizeof(USHORT));
    }

    SendPacket(sessionID, pResMessage);
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
    for (int i = 0; i < sectorAround.count; i++)
    {
        for (auto e : _sectors[sectorAround.around[i]._y][sectorAround.around[i]._x]._users)
        {
            bool checkSend = SendPacket(e->GetSessionID(), pMessage);
        }
    }
    pMessage->SubRefCount();
    return true;
}

HANDLE ChattingServer::LogicThreadStarter()
{
    auto threadFunc = [](void* pThis)->unsigned int
        {
            ChattingServer* server = static_cast<ChattingServer*>(pThis);
            server->LogicThreadProc();
            return 0;
        };

    return (HANDLE)_beginthreadex(NULL, 0, threadFunc, this, 0, NULL);
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

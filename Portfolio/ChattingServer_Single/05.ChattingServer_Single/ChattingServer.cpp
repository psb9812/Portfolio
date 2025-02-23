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
    //핸들 정리
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
    wprintf(L"[System] 채팅 서버 스타트 성공...\n");

    return true;
}

void ChattingServer::Stop()
{
    //네트워크 부분부터 먼저 닫아준다.
    NetServer::Stop();  

    _isLogicThreadRun = false;
    _isTimerThreadRun = false;

    //로직 스레드를 종료시키기 위해 Terminate Job을 Job Queue에 넣어주고 이벤트 시그널을 전송한다.
    Job* pJobForTerminate = _jobPool.Alloc();
    pJobForTerminate->Set(en_JobType::JOB_TERMINATE, NULL, NULL);
    _jobQueue.Enqueue(pJobForTerminate);
    SetEvent(_jobQueueEvent);

    WaitForSingleObject(_hLogicThread, INFINITE);
    WaitForSingleObject(_hTimerThread, INFINITE);
    wprintf(L"# 컨텐츠 스레드 종료...\n");

    _isServerAlive = false;

    wprintf(L"# 서버 종료.\n");
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
        //Enqueue해서 증가된 size가 0에서 1이 된 경우에만 이벤트 시그널을 전송하므로 커널 전환을 줄인다.
        if (jobQueueOptional.value() == 1)
        {
            SetEvent(_jobQueueEvent);
        }
    }
    else
    {
        SystemLog::GetInstance()->WriteLog(L"ERROR", SystemLog::LOG_LEVEL_ERROR, L"잡큐가 꽉 찼다.");
        //잡큐가 가득 찼으므로 비정상적인 상황이다.
        //TODO: 서버를 우아하게 종료시킨다.
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
        //Enqueue해서 증가된 size가 0에서 1이 된 경우에만 이벤트 시그널을 전송하므로 커널 전환을 줄인다.
        if (jobQueueOptional.value() == 1)
        {
            SetEvent(_jobQueueEvent);
        }
    }
    else
    {
        SystemLog::GetInstance()->WriteLog(L"ERROR", SystemLog::LOG_LEVEL_ERROR, L"잡큐가 꽉 찼다.");
        //잡큐가 가득 찼으므로 비정상적인 상황이다.
        //TODO: 서버를 우아하게 종료시킨다.
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
        //Enqueue해서 증가된 size가 0에서 1이 된 경우에만 이벤트 시그널을 전송하므로 커널 전환을 줄인다.
        if (jobQueueOptional.value() == 1)
        {
            SetEvent(_jobQueueEvent);
        }
    }
    else
    {
        SystemLog::GetInstance()->WriteLog(L"ERROR", SystemLog::LOG_LEVEL_ERROR, L"잡큐가 꽉 찼다.");
        //잡큐가 가득 찼으므로 비정상적인 상황이다.
        //TODO: 서버를 우아하게 종료시킨다.
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

        //잡큐가 빌 때까지 디큐잉해서 처리한다.
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
        //1초에 한 번씩 TimerJob을 Job Queue에 넣어준다.
        Job* timerJob = _jobPool.Alloc();
        timerJob->Set(en_JobType::JOB_TIMER, NULL, nullptr);
        std::optional<LONG> jobQueueOptional = _jobQueue.Enqueue(timerJob);
        if (jobQueueOptional.has_value())
        {
            //Enqueue해서 증가된 size가 0에서 1이 된 경우에만 이벤트 시그널을 전송하므로 커널 전환을 줄인다.
            if (jobQueueOptional.value() == 1)
            {
                SetEvent(_jobQueueEvent);
            }
        }
        else
        {
            //잡큐가 가득 찼으므로 비정상적인 상황이다.
            //TODO: 서버를 우아하게 종료시킨다.
        }

        //Update TPS 측정 처리
        _prevUpdateTPS = _updateTPS;
        _updateTPS = 0;
        Sleep(TIMER_INTERVAL);
    }
}

void ChattingServer::HandleAcceptJob(__int64 sessionID)
{
    //User 풀에서 할당받기
    User* pUser = _userPool.Alloc();
    //_sessionID는 매개변수로 받은 값으로 초기화
    pUser->Init(sessionID);
    //User 객체 컨테이너에 삽입
    _userContainer.insert({ sessionID, pUser });
}

void ChattingServer::HandleReleaseJob(__int64 sessionID)
{
    // sessionID로 제거할 User 찾기
    User* pUser = _userContainer[sessionID];
    SectorPos userSector = pUser->GetCurSector();

    //User가 SectorMove 패킷을 받아서 Sector에 들어있는지 확인
    bool CheckAboutUserInSector = !(userSector._x == MAX_SECTOR_X && userSector._y == MAX_SECTOR_Y);
    // _Sectors 배열에서 해당 User 객체 찾아서 내부 User unordered_map에서 제거
    if(CheckAboutUserInSector)
    {
        _sectors[userSector._y][userSector._x]._users.remove(pUser);
    }
    // User 컨테이너에서도 제거
    _userContainer.erase(sessionID);

    if (pUser->GetLogin())
    {
        _currentUserNum--;
    }
    // User객체 풀에 반환
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
    //Recv 처리에서 생성한 메시지 레퍼런스 카운트 내리기
    int ret = pMessage->SubRefCount();
    if (ret != -1)
    {
        wprintf(L"수신 메시지 삭제 안 됨.\n");
        SystemLog::GetInstance()->WriteLog(L"ERROR", SystemLog::LOG_LEVEL_ERROR, L"수신 메시지 삭제 안 됨");
    }
}

void ChattingServer::HandleTimerJob()
{
    //모든 User를 순회하며 타임 아웃 여부를 체크하고 타임 아웃이면 연결을 끊는다.
    for (auto& userPair : _userContainer)
    {
        User* pUser = userPair.second;
        UINT64 timeOutValue = pUser->GetLogin() ? _timeoutDisconnectForUser : _timeoutDisconnectForSession;
        ULONGLONG deltaTime = GetTickCount64() - pUser->GetLastRecvTime();
        if (deltaTime > _timeoutDisconnectForUser)
        {
            //테스트 중에는 끊지 않는다.
            //Disconnect(userPair.first);
        }
    }
}

void ChattingServer::HandleTerminateJob()
{
    //접속 중인 모든 User 객체 소멸
    for (auto userPair : _userContainer)
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
            _sectors[i][j]._users.clear();
        }
    }
}

void ChattingServer::PacketProc_Login(INT64 sessionID, Message* pMessage)
{
    //직렬화 버퍼의 내용을 꺼내기
    INT64 accountNo = -1;
    WCHAR id[ID_LEN] = { 0, };
    WCHAR nickName[NICKNAME_LEN] = { 0, };
    char sessionKey[SESSIONKEY_LEN] = { 0, };

    //악의적인 공격 방어
    if (pMessage->GetDataSize() != sizeof(accountNo) + sizeof(id) + sizeof(nickName) + sizeof(sessionKey))
    {
        Disconnect(sessionID);
        return;
    }

    *pMessage >> accountNo;
    pMessage->GetData((char*)id, ID_LEN * sizeof(WCHAR));
    pMessage->GetData((char*)nickName, NICKNAME_LEN * sizeof(WCHAR));
    pMessage->GetData(sessionKey, SESSIONKEY_LEN);

    //세션ID로 User 검색
    User* pUser = _userContainer[sessionID];
    pUser->UpdateLastRecvTime();

    //인증 토큰을 통해 로그인 인증과정을 거친다.
    //아직 로그인 서버를 만들지 않았으므로 무조건 로그인 성공으로 간주

    BYTE loginStatus = 1; //1 : 성공 / 0 : 실패

    //User Max 치를 넘겼다면 로그인 실패로 처리한다.
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

    //해당 세션에게 로그인 응답 패킷 보내기
    Message* pLoginResponseMsg = Message::Alloc();
    MakePacket_LoginResponse(pLoginResponseMsg, loginStatus, pUser->GetAccountNo());
    SendPacket(sessionID, pLoginResponseMsg);
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

    //세션ID로 User 검색
    User* pUser = _userContainer[sessionID];
    pUser->UpdateLastRecvTime();

    if (accountNo != pUser->GetAccountNo())
    {
        //SystemLog::GetInstance()->WriteLog(L"ERROR", SystemLog::LOG_LEVEL_ERROR, L"accountNO 불일치.");
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

    SectorPos oldUserSector = pUser->GetCurSector();
    // x,y의 최대치는 최초 User 객체가 생성되었을 때만 가지는 값이다.
    // 이 때는 _sectors 배열에 반영되지 않았으므로 erase를 하지 않는다.
    bool isFristAllocToSector = oldUserSector._x == MAX_SECTOR_X && oldUserSector._y == MAX_SECTOR_Y;
    if (!(isFristAllocToSector))
    {
        //유저 객체의 _curSector 정보로 _Sectors에서 해당 좌표의 섹터에 유저 unordered_map에서 자신 제거
        _sectors[oldUserSector._y][oldUserSector._x]._users.remove(pUser);
    }

    //유저 객체의 _curSector 정보를 받은 SectorX와 SectorY로 갱신
    pUser->SetSectorX(sectorX);
    pUser->SetSectorY(sectorY);
    SectorPos curUserSector = pUser->GetCurSector();

    //다시 유저 _curSector 정보로 _Sectors에서 해당 좌표의 섹터에 유저 unordered_map에서 자기 삽입
    _sectors[curUserSector._y][curUserSector._x]._users.push_back(pUser);

    //이동 결과 응답 패킷을 해당 세션에게 전송
    Message* pSectorMoveResponseMsg = Message::Alloc();
    MakePacket_SectorMoveResponse(pSectorMoveResponseMsg, pUser->GetAccountNo(), curUserSector._x, curUserSector._y);
    SendPacket(sessionID, pSectorMoveResponseMsg);
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
    if (messageLen == 0)
        __debugbreak();
    int retNum = pMessage->GetData((char*)message, messageLen);

    if (retNum != messageLen)
    {
        //__debugbreak();
        Disconnect(sessionID);
        return;
    }

    //세션ID로 User 검색
    User* pUser = _userContainer[sessionID];
    pUser->UpdateLastRecvTime();

    if (accountNo != pUser->GetAccountNo())
    {
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
    //세션ID로 User 검색
    User* pUser = _userContainer[sessionID];
    //User 객체의 lastRecvTime 정보를 현재 시간으로 갱신
    pUser->UpdateLastRecvTime();
}

void ChattingServer::PacketProc_UserDistribution(INT64 sessionID)
{
    Message* pResMessage = Message::Alloc();

    *pResMessage << (WORD)en_PACKET_CS_CHAT_RES_USER_DISTRIBUTION;

    USHORT localSectors[25][25];
    // 50x50에서 줄인 배열 계산
    for (int i = 0; i < 25; i++) {
        for (int j = 0; j < 25; j++) {
            USHORT sum = 0;
            sum += static_cast<USHORT>(_sectors[2 * i][2 * j]._users.size());         // 좌상
            sum += static_cast<USHORT>(_sectors[2 * i + 1][2 * j]._users.size());     // 좌하
            sum += static_cast<USHORT>(_sectors[2 * i][2 * j + 1]._users.size());     // 우상
            sum += static_cast<USHORT>(_sectors[2 * i + 1][2 * j + 1]._users.size()); // 우하
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
    // 직렬화 버퍼가 여러 세션에게 보내는 동안 Free되지 않도록 레퍼런스 카운트를 여기서 증가 시킨다.
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

#include "pch.h"
#include "LoginContents.h"
#include "EchoServer.h"
#include "Message.h"
#include "User.h"

LoginContents::LoginContents(NetServer* server, INT32 fps, int contentsID)
    :Contents(server, fps, contentsID)
{
    _pServer = reinterpret_cast<EchoServer*>(server);
}

LoginContents::~LoginContents()
{
}

void LoginContents::OnRecv(INT64 sessionID, Message* pMessage)
{
    if (pMessage->GetDataSize() < sizeof(WORD))
    {
        SystemLog::GetInstance()->WriteLog(L"ERROR",
            SystemLog::LOG_LEVEL_ERROR,
            L"로그인 컨텐츠에서 메시지 타입 크기 미만의 데이터 크기를 가지는 직렬화 버퍼 수신");
        DisconnectInContents(sessionID);
        pMessage->SubRefCount();
        return;
    }

    WORD messageType = -1;
    *pMessage >> messageType;

    switch (messageType)
    {
    case en_PACKET_CS_GAME_REQ_LOGIN:
        PacketProc_Login(sessionID, pMessage);
        break;
    default:
        SystemLog::GetInstance()->WriteLog(L"ERROR",
            SystemLog::LOG_LEVEL_ERROR,
            L"로그인 컨텐츠에서 유효하지 않은 패킷 타입을 받았습니다.");
        DisconnectInContents(sessionID);
        break;
    }

    pMessage->SubRefCount();
}

void LoginContents::OnEnter(INT64 sessionID, IUser* pUser)
{
    _userContainer.insert({ sessionID, pUser });
}

void LoginContents::OnLeave(INT64 sessionID)
{
    _userContainer.erase(sessionID);
}

void LoginContents::OnDisconnect(INT64 sessionID)
{
    User* pUser = reinterpret_cast<User*>(_userContainer[sessionID]);
    if (pUser->GetLogin())
    {
        _pServer->DecreaseCurrentUserNum();
    }
    //컨텐츠에서 관리하는 UserContainer에서 지우기
    _userContainer.erase(sessionID);
    //서버에서 관리하는 UserContainer에서 지우기
    _pServer->DeleteUser(sessionID);
    //User 메모리 반납
    _pServer->FreeUserMemory(pUser);
}

void LoginContents::OnUpdate()
{
    _frameCnt++;
}

void LoginContents::PacketProc_Login(INT64 sessionID, Message* pMessage)
{
    //직렬화 버퍼의 내용을 꺼내기
    INT64 accountNo = -1;
    char sessionKey[SESSIONKEY_LEN] = { 0, };

    //악의적인 공격 방어
    if (pMessage->GetDataSize() != sizeof(accountNo) + sizeof(sessionKey) + sizeof(int))
    {
        DisconnectInContents(sessionID);
        return;
    }

    *pMessage >> accountNo;
    pMessage->GetData(sessionKey, SESSIONKEY_LEN);

    //세션ID로 User 검색
    User* pUser = reinterpret_cast<User*>(_userContainer[sessionID]);
    pUser->UpdateLastRecvTime();

    //무조건 로그인 성공으로 간주

    BYTE loginStatus = 1; //1 : 성공 / 0 : 실패

    INT32 currentUserNum = _pServer->GetCurrentUserNum();
    //User Max 치를 넘겼다면 로그인 실패로 처리한다.
    if (_pServer->_numOfMaxUser <= currentUserNum)
    {
        loginStatus = 0;
    }

    if (loginStatus)
    {
        pUser->Login();
        pUser->SetAccountNo(accountNo);
        _pServer->IncreaseCurrentUserNum();
        ReserveMoveContents(sessionID, static_cast<INT32>(ContentsID::Echo));
    }
    else
    {
        DisconnectInContents(sessionID);
    }

    //해당 세션에게 로그인 응답 패킷 보내기
    Message* pLoginResponseMsg = Message::Alloc();
    MakePacket_LoginResponse(pLoginResponseMsg, loginStatus, pUser->GetAccountNo());
    _pServer->SendPacket(sessionID, pLoginResponseMsg, true);
}

void LoginContents::MakePacket_LoginResponse(Message* pMessage, BYTE status, INT64 accountNo)
{
    *pMessage << (WORD)en_PACKET_CS_GAME_RES_LOGIN << status << accountNo;
}

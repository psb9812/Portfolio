#include "pch.h"
#include "EchoContents.h"
#include "EchoServer.h"
#include "Message.h"
#include "User.h"

EchoContents::EchoContents(NetServer* server, INT32 fps, int contentsID)
	:Contents(server, fps, contentsID)
{
	_pServer = reinterpret_cast<EchoServer*>(server);
}

EchoContents::~EchoContents()
{
}

void EchoContents::OnRecv(INT64 sessionID, Message* pMessage)
{
    if (pMessage->GetDataSize() < sizeof(WORD))
    {
        SystemLog::GetInstance()->WriteLog(L"ERROR",
            SystemLog::LOG_LEVEL_ERROR,
            L"에코 컨텐츠에서 메시지 타입 크기 미만의 데이터 크기를 가지는 직렬화 버퍼 수신");
        DisconnectInContents(sessionID);
        pMessage->SubRefCount();
        return;
    }

    WORD messageType = -1;
    *pMessage >> messageType;

    switch (messageType)
    {
    case en_PACKET_CS_GAME_REQ_ECHO:
        PacketProc_Echo(sessionID, pMessage);
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

void EchoContents::OnEnter(INT64 sessionID, IUser* pUser)
{
	_userContainer.insert({ sessionID, pUser });
}

void EchoContents::OnLeave(INT64 sessionID)
{
	_userContainer.erase(sessionID);
}

void EchoContents::OnDisconnect(INT64 sessionID)
{
    User* pUser = reinterpret_cast<User*>(_userContainer[sessionID]);
    //컨텐츠에서 관리하는 UserContainer에서 지우기
    _userContainer.erase(sessionID);
    //서버에서 관리하는 UserContainer에서 지우기
    _pServer->DeleteUser(sessionID);

    _pServer->DecreaseCurrentUserNum();
    //User 메모리 반환
    _pServer->FreeUserMemory(pUser);
}

void EchoContents::OnUpdate()
{
    _frameCnt++;
}

void EchoContents::PacketProc_Echo(INT64 sessionID, Message* pMessage)
{
    //직렬화 버퍼의 내용을 꺼내기
    INT64 accountNo = -1;
    INT64 sendTick = -1;

    //악의적인 공격 방어
    if (pMessage->GetDataSize() != sizeof(accountNo) + sizeof(sendTick))
    {
        DisconnectInContents(sessionID);
        return;
    }

    *pMessage >> accountNo >> sendTick;
    
    //세션ID로 User 검색
    User* pUser = reinterpret_cast<User*>(_userContainer[sessionID]);

    pUser->UpdateLastRecvTime();

    Message* pEchoResponseMsg = Message::Alloc();
    MakePacket_EchoResponse(pEchoResponseMsg, pUser->GetAccountNo(), sendTick);
    _pServer->SendPacket(sessionID, pEchoResponseMsg, true);
}

void EchoContents::MakePacket_EchoResponse(Message* pMessage, INT64 accountNo, INT64 sendTick)
{
    *pMessage << (WORD)en_PACKET_CS_GAME_RES_ECHO << accountNo << sendTick;
}
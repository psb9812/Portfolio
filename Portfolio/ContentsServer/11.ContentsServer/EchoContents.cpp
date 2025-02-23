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
            L"���� ���������� �޽��� Ÿ�� ũ�� �̸��� ������ ũ�⸦ ������ ����ȭ ���� ����");
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
            L"�α��� ���������� ��ȿ���� ���� ��Ŷ Ÿ���� �޾ҽ��ϴ�.");
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
    //���������� �����ϴ� UserContainer���� �����
    _userContainer.erase(sessionID);
    //�������� �����ϴ� UserContainer���� �����
    _pServer->DeleteUser(sessionID);

    _pServer->DecreaseCurrentUserNum();
    //User �޸� ��ȯ
    _pServer->FreeUserMemory(pUser);
}

void EchoContents::OnUpdate()
{
    _frameCnt++;
}

void EchoContents::PacketProc_Echo(INT64 sessionID, Message* pMessage)
{
    //����ȭ ������ ������ ������
    INT64 accountNo = -1;
    INT64 sendTick = -1;

    //�������� ���� ���
    if (pMessage->GetDataSize() != sizeof(accountNo) + sizeof(sendTick))
    {
        DisconnectInContents(sessionID);
        return;
    }

    *pMessage >> accountNo >> sendTick;
    
    //����ID�� User �˻�
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
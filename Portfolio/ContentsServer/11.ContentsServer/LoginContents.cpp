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
            L"�α��� ���������� �޽��� Ÿ�� ũ�� �̸��� ������ ũ�⸦ ������ ����ȭ ���� ����");
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
            L"�α��� ���������� ��ȿ���� ���� ��Ŷ Ÿ���� �޾ҽ��ϴ�.");
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
    //���������� �����ϴ� UserContainer���� �����
    _userContainer.erase(sessionID);
    //�������� �����ϴ� UserContainer���� �����
    _pServer->DeleteUser(sessionID);
    //User �޸� �ݳ�
    _pServer->FreeUserMemory(pUser);
}

void LoginContents::OnUpdate()
{
    _frameCnt++;
}

void LoginContents::PacketProc_Login(INT64 sessionID, Message* pMessage)
{
    //����ȭ ������ ������ ������
    INT64 accountNo = -1;
    char sessionKey[SESSIONKEY_LEN] = { 0, };

    //�������� ���� ���
    if (pMessage->GetDataSize() != sizeof(accountNo) + sizeof(sessionKey) + sizeof(int))
    {
        DisconnectInContents(sessionID);
        return;
    }

    *pMessage >> accountNo;
    pMessage->GetData(sessionKey, SESSIONKEY_LEN);

    //����ID�� User �˻�
    User* pUser = reinterpret_cast<User*>(_userContainer[sessionID]);
    pUser->UpdateLastRecvTime();

    //������ �α��� �������� ����

    BYTE loginStatus = 1; //1 : ���� / 0 : ����

    INT32 currentUserNum = _pServer->GetCurrentUserNum();
    //User Max ġ�� �Ѱ�ٸ� �α��� ���з� ó���Ѵ�.
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

    //�ش� ���ǿ��� �α��� ���� ��Ŷ ������
    Message* pLoginResponseMsg = Message::Alloc();
    MakePacket_LoginResponse(pLoginResponseMsg, loginStatus, pUser->GetAccountNo());
    _pServer->SendPacket(sessionID, pLoginResponseMsg, true);
}

void LoginContents::MakePacket_LoginResponse(Message* pMessage, BYTE status, INT64 accountNo)
{
    *pMessage << (WORD)en_PACKET_CS_GAME_RES_LOGIN << status << accountNo;
}

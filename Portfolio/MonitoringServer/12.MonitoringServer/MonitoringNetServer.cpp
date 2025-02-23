#include "MonitoringNetServer.h"
#include "User.h"
#include "Tools/Message.h"
#include "../../CommonProtocol.h"



MonitoringNetServer::MonitoringNetServer()
{
	InitializeSRWLock(&_lock);
}

MonitoringNetServer::~MonitoringNetServer()
{
}

bool MonitoringNetServer::Start(const WCHAR* configFileName)
{
	NetServer::Start(configFileName);

	wprintf(L"# 모니터링 넷 서버 스타트.\n");
	return true;
}

void MonitoringNetServer::Stop()
{
	NetServer::Stop();

	wprintf(L"# 모니터링 넷 서버 종료.\n");
}

bool MonitoringNetServer::OnConnectionRequest(const WCHAR* connectIP, INT32 connectPort)
{
	return true;
}

void MonitoringNetServer::OnAccept(INT64 sessionID)
{
}

void MonitoringNetServer::OnRelease(INT64 sessionID)
{
	AcquireSRWLockExclusive(&_lock);
	for (auto iter = _users.begin(); iter != _users.end(); iter++)
	{
		if (*iter == sessionID)
		{
			_users.erase(iter);
			break;
		}
	}
	ReleaseSRWLockExclusive(&_lock);
}

void MonitoringNetServer::OnRecv(INT64 sessionID, Message* pMessage)
{
	WORD messageType = -1;
	(*pMessage) >> messageType;

	switch (messageType)
	{
	case en_PACKET_CS_MONITOR_TOOL_REQ_LOGIN:
		HandleMonitorToolLoginReq(sessionID, pMessage);
		break;
	default:
		Disconnect(sessionID);
		SystemLog::GetInstance()->WriteLog(L"ERROR", SystemLog::LOG_LEVEL_ERROR,
			L"알 수 없는 타입의 패킷");
		break;
	}
	pMessage->SubRefCount();
}

void MonitoringNetServer::OnSend(INT64 sessionID, INT32 sendSize)
{
}

void MonitoringNetServer::OnTime()
{
}

void MonitoringNetServer::OnError(INT32 errorCode, WCHAR* comment)
{
}

IUser* MonitoringNetServer::GetUser(INT64 sessionID)
{
	return nullptr;
}

void MonitoringNetServer::HandleMonitorToolLoginReq(INT64 sessionID, Message* pMessage)
{
	char loginSessionKey[33];

	if (pMessage->GetDataSize() != sizeof(loginSessionKey) - 1)
	{
		Disconnect(sessionID);
		__debugbreak();
		return;
	}

	pMessage->GetData(loginSessionKey, sizeof(loginSessionKey) - 1);
	loginSessionKey[32] = '\0';

	BYTE status = 0;
	if (strcmp(loginSessionKey, MONITOR_TOOL_SESSION_KEY) == 0)
	{
		AcquireSRWLockExclusive(&_lock);
		_users.push_back(sessionID);
		ReleaseSRWLockExclusive(&_lock);
		status = static_cast<BYTE>(dfMONITOR_TOOL_LOGIN_OK);
	}
	else
	{
		status = static_cast<BYTE>(dfMONITOR_TOOL_LOGIN_ERR_SESSIONKEY);
	}

	Message* loginResMessage = Message::Alloc();
	MakePacket_MonitorToolLoginRes(loginResMessage, status);
	SendPacket(sessionID, loginResMessage);
}

void MonitoringNetServer::MakePacket_MonitorToolLoginRes(Message* pMessage, BYTE status)
{
	*pMessage << static_cast<WORD>(en_PACKET_CS_MONITOR_TOOL_RES_LOGIN) << status;
}

bool MonitoringNetServer::SendPacket_AllSession(Message* pMessage)
{
	pMessage->AddRefCount();
	AcquireSRWLockExclusive(&_lock);
	for (INT64 sessionID : _users)
	{
		if (!SendPacket(sessionID, pMessage))
		{
			pMessage->SubRefCount();
			ReleaseSRWLockExclusive(&_lock);
			return false;
		}
	}
	ReleaseSRWLockExclusive(&_lock);
	pMessage->SubRefCount();
	return true;
}
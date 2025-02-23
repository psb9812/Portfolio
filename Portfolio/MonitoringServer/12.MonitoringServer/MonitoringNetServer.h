#pragma once
#include "NetServer.h"
#include <vector>
////////////////////////////////////////
//# MonitoringNetServer
// ����͸� Ŭ��� �� ����ϴ� ����͸� �� ����
// 
//# �ϴ� �� : 
// 1. ����͸� Ŭ���̾�Ʈ�� �α��� �޽��� ���� �� ����
// 2. �� �����͵��� �ִ�/���/�ּ� �� ���ϱ�
// 
////////////////////////////////////////

class Message;
class MonitoringNetServer :
    public NetServer
{
public:
	MonitoringNetServer();
	~MonitoringNetServer();

	bool Start(const WCHAR* configFileName) override;
	void Stop() override;
    bool SendPacket_AllSession(Message* pMessage);
private:
	bool OnConnectionRequest(const WCHAR* connectIP, INT32 connectPort) override;
	void OnAccept(INT64 sessionID) override;
	void OnRelease(INT64 sessionID) override;
	void OnRecv(INT64 sessionID, Message* pMessage) override;
	void OnSend(INT64 sessionID, INT32 sendSize) override;
	void OnTime() override;
	void OnError(INT32 errorCode, WCHAR* comment) override;

	virtual IUser* GetUser(INT64 sessionID) override;

	void HandleMonitorToolLoginReq(INT64 sessionID, Message* pMessage);
	void MakePacket_MonitorToolLoginRes(Message* pMessage, BYTE status);

public:

private:
    std::vector<INT64> _users;
	SRWLOCK _lock;

};


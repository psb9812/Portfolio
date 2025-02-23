#pragma once
#include "NetServer.h"
#include <vector>
////////////////////////////////////////
//# MonitoringNetServer
// 모니터링 클라와 넷 통신하는 모니터링 넷 서버
// 
//# 하는 일 : 
// 1. 모니터링 클라이언트의 로그인 메시지 수신 및 응답
// 2. 각 데이터들의 최대/평균/최소 값 구하기
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


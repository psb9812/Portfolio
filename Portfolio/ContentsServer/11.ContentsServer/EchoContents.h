#pragma once
#include "Contents.h"

class EchoServer;

class EchoContents : public Contents
{
public:
	EchoContents(NetServer* server, INT32 fps, int contentsID);
	virtual ~EchoContents();

	/////////////////////////////
	// Event callback function
	/////////////////////////////
	void OnRecv(INT64 sessionID, Message* pMessage) override;
	void OnEnter(INT64 sessionID, IUser* pUser) override;
	void OnLeave(INT64 sessionID) override;
	void OnDisconnect(INT64 sessionID) override;
	void OnUpdate() override;

private:
	void PacketProc_Echo(INT64 sessionID, Message* pMessage);
	void MakePacket_EchoResponse(Message* pMessage, INT64 accountNo, INT64 sendTick);

private:
	EchoServer* _pServer = nullptr;
};


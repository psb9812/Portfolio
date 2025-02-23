#pragma once
#include "Contents.h"

class EchoServer;

class LoginContents : public Contents
{
public:
	LoginContents(NetServer* server, INT32 fps, int contentsID);
	virtual ~LoginContents();

	/////////////////////////////
	// Event callback function
	/////////////////////////////
	void OnRecv(INT64 sessionID, Message* pMessage) override;
	void OnEnter(INT64 sessionID, IUser* pUser) override;
	void OnLeave(INT64 sessionID) override;
	void OnDisconnect(INT64 sessionID) override;
	void OnUpdate() override;

private:
	void PacketProc_Login(INT64 sessionID, Message* pMessage);

	void MakePacket_LoginResponse(Message* pMessage, BYTE status, INT64 accountNo);

private:
	EchoServer* _pServer = nullptr;
};


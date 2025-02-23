#pragma comment(lib, "ws2_32")
#pragma comment(lib, "Winmm.lib")
#include <iostream>
#include <WinSock2.h>
#include <Windows.h>
#include <list>
#include <locale>
#include "Session.h"
#include "PacketDefine.h"
#include "ScreenDefine.h"
#include "PacketGenerator.h"
#include "Message.h"

#define SERVERPORT 5000
#define MAX_PLAYER 50
#define MOVE_X_PER_FRAME		3
#define MOVE_Y_PER_FRAME		2

void error_display(const WCHAR* func)
{
	wprintf(L"%s ErrorCode : %d\n", func, GetLastError());
	return;
}

//��Ʈ��ũ �ۼ��� ó�� �Լ�
void NetIOProcess();
//���� ���� ������Ʈ �Լ�
void Update();
//���ο� ������ ������ ó���ϴ� �Լ�
void netProc_Accept();
//���� sendQ�� �ִ� �����͸� ������.
void netProc_Send(Session* session);
//���� recvQ�� �ִ� �����͸� �޴´�.
void netProc_Recv(Session* session);
//������ ���´�.
void netProc_Disconnect();
void ReserveDisconnect(Session* session);
//�� ���� �������� ���� �޽����� sendQ�� �ִ� �Լ�
void SendUnicast(Session* session, char* buffer, int size);
//���� ���� �������� ���� �޽����� sendQ�� �ִ� �Լ�
void SendBroadcast(Session* exceptSession, char* buffer, int size);
//��Ŷ ��� Ÿ�Կ� ���� �б� �Լ�
bool PacketProc(Session* session, unsigned char type, Message* buffer);

bool netPacketProc_MoveStart(Session* session, Message* buffer);
bool netPacketProc_MoveStop(Session* session, Message* buffer);
bool netPacketProc_Attack1(Session* session, Message* buffer);
bool netPacketProc_Attack2(Session* session, Message* buffer);
bool netPacketProc_Attack3(Session* session, Message* buffer);



std::list<Session*> g_sessionList;	//�������� ���� ����Ʈ
std::list<Session*> g_disconList;	//���� ���� ����Ʈ
int g_idCount = 0;					//�����ϸ� �ο��� id
bool g_isShutdown = false;
SOCKET g_ListenSocket;
PacketGenerator g_packetGenerator;

const DWORD frameDuration = 20;

int main()
{
	setlocale(LC_ALL, "");
	timeBeginPeriod(1);
	srand(static_cast<unsigned int>(time(NULL)));
	int retOfBind, retOfListen, retOfIoctl;

	WSADATA wsa;
	//���� �ʱ�ȭ
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		error_display(L"WSAStartup() error");
		return 0;
	}
	wprintf(L"WSAStartup #\n");

	//socket
	g_ListenSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (g_ListenSocket == SOCKET_ERROR)
	{
		error_display(L"socket() error");
		return 0;
	}

	SOCKADDR_IN serveraddr;
	memset(&serveraddr, 0, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons(SERVERPORT);

	//bind
	retOfBind = bind(g_ListenSocket, (SOCKADDR*)&serveraddr, sizeof(serveraddr));
	if (retOfBind == SOCKET_ERROR)
	{
		error_display(L"bind() error");
		return 0;
	}
	wprintf(L"Bind Ok #\n");
	//listen
	retOfListen = listen(g_ListenSocket, SOMAXCONN);
	if (retOfListen == SOCKET_ERROR)
	{
		error_display(L"listen() error");
		return 0;
	}
	wprintf(L"Listen Ok #\n");

	u_long on = 1;
	//����ŷ ���� ��ȯ
	retOfIoctl = ioctlsocket(g_ListenSocket, FIONBIO, &on);

	//���� �ɼ� ���� (TimeWait ���� ���ֱ�)
	linger optval;
	optval.l_linger = 0;
	optval.l_onoff = 1;
	setsockopt(g_ListenSocket, SOL_SOCKET, SO_LINGER, (char*) & optval, sizeof(optval));


	DWORD lastUpdateTime = timeGetTime();

	while (!g_isShutdown)
	{
		DWORD currentTime = timeGetTime();
		//��Ʈ��ũ �ۼ���
		NetIOProcess();

		// ���� �ð�
		DWORD elapsedTime = currentTime - lastUpdateTime;

		if (elapsedTime < frameDuration)
		{
			continue;
		}

		//������
		Update();
		
		for (auto session : g_sessionList)
		{
			wprintf(L"RecvBuffer DirectEnqueueSize : %d\nRecvBuffer FreeSize : \t%d\n",
				session->recvRingBuffer.DirectEnqueueSize(),
				session->recvRingBuffer.GetFreeSize());
		}

		lastUpdateTime = currentTime;

		//���� ���� �۾�
		netProc_Disconnect();
	}

	
	return 0;
}

void NetIOProcess()
{
	int retOfSelect = 0;
	int retSelectCount = 0;
	fd_set readSet;
	fd_set writeSet;
	timeval selectTime = { 0,0 };

	FD_ZERO(&readSet);
	FD_ZERO(&writeSet);
	FD_SET(g_ListenSocket, &readSet);

	//���� ���� ��� ������ readSet�� �ִ´�.
	for (Session* session : g_sessionList)
	{
		FD_SET(session->socket, &readSet);

		if (session->sendRingBuffer.GetUseSize() > 0)	//���� ���� �ִٸ� writeSet�� �����ϱ�
			FD_SET(session->socket, &writeSet);
	}
	
	retOfSelect = select(0, &readSet, &writeSet, NULL, &selectTime);
	if (retOfSelect == SOCKET_ERROR)
		error_display(L"select() error");
	else if (retOfSelect > 0)    //�ۼ��� ������ ���
	{
		//�ű� ������ ������ ���
		if (FD_ISSET(g_ListenSocket, &readSet) && g_sessionList.size() < MAX_PLAYER)
		{
			retSelectCount++;
			netProc_Accept();

			if (retSelectCount == retOfSelect)
				return;
		}
		
		for (auto session : g_sessionList)
		{
			if (FD_ISSET(session->socket, &readSet))
			{
				//���� ���� ���� �ִ� ���
				retSelectCount++;
				netProc_Recv(session);

				if (retSelectCount == retOfSelect)
					return;
			}

			if (FD_ISSET(session->socket, &writeSet))
			{
				//������ ������ ������.
				retSelectCount++;
				netProc_Send(session);

				if (retSelectCount == retOfSelect)
					return;
			}
		}
	}
}

void Update()
{
	//����
	for (Session* session : g_sessionList)
	{
		if (session->HP <= 0)
		{
			//���ó��
			ReserveDisconnect(session);
		}
		else
		{
			//���� ���ۿ� ���� ó��
			switch (session->action)
			{
			case PACKET_MOVE_DIR_LL:
				session->x -= MOVE_X_PER_FRAME;
				if (session->x < RANGE_MOVE_LEFT)
				{
					session->x = RANGE_MOVE_LEFT;
					session->action = PACKET_MOVE_STOP;
				}
				break;
			case PACKET_MOVE_DIR_LU:
				session->x -= MOVE_X_PER_FRAME;
				if (session->x < RANGE_MOVE_LEFT)
				{
					session->x = RANGE_MOVE_LEFT;
					session->action = PACKET_MOVE_STOP;
				}
				session->y -= MOVE_Y_PER_FRAME;
				if (session->y < RANGE_MOVE_TOP)
				{
					session->y = RANGE_MOVE_TOP;
					session->action = PACKET_MOVE_STOP;
				}
				break;
			case PACKET_MOVE_DIR_UU:
				session->y -= MOVE_Y_PER_FRAME;
				if (session->y < RANGE_MOVE_TOP)
				{
					session->y = RANGE_MOVE_TOP;
					session->action = PACKET_MOVE_STOP;
				}
				break;
			case PACKET_MOVE_DIR_RU:
				session->x += MOVE_X_PER_FRAME;
				if (session->x > RANGE_MOVE_RIGHT)
				{
					session->x = RANGE_MOVE_RIGHT;
					session->action = PACKET_MOVE_STOP;
				}
				session->y -= MOVE_Y_PER_FRAME;
				if (session->y < RANGE_MOVE_TOP)
				{
					session->y = RANGE_MOVE_TOP;
					session->action = PACKET_MOVE_STOP;
				}
				break;
			case PACKET_MOVE_DIR_RR:
				session->x += MOVE_X_PER_FRAME;
				if (session->x > RANGE_MOVE_RIGHT)
				{
					session->x = RANGE_MOVE_RIGHT;
					session->action = PACKET_MOVE_STOP;
				}
				break;
			case PACKET_MOVE_DIR_RD:
				session->x += MOVE_X_PER_FRAME;
				if (session->x > RANGE_MOVE_RIGHT)
				{
					session->x = RANGE_MOVE_RIGHT;
					session->action = PACKET_MOVE_STOP;
				}
				session->y += MOVE_Y_PER_FRAME;
				if (session->y > RANGE_MOVE_BOTTOM)
				{
					session->y = RANGE_MOVE_BOTTOM;
					session->action = PACKET_MOVE_STOP;
				}
				break;
			case PACKET_MOVE_DIR_DD:
				session->y += MOVE_Y_PER_FRAME;
				if (session->y > RANGE_MOVE_BOTTOM)
				{
					session->y = RANGE_MOVE_BOTTOM;
					session->action = PACKET_MOVE_STOP;
				}
				break;
			case PACKET_MOVE_DIR_LD:
				session->x -= MOVE_X_PER_FRAME;
				if (session->x < RANGE_MOVE_LEFT)
				{
					session->x = RANGE_MOVE_LEFT;
					session->action = PACKET_MOVE_STOP;
				}
				session->y += MOVE_Y_PER_FRAME;
				if (session->y > RANGE_MOVE_BOTTOM)
				{
					session->y = RANGE_MOVE_BOTTOM;
					session->action = PACKET_MOVE_STOP;
				}
				break;
			}
		}
	}
}

void netProc_Accept()
{
	SOCKADDR_IN clientAddr;
	int addrlen = sizeof(clientAddr);
	SOCKET clientSocket = accept(g_ListenSocket, (SOCKADDR*)&clientAddr, &addrlen);
	if (clientSocket == INVALID_SOCKET)
		error_display(L"accept() error");

	Session* newSession = new Session;
	newSession->socket = clientSocket;
	newSession->sockaddr = clientAddr;
	newSession->HP = 100;
	newSession->sessionID = g_idCount++;
	newSession->action = PACKET_MOVE_STOP;
	newSession->direction = PACKET_MOVE_DIR_RR;

	//������ ���� ��ġ ����
	newSession->x = rand() % (RANGE_MOVE_RIGHT - 10 - RANGE_MOVE_LEFT + 10)  + RANGE_MOVE_LEFT + 10;
	newSession->y = rand() % (RANGE_MOVE_BOTTOM - 10 - RANGE_MOVE_TOP + 10) + RANGE_MOVE_TOP + 10;

	g_sessionList.push_back(newSession);

	//�ش� �������� CREATE_MY_CHARACTER_PACKET ������
	{
		Message crtMyCharMsg;
		g_packetGenerator.MakeCreateMyCharacterPacket(&crtMyCharMsg, newSession->sessionID, newSession->HP,
			newSession->x, newSession->y, newSession->direction);

		SendUnicast(newSession, crtMyCharMsg.GetBufferPtr(), crtMyCharMsg.GetDataSize());
		wprintf(L"send!!\n Type: CreateMyCharactor / ID : %d / Dir : %d / X : %d / Y : %d\n",
			newSession->sessionID, newSession->direction, newSession->x, newSession->y);
	}

	//�ش� �������� ���� ���� ĳ���͸� �����ϱ� ���� CREATE_OTHER_CHARACTER_PACKET ������
	for (auto session : g_sessionList)
	{
		if (session == newSession)
			continue;

		Message crtOtherCharMsg;
		g_packetGenerator.MakeCreateOtherCharactorPacket(&crtOtherCharMsg,
			session->sessionID, session->HP, session->x, session->y, session->direction);

		SendUnicast(newSession, crtOtherCharMsg.GetBufferPtr(), crtOtherCharMsg.GetDataSize());

		//���� �ű� ������ ������ ��, �� ������ �̵� ���̶�� MoveStart ��Ŷ�� �ٷ� �������� ������ �̵��� ���̰� �Ѵ�.
		if (session->action != PACKET_MOVE_STOP)
		{
			Message MoveMsg;
			g_packetGenerator.MakeMoveStartPacket(&MoveMsg, session->sessionID, session->direction, session->x, session->y);

			SendUnicast(newSession, MoveMsg.GetBufferPtr(), MoveMsg.GetDataSize());
		}
	}


	//���� �����鿡�� �� ������ ĳ���͸� �����϶�� CREATE_OTHER_CHARACTER_PACKET ������
	{
		Message msg;
		g_packetGenerator.MakeCreateOtherCharactorPacket(&msg, newSession->sessionID, newSession->HP,
			newSession->x, newSession->y, newSession->direction);

		SendBroadcast(newSession, msg.GetBufferPtr(), msg.GetDataSize());
	}
}

void SendUnicast(Session* session, char* buffer, int size)
{
	int retOfEnqueue;
	retOfEnqueue = session->sendRingBuffer.Enqueue(buffer, size);
	if (retOfEnqueue < size)
	{
		//L4�� �۽Ź��۰� ���� ���� ���� ������ ���� ���۰� ������ ����̴�.
		//�̴� ������ ���������� ����� ���� ���ϴ� ��Ȳ�̹Ƿ� ������ ����� �Ѵ�.
		//L4�� �۽� ���۰� �������� send�� ����� ���� ���ϸ� �۽� �����۰� ���� �� �� ����.
		//���������� ���� ���������.
		wprintf(L"Error: SessionID: %d, �۽� �����۰� �������� ������ �����ϴ�.\n", session->sessionID);
		ReserveDisconnect(session);
	}
}

void SendBroadcast(Session* exceptSession, char* buffer, int size)
{
	int retOfEnqueue;

	for (Session* session : g_sessionList)
	{
		if (session == exceptSession)
			continue;

		retOfEnqueue = session->sendRingBuffer.Enqueue(buffer, size);
		if (retOfEnqueue < size)
		{
			wprintf(L"Error: SessionID: %d, ���� �����۰� �������� ������ �����ϴ�.\n", session->sessionID);
			ReserveDisconnect(session);
		}
	}
}

void netProc_Send(Session* session)
{
	int retOfSend;
	//sendQ�� �ִ� �����͸� ������.
	if (session->sendRingBuffer.GetUseSize() > session->sendRingBuffer.DirectDequeueSize())	//�������� ��迡 �����Ͱ� ��ģ ���
	{
		retOfSend = send(session->socket, session->sendRingBuffer.GetFrontBufferPtr(), session->sendRingBuffer.DirectDequeueSize(), 0);
		if (retOfSend == SOCKET_ERROR)
		{
			//���� ����
			wprintf(L"Error: SessionID: %d, send()ȣ�� ����\n", session->sessionID);
			ReserveDisconnect(session);
			return;
		}
		session->sendRingBuffer.MoveFront(retOfSend);
	}
	else
	{
		retOfSend = send(session->socket, session->sendRingBuffer.GetFrontBufferPtr(), session->sendRingBuffer.GetUseSize(), 0);
		if (retOfSend == SOCKET_ERROR)
		{
			//���� ����
			wprintf(L"Error: SessionID: %d, send() ȣ�� ����\n", session->sessionID);
			ReserveDisconnect(session);
			return;
		}
		session->sendRingBuffer.MoveFront(retOfSend);
	}
}

void netProc_Recv(Session* session)
{
	int retOfRecv;
	//�ϴ� �����͸� recvQ�� �ֱ�
	retOfRecv = recv(session->socket, session->recvRingBuffer.GetRearBufferPtr(), session->recvRingBuffer.DirectEnqueueSize(), 0);
	if (retOfRecv == 0)
	{
		if (session->recvRingBuffer.DirectEnqueueSize() != 0)
		{
			wprintf(L"recvError!!!");
			//����
			ReserveDisconnect(session);
			return;
		}
	}
	else if (retOfRecv == SOCKET_ERROR)
	{
		if (GetLastError() != 10054)
		{
			error_display(L"recv()");
		}
		ReserveDisconnect(session);
		return;
	}
	session->recvRingBuffer.MoveRear(retOfRecv);

	while (1)
	{
		//����� �ְų� ����� ���ٸ� �ݺ��� Ż��
		if (session->recvRingBuffer.GetUseSize() <= sizeof(PACKET_HEADER))
			break;

		PACKET_HEADER header;
		session->recvRingBuffer.Peek((char*) & header, sizeof(PACKET_HEADER));

		//��Ŷ �ڵ尡 �츮�� ������ �Ͱ� �ٸ��ٸ� �̻� ������ �Ǵ��ϰ� ���� ���´�.
		if (header.code != 0x89)
		{
			ReserveDisconnect(session);
			break;
		}

		//����� ��õ� ���̷ε庸�� ���� �����Ͱ� ������ Ż��
		if (header.size > session->recvRingBuffer.GetUseSize() - sizeof(PACKET_HEADER))
			break;

		session->recvRingBuffer.MoveFront(sizeof(PACKET_HEADER));

		char* test = new char;
		Message msgBuffer;
		session->recvRingBuffer.Dequeue(msgBuffer.GetBufferPtr(), header.size);
		msgBuffer.MoveWritePos(header.size);

		PacketProc(session, header.type, &msgBuffer);
	}
}

void netProc_Disconnect()
{
	for (auto& disconPlayer : g_disconList)
	{
		if (disconPlayer == nullptr)
			continue;
		//sessionList���� �����ϱ�
		g_sessionList.remove(disconPlayer);

		closesocket(disconPlayer->socket);

		//����� �÷��̾��� ĳ���͸� �����ϵ��� �ٸ� �������� ��ε�ĳ�����ϱ�
		Message delCharMsg;
		g_packetGenerator.MakeDeleteCharacterPacket(&delCharMsg, disconPlayer->sessionID);

		SendBroadcast(nullptr, delCharMsg.GetBufferPtr(), delCharMsg.GetDataSize());

		delete disconPlayer;
		disconPlayer = nullptr;
	}
	g_disconList.clear();
}
void ReserveDisconnect(Session* session)
{
	//�ߺ� push����
	if (std::find(g_disconList.begin(), g_disconList.end(), session) == g_disconList.end())
	{
		wprintf(L"SessionID : %d / ���� ����!!\n", session->sessionID);
		g_disconList.push_back(session);
	}
}

bool PacketProc(Session* session, unsigned char type, Message* buffer)
{
	wprintf(L"Recv Packet!!\n");

	switch (type)
	{
	case PACKET_CS_MOVE_START:
		return netPacketProc_MoveStart(session, buffer);
		break;
	case PACKET_CS_MOVE_STOP:
		return netPacketProc_MoveStop(session, buffer);
		break;
	case PACKET_CS_ATTACK1:
		return netPacketProc_Attack1(session, buffer);
		break;
	case PACKET_CS_ATTACK2:
		return netPacketProc_Attack2(session, buffer);
		break;
	case PACKET_CS_ATTACK3:
		return netPacketProc_Attack3(session, buffer);
		break;
	}

	return TRUE;
}

bool netPacketProc_MoveStart(Session* session, Message* buffer)
{
	unsigned char direction;
	short x;
	short y;

	*buffer >> direction >> x >> y;

	wprintf(L"session ID : %d, packet Type : CS_MOVE_START_PACKET, Dir : %d, X: %d, Y: %d\n", 
		session->sessionID, direction, x, y);

	if (abs(session->x - x) > ERROR_RANGE ||
		abs(session->y - y) > ERROR_RANGE)
	{
		ReserveDisconnect(session);
		wprintf(L"���� ���� : �̵� ���� �ʰ�/ session->x : %d, session->y : %d / MsgX : %d / MsgY : %d\n",
			session->x, session->y, x, y);
		return false;
	}

	session->action = direction;

	//switch (moveStart->direction)
	//{
	//case PACKET_MOVE_DIR_RR:
	//case PACKET_MOVE_DIR_RU:
	//case PACKET_MOVE_DIR_RD:
	//	session->direction = PACKET_MOVE_DIR_RR;
	//	break;
	//case PACKET_MOVE_DIR_LL:
	//case PACKET_MOVE_DIR_LU:
	//case PACKET_MOVE_DIR_LD:
	//	session->direction = PACKET_MOVE_DIR_LL;
	//	break;
	//}
	session->direction = direction;
	session->x = x;
	session->y = y;

	Message moveStartMsg;
	g_packetGenerator.MakeMoveStartPacket(&moveStartMsg, session->sessionID, session->direction, session->x, session->y);

	SendBroadcast(session, moveStartMsg.GetBufferPtr(), moveStartMsg.GetDataSize());
	return true;

}
bool netPacketProc_MoveStop(Session* session, Message* buffer)
{
	unsigned char direction;
	short x;
	short y;
	*buffer >> direction >> x >> y;

	wprintf(L"session ID : %d, packet Type : CS_MOVE_STOP_PACKET, Dir : %d, X: %d, Y: %d\n",
		session->sessionID, direction, x, y);
	if (abs(session->x - x) > ERROR_RANGE ||
		abs(session->y - y) > ERROR_RANGE)
	{
		ReserveDisconnect(session);
		wprintf(L"���� ���� : �̵� ���� �ʰ�/ session->x : %d, session->y : %d / MsgX : %d / MsgY : %d\n",
			session->x, session->y, x, y);
		return false;
	}

	session->action = PACKET_MOVE_STOP;
	session->direction = direction;
	session->x = x;
	session->y = y;

	Message moveStopMsg;
	g_packetGenerator.MakeMoveStopPacket(&moveStopMsg, session->sessionID, session->direction, session->x, session->y);

	SendBroadcast(session, moveStopMsg.GetBufferPtr(), moveStopMsg.GetDataSize());
	return true;
}

bool netPacketProc_Attack1(Session* session, Message* buffer)
{
	unsigned char direction;
	short x;
	short y;

	*buffer >> direction >> x >> y;
	wprintf(L"session ID : %d, packet Type : CS_ATTACK1_PACKET, Dir : %d, X: %d, Y: %d\n",
		session->sessionID, direction, x, y);

	//���� ����
	session->direction = direction;

	//�ٸ� �����鿡�� ���ݸ���� ������ ��ε� ĳ���� �Ѵ�.
	Message attack1Msg;
	g_packetGenerator.MakeAttack1Packet(&attack1Msg, session->sessionID, session->direction, session->x, session->y);

	SendBroadcast(session, attack1Msg.GetBufferPtr(), attack1Msg.GetDataSize());

	Session* targetPlayer = nullptr;
	for (Session* target : g_sessionList)
	{
		if (target == session)
			continue;

		if (session->direction == PACKET_MOVE_DIR_LL)
		{
			if (session->x - ATTACK1_RANGE_X < target->x && target->x < session->x &&
				session->y - ATTACK1_RANGE_Y / 2 < target->y && target->y < session->y + ATTACK1_RANGE_Y / 2)
			{
					targetPlayer = target;
					break;
			}
		}
		else if (session->direction == PACKET_MOVE_DIR_RR)
		{
			if (session->x < target->x && target->x < session->x + ATTACK1_RANGE_X &&
				session->y - ATTACK1_RANGE_Y / 2 < target->y && target->y < session->y + ATTACK1_RANGE_Y / 2)
			{
					targetPlayer = target;
					break;
			}
		}
	}

	//�´� �÷��̾ ã�� ��� ��������Ŷ�� �����ش�.
	if (targetPlayer != nullptr)
	{
		targetPlayer->HP -= ATTACK1_DAMAGE;	//���� ������ �� ������ ������
		Message dmgMsg;
		g_packetGenerator.MakeDamagePacket(&dmgMsg, session->sessionID, targetPlayer->sessionID, targetPlayer->HP);

		SendBroadcast(nullptr, dmgMsg.GetBufferPtr(), dmgMsg.GetDataSize());
	}

	return true;
}
bool netPacketProc_Attack2(Session* session, Message* buffer)
{
	unsigned char direction;
	short x;
	short y;
	*buffer >> direction >> x >> y;
	wprintf(L"session ID : %d, packet Type : CS_ATTACK2_PACKET, Dir : %d, X: %d, Y: %d\n",
		session->sessionID, direction, x, y);

	//���� ����
	session->direction = direction;

	//�ٸ� �����鿡�� ���ݸ���� ������ ��ε� ĳ���� �Ѵ�.
	Message attack2Msg;
	g_packetGenerator.MakeAttack2Packet(&attack2Msg, session->sessionID, session->direction, session->x, session->y);

	SendBroadcast(session, attack2Msg.GetBufferPtr(), attack2Msg.GetDataSize());

	Session* targetPlayer = nullptr;
	for (Session* target : g_sessionList)
	{
		if (target == session)
			continue;

		if (session->direction == PACKET_MOVE_DIR_LL)
		{
			if (session->x - ATTACK2_RANGE_X < target->x && target->x < session->x &&
				session->y - ATTACK2_RANGE_Y / 2 < target->y && target->y < session->y + ATTACK2_RANGE_Y / 2)
			{
				targetPlayer = target;
				break;
			}
		}
		else if (session->direction == PACKET_MOVE_DIR_RR)
		{
			if (session->x < target->x && target->x < session->x + ATTACK2_RANGE_X &&
				session->y - ATTACK2_RANGE_Y / 2 < target->y && target->y < session->y + ATTACK2_RANGE_Y / 2)
			{
				targetPlayer = target;
				break;
			}
		}
	}

	//�´� �÷��̾ ã�� ��� ��������Ŷ�� �����ش�.
	if (targetPlayer != nullptr)
	{
		targetPlayer->HP -= ATTACK2_DAMAGE;	//���� ������ �� ������ ������
		Message dmgMsg;
		g_packetGenerator.MakeDamagePacket(&dmgMsg, session->sessionID, targetPlayer->sessionID, targetPlayer->HP);

		SendBroadcast(nullptr, dmgMsg.GetBufferPtr(), dmgMsg.GetDataSize());
	}
	return true;
}
bool netPacketProc_Attack3(Session* session, Message* buffer)
{
	unsigned char direction;
	short x;
	short y;
	*buffer >> direction >> x >> y;

	wprintf(L"session ID : %d, packet Type : CS_ATTACK3_PACKET, Dir : %d, X: %d, Y: %d\n",
		session->sessionID, direction, x, y);

	//���� ����
	session->direction = direction;

	//�ٸ� �����鿡�� ���ݸ���� ������ ��ε� ĳ���� �Ѵ�.
	Message attack3Msg;
	g_packetGenerator.MakeAttack3Packet(&attack3Msg, session->sessionID, session->direction, session->x, session->y);

	SendBroadcast(session, attack3Msg.GetBufferPtr(), attack3Msg.GetDataSize());

	Session* targetPlayer = nullptr;
	for (Session* target : g_sessionList)
	{
		if (target == session)
			continue;

		if (session->direction == PACKET_MOVE_DIR_LL)
		{
			if (session->x - ATTACK3_RANGE_X < target->x && target->x < session->x &&
				session->y - ATTACK3_RANGE_Y / 2 < target->y && target->y < session->y + ATTACK3_RANGE_Y / 2)
			{
				targetPlayer = target;
				break;
			}
		}
		else if (session->direction == PACKET_MOVE_DIR_RR)
		{
			if (session->x < target->x && target->x < session->x + ATTACK3_RANGE_X &&
				session->y - ATTACK3_RANGE_Y / 2 < target->y && target->y < session->y + ATTACK3_RANGE_Y / 2)
			{
				targetPlayer = target;
				break;
			}
		}
	}

	//�´� �÷��̾ ã�� ��� ��������Ŷ�� �����ش�.
	if (targetPlayer != nullptr)
	{
		targetPlayer->HP -= ATTACK3_DAMAGE;	//���� ������ �� ������ ������
		Message dmgMsg;
		g_packetGenerator.MakeDamagePacket(&dmgMsg, session->sessionID, targetPlayer->sessionID, targetPlayer->HP);

		SendBroadcast(nullptr, dmgMsg.GetBufferPtr(), dmgMsg.GetDataSize());
	}
	return true;
}
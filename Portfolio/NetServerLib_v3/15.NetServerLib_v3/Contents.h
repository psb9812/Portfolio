#pragma once
#include "Windows.h"
#include <vector>
#include <unordered_map>
#include "LockFreeQueue.h"

class IUser;
class Message;
class NetServer;

enum class InterThreadMessageType
{
	Enter,
	Disconnect,
};

class Contents
{
public:
	Contents(NetServer* server, INT32 fps, int contentsID);
	virtual ~Contents();

	//�� �������� ���� �� ȣ���ϴ� �Լ�
	void NotifyEnterContents(INT64 sessionID);
	//������ �������� �˷��ִ� �Լ�
	void NotifyDisconnectSession(INT64 sessionID);
	//�� ���������� ���� �� ȣ���ϴ� �Լ�
	//void LeaveContents(INT64 sessionID);

	void DisconnectInContents(INT64 sessionID);

	inline INT32 GetSessionNum() const { return static_cast<INT32>(_sessionIDContainer.size()); }
	inline INT32 GetUserNum() const { return static_cast<INT32>(_userContainer.size()); }
	inline INT32 GetFPS() const { return _FPS; }

	bool DeleteSessionID(INT64 sessionID);
	void InsertSessionID(INT64 sessionID);

	void EnqueueInterThreadQueue(Message* pMessage);
	void ProcessingInterThreadMessage(Message* pMessage);
	void HandleEnterContentsMessage(Message* pMessage);
	void HandleDisconnectSessionMessage(Message* pMessage);

public:
	/////////////////////////////
	// Event callback function
	/////////////////////////////
	virtual void OnRecv(INT64 sessionID, Message* pMessage) = 0;
	virtual void OnEnter(INT64 sessionID, IUser* pUser) = 0;
	virtual void OnLeave(INT64 sessionID) = 0;
	virtual void OnDisconnect(INT64 sessionID) = 0;
	virtual void OnUpdate() = 0;

	static unsigned int WINAPI ContentsThread(LPVOID pPrame);

protected:
	//�� ���������� �ٸ� �������� �̵��� �� ȣ���ϴ� �Լ�
	void ReserveMoveContents(INT64 sessionID, INT32 destContentsID);
	void ExitContents(INT64 sessionID);

private:
	void MoveContents();

private:
	std::vector<INT64> _sessionIDContainer;
	std::vector<INT64> _reserveMoveContentsSessionContainer;
	INT32 _FPS;
	HANDLE _hContentsThread;
	bool _isContentsThreadRun;
	const int _contentsID;

	//������ �����尣 ��� ť (���� ����, ���� �޽��� ����)
	LockFreeQueue<Message*> _interThreadQueue;
protected:
	NetServer* _pNetworkLib;
	std::unordered_map<INT64, IUser*> _userContainer;
public:
	LONG _eventCounter = 0;			//WaitOnAddress�� ���� ī���� ��
	LONG _prevFrameCnt = 0;
	LONG _frameCnt = 0;
};
#include "NetServer.h"
#include "Contents.h"
#include "algorithm"
#include <process.h>
#include "Message.h"
#include <synchapi.h>

Contents::Contents(NetServer* server, INT32 fps, int contentsID)
	:_pNetworkLib(server), _FPS(fps), _hContentsThread(INVALID_HANDLE_VALUE), _isContentsThreadRun(false), _contentsID(contentsID)
{
	_isContentsThreadRun = true;
	_hContentsThread = (HANDLE)_beginthreadex(NULL, 0, ContentsThread, reinterpret_cast<LPVOID>(this), 0, NULL);
}

Contents::~Contents()
{
	_isContentsThreadRun = false;
	WaitForSingleObject(_hContentsThread, INFINITE);
	CloseHandle(_hContentsThread);
}

void Contents::NotifyEnterContents(INT64 sessionID)
{
	Session* pSession = _pNetworkLib->AcquireSessionLicense(sessionID);
	if (pSession == nullptr)
	{
		return;
	}
	// ���⼭ IOCount�� �ϳ� �÷��� �ش� ������ �������� �ʵ��� �Ѵ�.
	// ������ ���������� ī��Ʈ�� ������.
	pSession->IncreaseIOCount();
	_pNetworkLib->ReleaseSessionLicense(pSession);

	//Enter �޽��� �����
	Message* pEnterContentsMessage = Message::Alloc();
	pEnterContentsMessage->AddRefCount();
	*pEnterContentsMessage << static_cast<short>(InterThreadMessageType::Enter) << sessionID;

	//���� ������ ť�� Enter �޽����� ��ť�Ѵ�.
	EnqueueInterThreadQueue(pEnterContentsMessage);
	//�ڱ� �ڽ� �����
	_pNetworkLib->AwakeContentsThread(_contentsID);
}

void Contents::NotifyDisconnectSession(INT64 sessionID)
{
	//Disconnect �޽��� �����
	Message* pDisconnectContentsMessage = Message::Alloc();
	pDisconnectContentsMessage->AddRefCount();
	*pDisconnectContentsMessage << static_cast<short>(InterThreadMessageType::Disconnect) << sessionID;

	//���� ������ ť�� Enter �޽����� ��ť�Ѵ�.
	EnqueueInterThreadQueue(pDisconnectContentsMessage);
	//�ڱ� �ڽ� �����
	_pNetworkLib->AwakeContentsThread(_contentsID);
}

void Contents::MoveContents()
{
	//������ �̵� �����̳ʿ� ����� ��� ������ �̵���Ų��.
	for (INT64 sessionID : _reserveMoveContentsSessionContainer)
	{
		ExitContents(sessionID);

		//������ ������ ID ���ϱ�
		Session* pSession = _pNetworkLib->AcquireSessionLicense(sessionID);
		if (pSession == nullptr)
		{
			return;
		}
		LONG destContentsID = pSession->_contentsID;
		_pNetworkLib->ReleaseSessionLicense(pSession);

		//Enter �޽��� �����
		Message* pEnterContentsMessage = Message::Alloc();
		pEnterContentsMessage->AddRefCount();
		*pEnterContentsMessage << static_cast<short>(InterThreadMessageType::Enter) << sessionID;

		//������ �������� ���� ������ ť�� Enter �޽����� ��ť�Ѵ�.
		_pNetworkLib->_contentsHashTable[destContentsID]->EnqueueInterThreadQueue(pEnterContentsMessage);

		_pNetworkLib->AwakeContentsThread(destContentsID);
	}

	_reserveMoveContentsSessionContainer.clear();
}

void Contents::ReserveMoveContents(INT64 sessionID, INT32 destContentsID)
{
	// ������ �Ҽ� �ٲٱ�, ���⼭ �Ҽ��� �ٲ�� �ܺ��� �޽��� ó�� ������ �������� �� �ִ�.
	Session* pSession = _pNetworkLib->AcquireSessionLicense(sessionID);
	if (pSession == nullptr)
	{
		return;
	}
	// ���⼭ IOCount�� �ϳ� �÷��� �ش� ������ �������� �ʵ��� �Ѵ�.
	// �÷ȴ� ī��Ʈ�� ������ ���������� ������.
	pSession->IncreaseIOCount();

	pSession->EnterContents(destContentsID);

	_pNetworkLib->ReleaseSessionLicense(pSession);

	//�̵� ���� �ڷᱸ���� sessionID�� �����Ѵ�.
	_reserveMoveContentsSessionContainer.push_back(sessionID);
}

void Contents::ExitContents(INT64 sessionID)
{
	if (!DeleteSessionID(sessionID))
	{
		__debugbreak();
	}

	OnLeave(sessionID);
}

void Contents::DisconnectInContents(INT64 sessionID)
{
	_pNetworkLib->Disconnect(sessionID);
}

bool Contents::DeleteSessionID(INT64 sessionID)
{
	for (auto iter = _sessionIDContainer.begin(); iter != _sessionIDContainer.end(); iter++)
	{
		if (*iter == sessionID)
		{
			_sessionIDContainer.erase(iter);
			return true;
		}
	}

	return false;
}

void Contents::InsertSessionID(INT64 sessionID)
{
	_sessionIDContainer.push_back(sessionID);
}

void Contents::EnqueueInterThreadQueue(Message* pMessage)
{
	_interThreadQueue.Enqueue(pMessage);
}

void Contents::ProcessingInterThreadMessage(Message* pMessage)
{
	short messageType;
	*pMessage >> messageType;

	switch (messageType)
	{
	case static_cast<int>(InterThreadMessageType::Enter):
		HandleEnterContentsMessage(pMessage);
		break;
	case static_cast<int>(InterThreadMessageType::Disconnect):
		HandleDisconnectSessionMessage(pMessage);
		break;
	default:
		break;
	}

	pMessage->SubRefCount();
}

void Contents::HandleEnterContentsMessage(Message* pMessage)
{
	INT64 sessionID;
	*pMessage >> sessionID;

	Session* pSession = _pNetworkLib->AcquireSessionLicense(sessionID);
	if (pSession == nullptr)
	{
		return;
	}
	//IOCount ������
	pSession->DecreaseIOCount();
	_pNetworkLib->ReleaseSessionLicense(pSession);

	InsertSessionID(sessionID);
	IUser* pUser = _pNetworkLib->GetUser(sessionID);

	OnEnter(sessionID, pUser);
}

void Contents::HandleDisconnectSessionMessage(Message* pMessage)
{
	INT64 sessionID;
	*pMessage >> sessionID;

	if (!DeleteSessionID(sessionID))
	{
		__debugbreak();
	}
	OnDisconnect(sessionID);
}

unsigned int __stdcall Contents::ContentsThread(LPVOID pParam)
{
	Contents* pContents = reinterpret_cast<Contents*>(pParam);
	NetServer* pNetServer = pContents->_pNetworkLib;
	DWORD prevTime = timeGetTime();
	DWORD timePerFrame = 1000 / pContents->_FPS;
	LONG compareAddress = 0;

	while (pContents->_isContentsThreadRun)
	{
		//�ƹ� �̺�Ʈ ���� ������ �����尡 �ھ� �����ϴ� ���� �����ϱ� ���� WaitOnAddress ���
		WaitOnAddress(&pContents->_eventCounter, &compareAddress, sizeof(compareAddress), timePerFrame);

		//���� ������ ť üũ
#pragma region Inter_Thread_Queue_Check
		//Inter Thread Queue üũ�ؼ� �޽��� ó��
		while (pContents->_interThreadQueue.GetSize() > 0)
		{
			auto opt_pMessage = pContents->_interThreadQueue.Dequeue();
			if (opt_pMessage.has_value())
			{
				pContents->ProcessingInterThreadMessage(opt_pMessage.value());
				InterlockedDecrement(&pContents->_eventCounter);
			}
		}
#pragma endregion
		//�޽��� ó��
#pragma region MessageProcessing
		//���� ����Ʈ ��ȸ�ϸ鼭 recvMessageQueue���� Message ������ OnRecv ȣ�����ֱ�
		for (INT64 sessionID : pContents->_sessionIDContainer)
		{
			Session* pSession = pNetServer->AcquireSessionLicense(sessionID);
			if (pSession == nullptr)
				continue;
			
			//recvMessageQueue�� �� ������ �޽��� ó���� ���ش�.
			while (pSession->_recvMessageQueue.GetSize() > 0)
			{
				auto opt_pMessage= pSession->_recvMessageQueue.Dequeue();
				if (opt_pMessage.has_value())
				{
					pContents->OnRecv(sessionID, opt_pMessage.value());
					InterlockedDecrement(&pContents->_eventCounter);
				}

				//������ �� �������� �Ҽӵ� �������� �Ǵ�.
				if (pSession->_contentsID != pContents->_contentsID)
					break;
			}

			pNetServer->ReleaseSessionLicense(pSession);
		}

		//�������� �̵��� ������ ���ǵ��� ó��
		pContents->MoveContents();
#pragma endregion

		//������ üũ
#pragma region FrameCheck
		DWORD deltaTime = timeGetTime() - prevTime;
		if (deltaTime < timePerFrame)
		{
			continue;
		}
		prevTime += timePerFrame;
#pragma endregion
		//update ȣ��
		pContents->OnUpdate();
	}

	return 0;
}

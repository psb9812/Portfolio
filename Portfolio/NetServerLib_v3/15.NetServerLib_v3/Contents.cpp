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
	// 여기서 IOCount를 하나 올려서 해당 세션이 삭제되지 않도록 한다.
	// 목적지 컨텐츠에서 카운트는 내린다.
	pSession->IncreaseIOCount();
	_pNetworkLib->ReleaseSessionLicense(pSession);

	//Enter 메시지 만들기
	Message* pEnterContentsMessage = Message::Alloc();
	pEnterContentsMessage->AddRefCount();
	*pEnterContentsMessage << static_cast<short>(InterThreadMessageType::Enter) << sessionID;

	//인터 스레드 큐에 Enter 메시지를 인큐한다.
	EnqueueInterThreadQueue(pEnterContentsMessage);
	//자기 자신 깨우기
	_pNetworkLib->AwakeContentsThread(_contentsID);
}

void Contents::NotifyDisconnectSession(INT64 sessionID)
{
	//Disconnect 메시지 만들기
	Message* pDisconnectContentsMessage = Message::Alloc();
	pDisconnectContentsMessage->AddRefCount();
	*pDisconnectContentsMessage << static_cast<short>(InterThreadMessageType::Disconnect) << sessionID;

	//인터 스레드 큐에 Enter 메시지를 인큐한다.
	EnqueueInterThreadQueue(pDisconnectContentsMessage);
	//자기 자신 깨우기
	_pNetworkLib->AwakeContentsThread(_contentsID);
}

void Contents::MoveContents()
{
	//컨텐츠 이동 컨테이너에 예약된 모든 세션을 이동시킨다.
	for (INT64 sessionID : _reserveMoveContentsSessionContainer)
	{
		ExitContents(sessionID);

		//목적지 컨텐츠 ID 구하기
		Session* pSession = _pNetworkLib->AcquireSessionLicense(sessionID);
		if (pSession == nullptr)
		{
			return;
		}
		LONG destContentsID = pSession->_contentsID;
		_pNetworkLib->ReleaseSessionLicense(pSession);

		//Enter 메시지 만들기
		Message* pEnterContentsMessage = Message::Alloc();
		pEnterContentsMessage->AddRefCount();
		*pEnterContentsMessage << static_cast<short>(InterThreadMessageType::Enter) << sessionID;

		//목적지 컨텐츠의 인터 스레드 큐에 Enter 메시지를 인큐한다.
		_pNetworkLib->_contentsHashTable[destContentsID]->EnqueueInterThreadQueue(pEnterContentsMessage);

		_pNetworkLib->AwakeContentsThread(destContentsID);
	}

	_reserveMoveContentsSessionContainer.clear();
}

void Contents::ReserveMoveContents(INT64 sessionID, INT32 destContentsID)
{
	// 컨텐츠 소속 바꾸기, 여기서 소속을 바꿔야 외부의 메시지 처리 루프를 빠져나갈 수 있다.
	Session* pSession = _pNetworkLib->AcquireSessionLicense(sessionID);
	if (pSession == nullptr)
	{
		return;
	}
	// 여기서 IOCount를 하나 올려서 해당 세션이 삭제되지 않도록 한다.
	// 올렸던 카운트는 목적지 컨텐츠에서 내린다.
	pSession->IncreaseIOCount();

	pSession->EnterContents(destContentsID);

	_pNetworkLib->ReleaseSessionLicense(pSession);

	//이동 예약 자료구조에 sessionID를 삽입한다.
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
	//IOCount 내리기
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
		//아무 이벤트 없는 컨텐츠 스레드가 코어 낭비하는 것을 방지하기 위한 WaitOnAddress 대기
		WaitOnAddress(&pContents->_eventCounter, &compareAddress, sizeof(compareAddress), timePerFrame);

		//인터 스레드 큐 체크
#pragma region Inter_Thread_Queue_Check
		//Inter Thread Queue 체크해서 메시지 처리
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
		//메시지 처리
#pragma region MessageProcessing
		//세션 리스트 순회하면서 recvMessageQueue에서 Message 꺼내서 OnRecv 호출해주기
		for (INT64 sessionID : pContents->_sessionIDContainer)
		{
			Session* pSession = pNetServer->AcquireSessionLicense(sessionID);
			if (pSession == nullptr)
				continue;
			
			//recvMessageQueue가 빌 때까지 메시지 처리를 해준다.
			while (pSession->_recvMessageQueue.GetSize() > 0)
			{
				auto opt_pMessage= pSession->_recvMessageQueue.Dequeue();
				if (opt_pMessage.has_value())
				{
					pContents->OnRecv(sessionID, opt_pMessage.value());
					InterlockedDecrement(&pContents->_eventCounter);
				}

				//여전히 이 컨텐츠에 소속된 세션인지 판단.
				if (pSession->_contentsID != pContents->_contentsID)
					break;
			}

			pNetServer->ReleaseSessionLicense(pSession);
		}

		//컨텐츠간 이동을 예약한 세션들을 처리
		pContents->MoveContents();
#pragma endregion

		//프레임 체크
#pragma region FrameCheck
		DWORD deltaTime = timeGetTime() - prevTime;
		if (deltaTime < timePerFrame)
		{
			continue;
		}
		prevTime += timePerFrame;
#pragma endregion
		//update 호출
		pContents->OnUpdate();
	}

	return 0;
}


#include "SessionContainer.h"

SessionContainer::SessionContainer()
	:_sessionArray(nullptr), _currentUserNum(0), _maxUserNum(0) {}

SessionContainer::~SessionContainer()
{
	delete[] _sessionArray;
	_sessionArray = nullptr;
}

void SessionContainer::InitContainer(int numOfMaxUser)
{
	_maxUserNum = numOfMaxUser;
	_sessionArray = new Session[_maxUserNum];

	for (int i = 0; i < _maxUserNum; i++)
		_insertIndexStack.Push(i);

	_currentUserNum = 0;
}

Session* SessionContainer::SearchSession(__int64 sessionID)
{
	//sessionID로 인덱스 찾기
	USHORT targetIndex = sessionID >> ID_BITWIDE;

	if (_sessionArray[targetIndex]._isEmptyIndex)
		return nullptr;
	else
		return &_sessionArray[targetIndex];
}

void SessionContainer::DeleteSession(__int64 sessionID)
{
	USHORT targetIndex = sessionID >> ID_BITWIDE;

	Session& targetSession = _sessionArray[targetIndex];

	InterlockedExchange(&targetSession._isEmptyIndex, TRUE);

	_insertIndexStack.Push(targetIndex);

	InterlockedDecrement(&_currentUserNum);
}

Session* SessionContainer::InsertSession(__int64 ID, SOCKET clientSock, IN_ADDR clientIP, int clientPort)
{
	USHORT insertIndex = _insertIndexStack.Pop().value();

	//인덱스 정보와 ID 정보를 비트 단위 결합해서 새로운 세션 ID를 만든다.
	__int64 sessionID = MakeSessionID(ID, insertIndex);

	//아직 비어있지 않은 세션 자리에 넣을 수는 없으므로 false를 리턴해서 알린다.
	if (_sessionArray[insertIndex]._isEmptyIndex == false)
	{
		//꺼냈던 것 다시 넣기
		_insertIndexStack.Push(insertIndex);
		return nullptr;
	}

	_sessionArray[insertIndex].LaundrySession(sessionID, clientSock, clientIP, clientPort);

	InterlockedIncrement(&_currentUserNum);
	return &_sessionArray[insertIndex];
}

__int64	SessionContainer::MakeSessionID(__int64 ID, USHORT insertIdx)
{
	__int64 index = static_cast<__int64>(insertIdx);
	index = index << ID_BITWIDE;	//인덱스 정보를 6 바이트 뒤로 시프트.
	__int64 sessionID = ID | index;

	return sessionID;
}

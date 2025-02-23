
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

Session* SessionContainer::SearchSession(INT64 sessionID)
{
	//sessionID�� �ε��� ã��
	USHORT targetIndex = sessionID >> ID_BITWIDE;

	if (_sessionArray[targetIndex]._isEmptyIndex)
		return nullptr;
	else
		return &_sessionArray[targetIndex];
}

void SessionContainer::DeleteSession(INT64 sessionID)
{
	//�ε��� ����
	USHORT targetIndex = sessionID >> ID_BITWIDE;

	Session& targetSession = _sessionArray[targetIndex];
	//����ִ� index���� ǥ��
	InterlockedExchange(&targetSession._isEmptyIndex, TRUE);
	//����� �� �������Ƿ� index ���ÿ� �ٽ� �ݳ��ϱ�
	_insertIndexStack.Push(targetIndex);

	InterlockedDecrement(&_currentUserNum);
}

Session* SessionContainer::InsertSession(INT64 ID, SOCKET clientSock, IN_ADDR clientIP, int clientPort)
{
	USHORT insertIndex = _insertIndexStack.Pop().value();

	//�ε��� ������ ID ������ ��Ʈ ���� �����ؼ� ���ο� ���� ID�� �����.
	INT64 sessionID = MakeSessionID(ID, insertIndex);

	//���� ������� ���� ���� �ڸ��� ���� ���� �����Ƿ� false�� �����ؼ� �˸���.
	if (_sessionArray[insertIndex]._isEmptyIndex == false)
	{
		//���´� �� �ٽ� �ֱ�
		_insertIndexStack.Push(insertIndex);
		return nullptr;
	}

	//������ ��Ȱ���ϱ� ���� ���� �����͸� ��Ź�Ѵ�.
	_sessionArray[insertIndex].LaundrySession(sessionID, clientSock, clientIP, clientPort);

	InterlockedIncrement(&_currentUserNum);
	return &_sessionArray[insertIndex];
}

INT64	SessionContainer::MakeSessionID(INT64 ID, USHORT insertIdx)
{
	INT64 index = static_cast<INT64>(insertIdx);
	index = index << ID_BITWIDE;	//�ε��� ������ 6 ����Ʈ �ڷ� ����Ʈ.
	INT64 sessionID = ID | index;

	return sessionID;
}

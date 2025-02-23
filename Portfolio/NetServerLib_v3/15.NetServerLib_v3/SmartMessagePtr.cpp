#include "SmartMessagePtr.h"


SmartMessagePtr::SmartMessagePtr(Message* pMsg)
	:_pMessage(pMsg)
{
	_pMessage->AddRefCount();
}

SmartMessagePtr::~SmartMessagePtr()
{
	_pMessage->SubRefCount();
}

SmartMessagePtr::SmartMessagePtr(const SmartMessagePtr& other)
{
	_pMessage = other._pMessage;
	_pMessage->AddRefCount();
}

SmartMessagePtr& SmartMessagePtr::operator=(const SmartMessagePtr& other)
{
	//�ڽ� ���� ó��
	if (this == &other)
	{
		return *this;
	}

	_pMessage = other._pMessage;
	_pMessage->AddRefCount();
	return *this;
}

Message& SmartMessagePtr::operator*()
{
	return *_pMessage;
}

Message& SmartMessagePtr::operator*() const
{
	return *_pMessage;
}

Message* SmartMessagePtr::operator->()
{
	return _pMessage;
}

Message* SmartMessagePtr::operator->() const
{
	return _pMessage;
}

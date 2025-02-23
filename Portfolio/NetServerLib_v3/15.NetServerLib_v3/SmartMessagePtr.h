#pragma once

#include "Message.h"

class SmartMessagePtr
{
private:
	Message* _pMessage;
public:
	SmartMessagePtr(Message* pMsg);
	~SmartMessagePtr();
	SmartMessagePtr(const SmartMessagePtr& other);
	SmartMessagePtr& operator=(const SmartMessagePtr& other);

	Message& operator*();
	Message& operator*() const;
	Message* operator->();
	Message* operator->() const;
};


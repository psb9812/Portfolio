#pragma once

class Message;

enum class en_JobType
{
	JOB_DEFAULT,
	JOB_ACCEPT,
	JOB_RELEASE,
	JOB_RECV,
	JOB_TIMER,
	JOB_TERMINATE
};

class Job
{
public:
	Job()
		:_type(en_JobType::JOB_DEFAULT), _sessionID(0), _pMessage(nullptr) {}
	~Job() = default;

	void Set(en_JobType type, __int64 sessionID, Message* pMessage)
	{
		_type = type;
		_sessionID = sessionID;
		_pMessage = pMessage;
	}

public:
	en_JobType _type;
	__int64 _sessionID;
	Message* _pMessage;
};
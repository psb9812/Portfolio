#pragma once
#include <Windows.h>

enum Operation
{
	ERR_DECODE,
	ERR_LOGIN,
	ERR_SECTORMOVE,
	ERR_MESSAGE,
	RELEASE
};

const int arrLen = 100000;

class MemoryLog
{

	struct LogData
	{
		DWORD threadID;
		Operation operation;
		LONGLONG sessionID;
	};

public:
	unsigned long long WriteMemoryLog(Operation op, LONGLONG sessionID)
	{
		UINT64 ret = InterlockedIncrement64(&logCount);

		UINT64 idx = ret % arrLen;

		logArr[idx].threadID = GetCurrentThreadId();
		logArr[idx].operation = op;
		logArr[idx].sessionID = sessionID;

		return ret;
	}

	LogData logArr[arrLen] = { 0, };
	long long logCount = -1;
};
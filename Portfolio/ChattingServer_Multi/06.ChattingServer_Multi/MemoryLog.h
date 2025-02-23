#pragma once
#include <Windows.h>

enum Operation
{
	SLOCK,
	SUNLOCK,
	ELOCK,
	EUNLOCK
};

const int arrLen = 100000;

class MemoryLog
{

	struct LogData
	{
		DWORD threadID;
		Operation operation;
		short x;
		short y;
	};

public:
	unsigned long long WriteMemoryLog(Operation op, short x, short y)
	{
		UINT64 ret = InterlockedIncrement64(&logCount);

		UINT64 idx = ret % arrLen;

		logArr[idx].threadID = GetCurrentThreadId();
		logArr[idx].operation = op;
		logArr[idx].x = x;
		logArr[idx].y = y;

		return ret;
	}

	LogData logArr[arrLen] = { 0, };
	long long logCount = -1;
};
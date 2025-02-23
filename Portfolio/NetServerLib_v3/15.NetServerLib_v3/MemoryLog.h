#pragma once
#include <Windows.h>

enum Operation
{
	SendMessagePerSendPost,
};

const int arrLen = 100000;

class MemoryLog
{

	struct LogData
	{
		DWORD threadID;
		Operation operation;
		LONGLONG sessionID;
		int data;
	};

public:
	unsigned long long WriteMemoryLog(Operation op, LONGLONG sessionID, int data)
	{
		UINT64 ret = InterlockedIncrement64(&logCount);

		UINT64 idx = ret % arrLen;

		logArr[idx].threadID = GetCurrentThreadId();
		logArr[idx].operation = op;
		logArr[idx].sessionID = sessionID;
		logArr[idx].data = data;

		return ret;
	}

	LogData logArr[arrLen] = { 0, };
	long long logCount = -1;
};
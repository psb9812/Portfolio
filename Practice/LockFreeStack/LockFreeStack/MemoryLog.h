#pragma once
#include <Windows.h>
#include "LockFreeStack.h"

enum Operation
{
	PUSH,
	POP
};

const int arrLen = 100000;

class MemoryLog
{

	struct LogData
	{
		DWORD threadID;
		Operation operation;
		long long logCount;
		void* localTop;
		void* localTopNext;
		void* newTop;
		void* newTopNext;
		void* RealTopNext;
		LONG size;
	};

public:
	unsigned long long WriteMemoryLog(Operation op, void* newTop, void* newTopNext, void* localTop, void* localTopNext, void* realTopNext, LONG size)
	{
		UINT64 ret = InterlockedIncrement64(&logCount);

		UINT64 idx = ret % arrLen;

		logArr[idx].threadID = GetCurrentThreadId();
		logArr[idx].operation = op;
		logArr[idx].logCount = ret;
		logArr[idx].newTop = newTop;
		logArr[idx].localTop = localTop;
		logArr[idx].RealTopNext = realTopNext;
		logArr[idx].size = size;
		logArr[idx].localTopNext = localTopNext;
		logArr[idx].newTopNext = newTopNext;

		return ret;
	}

	LogData logArr[arrLen];
	long long logCount = -1;
};


#pragma once
#include <Windows.h>
#include "LockFreeQueue.h"

enum Operation
{
	Enqueue_CAS1,
	Enqueue_CAS2_Success,
	Enqueue_CAS2_fail,
	Enqueue_Tail_manipulrate,
	Dequeue
};


class MemoryLog
{
	//로그 구조체
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
	const int arrLen = 100000;			//로그 배열 크기
	LogData logArr[arrLen] = { 0, };	//로그 배열
	long long logCount = -1;			//로그 카운트

public:
	unsigned long long WriteMemoryLog(Operation op, void* newTop, void* newTopNext,
		void* localTop, void* localTopNext, void* realTopNext, LONG size)
	{
		//로그 카운트 증가
		UINT64 retLogCount = InterlockedIncrement64(&logCount);
		UINT64 idx = retLogCount % arrLen;
		//로그 데이터 저장
		logArr[idx].threadID = GetCurrentThreadId();
		logArr[idx].operation = op;
		logArr[idx].logCount = retLogCount;
		logArr[idx].newTop = newTop;
		logArr[idx].localTop = localTop;
		logArr[idx].RealTopNext = realTopNext;
		logArr[idx].size = size;
		logArr[idx].localTopNext = localTopNext;
		logArr[idx].newTopNext = newTopNext;
		//로그 카운터를 반환해서 logArr에서 살펴봐야할 인덱스를 파악할 수 있게한다.
		return retLogCount;
	}
};
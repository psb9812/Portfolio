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
	//�α� ����ü
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
	const int arrLen = 100000;			//�α� �迭 ũ��
	LogData logArr[arrLen] = { 0, };	//�α� �迭
	long long logCount = -1;			//�α� ī��Ʈ

public:
	unsigned long long WriteMemoryLog(Operation op, void* newTop, void* newTopNext,
		void* localTop, void* localTopNext, void* realTopNext, LONG size)
	{
		//�α� ī��Ʈ ����
		UINT64 retLogCount = InterlockedIncrement64(&logCount);
		UINT64 idx = retLogCount % arrLen;
		//�α� ������ ����
		logArr[idx].threadID = GetCurrentThreadId();
		logArr[idx].operation = op;
		logArr[idx].logCount = retLogCount;
		logArr[idx].newTop = newTop;
		logArr[idx].localTop = localTop;
		logArr[idx].RealTopNext = realTopNext;
		logArr[idx].size = size;
		logArr[idx].localTopNext = localTopNext;
		logArr[idx].newTopNext = newTopNext;
		//�α� ī���͸� ��ȯ�ؼ� logArr���� ��������� �ε����� �ľ��� �� �ְ��Ѵ�.
		return retLogCount;
	}
};
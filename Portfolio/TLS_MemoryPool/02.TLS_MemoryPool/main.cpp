#include <iostream>
#include <Windows.h>
#include <process.h>
#include "TLSMemoryPool.h"

constexpr int THREAD_NUM = 5;

constexpr int TEST_NUM = 10000;

UINT64 g_initValue = 0;

UINT64 g_allocCount = 0;

struct Data
{
	UINT64 data;

	Data()
		:data(0)
	{
		data = 0;
	}
	Data(int val)
		:data(val)
	{

	}
};
Data* g_dataPointerArr[TEST_NUM];



TLSMemoryPool<Data> g_memoryPool(0, false);

unsigned int WINAPI ThreadProc(LPVOID pParam)
{
	for (int i = 0; i < TEST_NUM / THREAD_NUM; i++)
	{
		//Alloc 횟수가 TEST_NUM보다 크다면 스레드를 종료시킨다.
		UINT64 localAllocCount = InterlockedIncrement(&g_allocCount);
		UINT64 dataPointerArrIdx = (localAllocCount - 1);
		if (localAllocCount > TEST_NUM)
		{
			break;
		}

		Data* pData = g_memoryPool.Alloc();
		g_dataPointerArr[dataPointerArrIdx] = pData;
		if (pData == nullptr)
		{
			__debugbreak();
		}

		InterlockedIncrement(&(pData->data));

		//해당 스레드가 할당 받은 메모리를 좀 더 오래 들고 있도록 유도한다.
		Sleep(0);
		Sleep(0);
		Sleep(0);
	}


	return 0;
}

int wmain()
{
	while (true)
	{
		//초기값 세팅
		for (int i = 0; i < TEST_NUM; i++)
		{
			g_dataPointerArr[i] = g_memoryPool.Alloc();
			g_dataPointerArr[i]->data = g_initValue;
		}

		//전부 다 프리
		for (int i = 0; i < TEST_NUM; i++)
		{
			g_memoryPool.Free(g_dataPointerArr[i]);
			g_dataPointerArr[i] = nullptr;
		}

		// 스레드 생성
		HANDLE threads[THREAD_NUM] = { 0, };

		for (int i = 0; i < THREAD_NUM; i++)
		{
			threads[i] = (HANDLE)_beginthreadex(nullptr, 0, ThreadProc, nullptr, CREATE_SUSPENDED, nullptr);
		}

		for (int i = 0; i < THREAD_NUM; i++)
		{
			ResumeThread(threads[i]);
		}

		//모든 스레드가 종료될 때까지 대기
		WaitForMultipleObjects(THREAD_NUM, threads, TRUE, INFINITE);

		for (int i = 0; i < THREAD_NUM; i++)
		{
			CloseHandle(threads[i]);
		}

		//예상값과 data값이 같으지 체크 및 해제
		for (int i = 0; i < TEST_NUM; i++)
		{
			if (g_dataPointerArr[i] == nullptr)
			{
				__debugbreak();
			}
			bool isValidData = g_dataPointerArr[i]->data == g_initValue + 1;
			if (isValidData)
			{
				g_memoryPool.Free(g_dataPointerArr[i]);
			}
			else
			{
				__debugbreak();
			}
		}

		//메모리 풀 용량과 사용량 체크
		bool capacityCheck = g_memoryPool.GetCapacityCount() == TEST_NUM/100;
		if (!capacityCheck)
		{
			printf("Capacity Error : %d\n", g_memoryPool.GetCapacityCount());
		}
		bool useCountCheck = g_memoryPool.GetUseCount() == 0;
		if (!useCountCheck)
		{
			printf("UseCount Error : %d\n", g_memoryPool.GetUseCount());
		}

		//문제 없음 출력
		printf("No Problem / InitValue : %lld, Next initValue : %lld\n", g_initValue, g_initValue + 1);

		//초기값 증가된 걸로 세팅.
		g_initValue++;

		g_allocCount = 0;

		//전역 배열 비워주기
		for (int i = 0; i < TEST_NUM; i++)
		{
			g_dataPointerArr[i] = nullptr;
		}
	}
}
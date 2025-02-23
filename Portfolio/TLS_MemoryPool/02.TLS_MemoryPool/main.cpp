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
		//Alloc Ƚ���� TEST_NUM���� ũ�ٸ� �����带 �����Ų��.
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

		//�ش� �����尡 �Ҵ� ���� �޸𸮸� �� �� ���� ��� �ֵ��� �����Ѵ�.
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
		//�ʱⰪ ����
		for (int i = 0; i < TEST_NUM; i++)
		{
			g_dataPointerArr[i] = g_memoryPool.Alloc();
			g_dataPointerArr[i]->data = g_initValue;
		}

		//���� �� ����
		for (int i = 0; i < TEST_NUM; i++)
		{
			g_memoryPool.Free(g_dataPointerArr[i]);
			g_dataPointerArr[i] = nullptr;
		}

		// ������ ����
		HANDLE threads[THREAD_NUM] = { 0, };

		for (int i = 0; i < THREAD_NUM; i++)
		{
			threads[i] = (HANDLE)_beginthreadex(nullptr, 0, ThreadProc, nullptr, CREATE_SUSPENDED, nullptr);
		}

		for (int i = 0; i < THREAD_NUM; i++)
		{
			ResumeThread(threads[i]);
		}

		//��� �����尡 ����� ������ ���
		WaitForMultipleObjects(THREAD_NUM, threads, TRUE, INFINITE);

		for (int i = 0; i < THREAD_NUM; i++)
		{
			CloseHandle(threads[i]);
		}

		//���󰪰� data���� ������ üũ �� ����
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

		//�޸� Ǯ �뷮�� ��뷮 üũ
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

		//���� ���� ���
		printf("No Problem / InitValue : %lld, Next initValue : %lld\n", g_initValue, g_initValue + 1);

		//�ʱⰪ ������ �ɷ� ����.
		g_initValue++;

		g_allocCount = 0;

		//���� �迭 ����ֱ�
		for (int i = 0; i < TEST_NUM; i++)
		{
			g_dataPointerArr[i] = nullptr;
		}
	}
}
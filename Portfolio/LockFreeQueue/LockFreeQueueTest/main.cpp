#include <iostream>
#include <Windows.h>
#include <process.h>
#include "LockFreeQueue.h"

constexpr int THREAD_NUM = 2;
constexpr int LOOP_NUM = 3;

INT64 g_counter = 0;
LockFreeQueue<int> g_Queue;

unsigned int WINAPI ThreadProc(LPVOID param)
{
	while (1)
	{
		for (int i = 0; i < LOOP_NUM; i++)
		{
			g_Queue.Enqueue(i);
		}
		for (int i = 0; i < LOOP_NUM; i++)
		{
			int ret;
			g_Queue.Dequeue(ret);
			if (ret == -1)
			{
				__debugbreak();
			}
		}
	}
	return 0;
}

unsigned int WINAPI DebugProc(LPVOID param)
{
	while (1)
	{
		wprintf(L"Size : %d\n", g_Queue.GetSize());
		Sleep(1000);
	}
}

int main()
{
	HANDLE threads[THREAD_NUM] = { 0, };
	HANDLE debugThread = INVALID_HANDLE_VALUE;
	for (int i = 0; i < THREAD_NUM; i++)
	{
		threads[i] = (HANDLE)_beginthreadex(nullptr, 0, ThreadProc, nullptr, CREATE_SUSPENDED, nullptr);
	}

	for (int i = 0; i < THREAD_NUM; i++)
	{
		ResumeThread(threads[i]);
	}

	debugThread = (HANDLE)_beginthreadex(nullptr, 0, DebugProc, nullptr, 0, nullptr);

	WaitForMultipleObjects(THREAD_NUM, threads, TRUE, INFINITE);
	printf("³¡\n");
}
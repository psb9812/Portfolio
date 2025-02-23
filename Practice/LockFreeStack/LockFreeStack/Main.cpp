#include <iostream>
#include <Windows.h>
#include <process.h>
#include "LockFreeStack.h"

constexpr int THREAD_NUM = 5;
constexpr int LOOP_NUM = 1000;

INT64 g_counter = 0;
LockFreeStack<int> g_Stack;

unsigned int WINAPI ThreadProc(LPVOID param)
{
	while (1)
	{
		for (int i = 0; i < LOOP_NUM; i++)
		{
			g_Stack.Push(i);
		}
		for (int i = 0; i < LOOP_NUM; i++)
		{
			int val = g_Stack.Pop();
		}

		printf("%lld\n", InterlockedIncrement64(&g_counter));
	}
	return 0;
}

int main()
{
	HANDLE threads[THREAD_NUM] = { 0, };
	for (int i = 0; i < THREAD_NUM; i++)
	{
		threads[i] = (HANDLE)_beginthreadex(nullptr, 0, ThreadProc, nullptr, CREATE_SUSPENDED, nullptr);
	}

	for (int i = 0; i < THREAD_NUM; i++)
	{
		ResumeThread(threads[i]);
	}

	WaitForMultipleObjects(THREAD_NUM, threads, TRUE, INFINITE);
	printf("³¡\n");
}
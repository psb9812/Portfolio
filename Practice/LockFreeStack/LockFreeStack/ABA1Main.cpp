#include <iostream>
#include <Windows.h>
#include <process.h>
#include "LockFreeStack.h"

constexpr int THREAD_NUM = 4;
constexpr int LOOP_NUM = 2;

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
			g_Stack.Pop();
		}
	}
	return 0;
}

int main()
{
	HANDLE threads[THREAD_NUM];
	for (int i = 0; i < THREAD_NUM; i++)
	{
		threads[i] = (HANDLE)_beginthreadex(nullptr, 0, ThreadProc, nullptr, CREATE_SUSPENDED, nullptr);
	}

	for (int i = 0; i < THREAD_NUM; i++)
	{
		ResumeThread(threads[i]);
	}

	WaitForMultipleObjects(THREAD_NUM, threads, TRUE, INFINITE);
}
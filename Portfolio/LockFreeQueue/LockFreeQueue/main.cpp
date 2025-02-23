#include <iostream>
#include <Windows.h>
#include <process.h>
#include <conio.h>
#include "LockFreeQueue.h"
#include "LockQueue.h"
#define PROFILE
#include "Profiler.h"

constexpr int THREAD_NUM = 8;
const WCHAR* textName = L"SRW_Test_Thread8.txt";
constexpr int LOOP_NUM = 200;

INT64 g_counter = 0;
//LockFreeQueue<int> g_Queue(LOOP_NUM + 1);
LockQueue<int> g_Queue;
bool g_isThreadRun = true;


unsigned int WINAPI ThreadProc(LPVOID param)
{
	while (g_isThreadRun)
	{
		for (int i = 0; i < LOOP_NUM; i++)
		{
			START_PROFILEING(enque, "SRW_Enque");
			g_Queue.Enqueue(i);
		}
		
		for (int i = 0; i < LOOP_NUM; i++)
		{
			START_PROFILEING(deque, "SRW_Deque");
			g_Queue.Dequeue();
			//auto ret = g_Queue.Dequeue();

			//if (ret.has_value())
			//{
			//	__debugbreak();
			//}
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

	//debugThread = (HANDLE)_beginthreadex(nullptr, 0, DebugProc, nullptr, 0, nullptr);

	//키보드 입력 받기
	while (true)
	{
		if (_kbhit())
		{
			char input = _getch();

			if (input == 'r' || input == 'R')
			{
				//데이터 리셋
				ProfileManager::ProfileReset();
			}
			else if (input == 'w' || input == 'W')
			{
				//데이터 쓰기
				ProfileManager::ProfileDataOutText(textName);
			}
			else if (input == 'q' || input == 'Q')
			{
				//종료
				g_isThreadRun = false;
				break;
			}
		}

		Sleep(1000);
	}

	WaitForMultipleObjects(THREAD_NUM, threads, TRUE, INFINITE);
	wprintf(L"size : %d\n", g_Queue.GetSize());
	//ProfileManager::ProfileDataOutText(L"Spin_Enqueue_Test_Thread16_3.txt");
	//ProfileManager::ProfileDataOutText(L"LockFree_Enqueue_Test_Thread16_3.txt");
	//ProfileManager::ProfileDataOutText(L"SRW_Test_Thread1.txt");
	//ProfileManager::ProfileDataOutText(L"Spin_Dequeue_Test_Thread16_1.txt");
	//ProfileManager::ProfileDataOutText(L"LockFree_Dequeue_Test_Thread16_3.txt");
	//ProfileManager::ProfileDataOutText(L"SRW_Dequeue_Test_Thread16_3.txt");
}
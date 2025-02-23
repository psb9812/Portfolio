#include <iostream>
#include <Windows.h>
#include <process.h>
#include "SystemLog.h"
using namespace std;

constexpr int THREAD_NUM = 3;
psbe_en::SystemLog* pG_log;

unsigned int WINAPI ThreadFunc(LPVOID lpParam)
{
	int value = *reinterpret_cast<int*>(lpParam);

	for (size_t i = 0; i < 3; i++)
	{
		switch (value)
		{
		case 0:
			pG_log->WriteLog(L"Bettle", psbe_en::SystemLog::LOG_LEVEL_DEBUG, L"배틀 테스트 로그 %d", value);
			break;
		case 1:
			pG_log->WriteLog(L"CHAT", psbe_en::SystemLog::LOG_LEVEL_SYSTEM, L"채팅 테스트 로그 %d", value);
			break;
		case 2:
			pG_log->WriteLog(L"BOSS", psbe_en::SystemLog::LOG_LEVEL_ERROR, L"보스 테스트 로그 %d", value);
			break;
		default:
			break;
		}
	}

	return 0;
}


struct Man
{
	int height;
	int age;
};


int main()
{
	setlocale(LC_ALL, "");
	pG_log = psbe_en::SystemLog::GetInstance();

	//로그 세팅
	pG_log->SetLogDirectory(L"TestLogDir");
	pG_log->SetLogLevel(psbe_en::SystemLog::LOG_LEVEL_ERROR);

	//HANDLE hThreads[THREAD_NUM] = { 0, };
	//int values[THREAD_NUM] = { 0, 1, 2 };
	//for (int i = 0; i < THREAD_NUM; i++)
	//{
	//	hThreads[i] = (HANDLE)_beginthreadex(NULL, 0, ThreadFunc, (void*)&values[i], 0, NULL);
	//}

	//WaitForMultipleObjects(THREAD_NUM, hThreads, TRUE, INFINITE);

	
	Man psb;

	psb.age = 24;
	psb.height = 180;
	
	pG_log->WriteLogHex(L"HEX", psbe_en::SystemLog::LOG_LEVEL_ERROR, L"Man 객체 : ", (BYTE*)&psb, sizeof(psb));

	pG_log->WriteLogConsole(psbe_en::SystemLog::LOG_LEVEL_SYSTEM, L"skldjflkasdf\n", psb.age, psb.height);

	return 0;
}
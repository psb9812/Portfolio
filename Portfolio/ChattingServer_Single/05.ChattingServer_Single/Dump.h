#pragma once
#pragma comment(lib, "DbgHelp.lib")
#include <Windows.h>
#include <DbgHelp.h>
#include <crtdbg.h>
#include <Psapi.h>
#include <iostream>

class Dump
{
public:
	Dump()
	{
		_dumpCount = 0;

		_invalid_parameter_handler oldHandler, newHandler;
		newHandler = myInvalidParameterHandler;

		oldHandler = _set_invalid_parameter_handler(newHandler);		//crt �Լ��� null ������ ���� �־��� ��..
		_CrtSetReportMode(_CRT_WARN, 0);								//CRT ���� �޽��� ǥ�� �ߴ�. �ٷ� ���� ������ ����
		_CrtSetReportMode(_CRT_ASSERT, 0);								//CRT ���� �޽��� ǥ�� �ߴ�. �ٷ� ���� ������ ����
		_CrtSetReportMode(_CRT_ERROR, 0);								//CRT ���� �޽��� ǥ�� �ߴ�. �ٷ� ���� ������ ����

		_CrtSetReportHook(_custom_Report_hook);

		_set_purecall_handler(myPurecallHandler);

		SetHandlerDump();
	}
	~Dump()
	{

	}

	static void Crash(void)
	{
		int* p = nullptr;
		*p = 0;
	}

	static LONG WINAPI MyExceptionFilter(__in PEXCEPTION_POINTERS pExceptionPointer)
	{
		int			workingMemory = 0;
		SYSTEMTIME	nowTime;

		long dumpCount = InterlockedIncrement(&_dumpCount);

		//----------------------------------------------
		//���� ���μ����� �޸� ��뷮�� ���´�.
		//----------------------------------------------
		HANDLE hProcess = 0;
		PROCESS_MEMORY_COUNTERS pmc;

		hProcess = GetCurrentProcess();

		if (hProcess == NULL)
		{
			return 0;
		}

		if (GetProcessMemoryInfo(hProcess, &pmc, sizeof(pmc)))
		{
			workingMemory = (int)(pmc.WorkingSetSize / 1024 / 1024);
		}
		CloseHandle(hProcess);

		//----------------------------------------------
		//���� ��¥�� �ð��� �˾ƿ´�.
		//----------------------------------------------
		WCHAR filename[MAX_PATH];

		GetLocalTime(&nowTime);
		wsprintf(filename, L"Dump_%d%02d%02d_%02d.%02d.%02d_%d.dmp",
			nowTime.wYear, nowTime.wMonth, nowTime.wDay, nowTime.wHour, nowTime.wMinute, nowTime.wSecond, dumpCount);

		wprintf(L"\n\n\n!!! Crash Error !!! %d.%d.%d / %d:%d:%d \n",
			nowTime.wYear, nowTime.wMonth, nowTime.wDay, nowTime.wHour, nowTime.wMinute, nowTime.wSecond);
		wprintf(L"Now Save dump file...\n");

		HANDLE hDumpFile = ::CreateFile(filename,
			GENERIC_WRITE,
			FILE_SHARE_WRITE,
			NULL,
			CREATE_ALWAYS,
			FILE_ATTRIBUTE_NORMAL,
			NULL);

		if (hDumpFile != INVALID_HANDLE_VALUE)
		{
			_MINIDUMP_EXCEPTION_INFORMATION MinidumpExceptionInformation;

			MinidumpExceptionInformation.ThreadId = ::GetCurrentThreadId();
			MinidumpExceptionInformation.ExceptionPointers = pExceptionPointer;
			MinidumpExceptionInformation.ClientPointers = TRUE;

			MiniDumpWriteDump(GetCurrentProcess(),
				GetCurrentProcessId(),
				hDumpFile,
				MiniDumpWithFullMemory,
				&MinidumpExceptionInformation,
				NULL,
				NULL);
			CloseHandle(hDumpFile);

			wprintf(L"CrashDump Save Finish!");
		}
		return EXCEPTION_EXECUTE_HANDLER;
	}

	static void SetHandlerDump()
	{
		SetUnhandledExceptionFilter(MyExceptionFilter);
	}

	//invalid parameter handler
	static void myInvalidParameterHandler(const wchar_t* expression, const wchar_t* function, const wchar_t* file, unsigned int line, uintptr_t pReserved)
	{
		Crash();
	}

	static int _custom_Report_hook(int reposType, char* message, int* returnValue)
	{
		Crash();
		return true;
	}

	static void myPurecallHandler(void)
	{
		Crash();
	}

private:
	inline static long _dumpCount = 0;
};
#include "SystemLog.h"
#include <WinBase.h>
#include <strsafe.h>

SystemLog* SystemLog::_pInstance = nullptr;
std::mutex SystemLog::_lock;

SystemLog* SystemLog::GetInstance()
{
	//���� üŷ �� �������� ��Ƽ ������ ȯ�濡����
	//�̱��� ��ü�� ���ϼ��� ������.
	if (_pInstance == nullptr)
	{
		std::lock_guard<std::mutex> lock(_lock);
		if (_pInstance == nullptr)
		{
			_pInstance = new SystemLog();
		}
	}
	return _pInstance;
}

void SystemLog::WriteLog(const WCHAR* type, LogLevel logLevel, const WCHAR* stringFormat, ...)
{
	if (_logLevel > logLevel)
	{
		return;
	}

	tm timeInfo;
	time_t nowTime = time(NULL);
	localtime_s(&timeInfo, &nowTime);

	//���͸� ����
	CreateDirectory(_directoryName, NULL);

	//type���� �׸��� �� �ȿ����� �� ���� �αװ� ������� �Ѵ�.
	WCHAR fileName[256];

	//���� �̸� ���� ex) 201509_Battle.txt
	StringCchPrintf(fileName, 256, L".\\%s\\%d%02d_%s.txt",
		_directoryName, timeInfo.tm_year + 1900, timeInfo.tm_mon + 1, type);


	long long logCount = InterlockedIncrement64(&_logCount);

	WCHAR logLevelStr[10] = { 0, };
	switch (logLevel)
	{
	case SystemLog::LOG_LEVEL_DEBUG:
		StringCchPrintf(logLevelStr, 10, L"DEBUG");
		break;
	case SystemLog::LOG_LEVEL_ERROR:
		StringCchPrintf(logLevelStr, 10, L"ERROR");
		break;
	case SystemLog::LOG_LEVEL_SYSTEM:
		StringCchPrintf(logLevelStr, 10, L"SYSTEM");
		break;
	default:
		break;
	}

	//[Battle] [2015-09-11 19:00:00 / DEBUG / 000000001] �α׹��ڿ�.........	

	//�α� ������ ����
	WCHAR messageBuf[256];

	va_list va;
	va_start(va, stringFormat);
	HRESULT hResult = StringCchVPrintf(messageBuf, 256, stringFormat, va);
	if (hResult != S_OK)
	{
		WriteLog(L"ERROR", LOG_LEVEL_ERROR, L"�α� ���� ũ�� ����");
	}
	va_end(va);

	WCHAR totalBuf[512];
	//��Ż ���ۿ� �ְ� ���� ����
	hResult = StringCchPrintf(totalBuf, 512, L"[%s] [%d-%02d-%02d %02d:%02d:%02d / %s / %09lld] %s\n",
		type, timeInfo.tm_year + 1900, timeInfo.tm_mon + 1, timeInfo.tm_mday,
		timeInfo.tm_hour, timeInfo.tm_min, timeInfo.tm_sec, logLevelStr, logCount, messageBuf);

	if (hResult != S_OK)
	{
		WriteLog(L"ERROR", LOG_LEVEL_ERROR, L"�α� ���� ũ�� ����");
	}

	{
		std::lock_guard<std::mutex>lock(_lock);

		//�б�� ������ �� �ֵ��� ������ ����.
		_pFile = _wfsopen(fileName, L"a, ccs = UTF-16LE", _SH_DENYWR);
		if (_pFile == nullptr)
		{
			wprintf(L"file open fail, errorCode = %d\n", GetLastError());
			return;
		}

		fwrite(totalBuf, 1, wcslen(totalBuf) * sizeof(WCHAR), _pFile);

		fclose(_pFile);
		_pFile = nullptr;
	}

}

void SystemLog::WriteLogHex(const WCHAR* type, LogLevel logLevel, const WCHAR* log, BYTE* pByte, int byteLen)
{
	WCHAR messageBuf[10000] = { 0, };
	int offset = 0;

	// �α� �޽��� �߰�
	offset += swprintf(messageBuf + offset, 10000 - offset, L"%s: ", log);

	// �޸� �����͸� 16������ ��ȯ�Ͽ� �߰�
	for (int i = 0; i < byteLen; i++)
	{
		offset += swprintf(messageBuf + offset, 10000 - offset, L"%02X ", pByte[i]);
	}

	WriteLog(type, logLevel, messageBuf);
}

void SystemLog::WriteLogConsole(LogLevel logLevel, const WCHAR* stringFormat, ...)
{
	if (_logLevel > logLevel)
	{
		return;
	}

	WCHAR logBuffer[1024] = { 0, };

	va_list va;

	va_start(va, stringFormat);
	HRESULT hResult = StringCchVPrintf(logBuffer, 1024, stringFormat, va);
	if (hResult != S_OK)
	{
		WriteLogConsole(LOG_LEVEL_ERROR, L"�α� ���� ũ�� ����");
	}
	va_end(va);

	wprintf(L"%s\n", logBuffer);
}

SystemLog::SystemLog()
	:_asyncLogArray(nullptr)
{

}

void SystemLog::SetLogDirectory(const WCHAR* directoryName)
{
	_directoryName = directoryName;
}

void SystemLog::SetLogLevel(LogLevel logLevel)
{
	_logLevel = logLevel;
}

void SystemLog::InitAsyncLog()
{
	_asyncLogArray = new WCHAR * [ASYNC_LOG_ROW];
	for (int i = 0; i < ASYNC_LOG_ROW; i++)
	{
		//ASYNC_LOG_COL�� ���̸� �Ѿ�� �α״� ����� �Ѵ�. ������ ����ó�� ���� ���� ����.
		_asyncLogArray[i] = new WCHAR[ASYNC_LOG_COL];
	}

	_hAsyncEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	_hAsyncLogThread = StartAsyncLogThread();
	CloseHandle(_hAsyncLogThread);
}

void SystemLog::CloseAsyncLog()
{
	SetEvent(_hAsyncEvent);
	WaitForSingleObject(_hAsyncLogThread, INFINITE);
	CloseHandle(_hAsyncEvent);

	for (int i = 0; i < ASYNC_LOG_ROW; i++)
	{
		delete[] _asyncLogArray[i];
	}
	delete[] _asyncLogArray;
}

HANDLE SystemLog::StartAsyncLogThread()
{
	auto threadFunc = [](void* pThis) -> unsigned int
		{
			SystemLog* pSystemLog = static_cast<SystemLog*>(pThis);
			pSystemLog->AsyncLogThreadProc();
			return 0;
		};

	return (HANDLE)_beginthreadex(NULL, 0, threadFunc, this, 0, nullptr);
}

void SystemLog::AsyncLogThreadProc()
{
	DWORD result;
	while (1)
	{
		result = WaitForSingleObject(_hAsyncEvent, 50);
		if (result == WAIT_OBJECT_0)
			break;

		if (_isAsyncArrayEmpty == TRUE)
			continue;

		//���� �ε����� �� �ε���
		int startIdx = _startAsyncLogIndex;
		int endIdx = _endAsyncLogIndex;
		InterlockedExchange(&_isAsyncArrayEmpty, TRUE);

		if (startIdx <= endIdx)
		{
			for (int i = startIdx; i <= endIdx; i++)
			{
				WriteLog(L"Async", LOG_LEVEL_SYSTEM, _asyncLogArray[i]);
			}
		}
		else
		{
			for (int i = startIdx; i < ASYNC_LOG_ROW; i++)
			{
				WriteLog(L"Async", LOG_LEVEL_SYSTEM, _asyncLogArray[i]);
			}

			for (int j = 0; j <= endIdx; j++)
			{
				WriteLog(L"Async", LOG_LEVEL_SYSTEM, _asyncLogArray[j]);
			}
		}

		WriteLog(L"Async", LOG_LEVEL_SYSTEM, L"========================");

	}

	return;
}

void SystemLog::WriteAsyncLog(const WCHAR* stringFormat, ...)
{
	unsigned long long idx;
	int len = 0;

	idx = InterlockedIncrement(&_curAsyncLogIndex);

	idx %= ASYNC_LOG_ROW;

	WCHAR* pLog = nullptr;
	pLog = _asyncLogArray[idx];

	//������ ID �޸𸮿� ����
	HRESULT hResult;
	hResult = StringCchPrintf(pLog, ASYNC_LOG_COL, L"Thread ID[%d] : ", GetCurrentThreadId());
	len = wcslen(pLog);

	//�α� ���� �޸𸮿� ����
	va_list va;
	va_start(va, stringFormat);
	hResult = StringCchVPrintf(pLog + len - 1, ASYNC_LOG_COL - (len - 1), stringFormat, va);
	va_end(va);
	len = wcslen(pLog);
	pLog[len] = L'\n';
	pLog[len + 1] = L'\0';

	//�α׸� �� �����ϱ� ���⼭ ���� �ε��� ����
	_endAsyncLogIndex = idx;

	//���� ����־��ٸ� ���ʷ� �α׸� �� �ε����̹Ƿ� ��ŸƮ �ε����� ����
	if (InterlockedExchange(&_isAsyncArrayEmpty, FALSE) == TRUE)
	{
		_startAsyncLogIndex = idx;
	}

}



SystemLog::~SystemLog()
{

}


#include "SystemLog.h"
#include <WinBase.h>

using namespace psbe_en;

//static 멤버 변수 정의
SystemLog* SystemLog::_pInstance = nullptr;
std::mutex SystemLog::_lock;

SystemLog* SystemLog::GetInstance()
{
	//더블 체킹 락 패턴으로 멀티 스레드 환경에서도
	//싱글톤 객체의 유일성을 보장함.
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

	//디렉터리 생성
	CreateDirectory(_directoryName, NULL);

	//type별로 그리고 그 안에서는 월 별로 로그가 나뉘어야 한다.
	WCHAR fileName[256];

	//파일 이름 결정 ex) 201509_Battle.txt
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
	
	//[Battle] [2015-09-11 19:00:00 / DEBUG / 000000001] 로그문자열.........	

	//로그 데이터 세팅
	WCHAR messageBuf[256];

	va_list va;
	va_start(va, stringFormat);
	HRESULT hResult = StringCchVPrintf(messageBuf, 256, stringFormat, va);
	if (hResult != S_OK)
	{
		WriteLog(L"ERROR", LOG_LEVEL_ERROR, L"로그 버퍼 크기 부족");
	}
	va_end(va);

	WCHAR totalBuf[512];
	//토탈 버퍼에 넣고 파일 쓰기
	hResult = StringCchPrintf(totalBuf, 512, L"[%s] [%d-%02d-%02d %02d:%02d:%02d / %s / %09lld] %s\n",
		type, timeInfo.tm_year + 1900, timeInfo.tm_mon + 1, timeInfo.tm_mday,
		timeInfo.tm_hour, timeInfo.tm_min, timeInfo.tm_sec, logLevelStr, logCount, messageBuf);

	if (hResult != S_OK)
	{
		WriteLog(L"ERROR", LOG_LEVEL_ERROR, L"로그 버퍼 크기 부족");
	}

	{
		std::lock_guard<std::mutex>lock(_lock);

		//읽기는 공유할 수 있도록 파일을 연다.
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
	
	// 로그 메시지 추가
	offset += swprintf(messageBuf + offset, 10000 - offset, L"%s: ", log);

	// 메모리 데이터를 16진수로 변환하여 추가
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
		WriteLogConsole(LOG_LEVEL_ERROR, L"로그 버퍼 크기 부족");
	}
	va_end(va);

	wprintf(L"%s\n", logBuffer);
}

SystemLog::SystemLog()
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

SystemLog::~SystemLog()
{

}


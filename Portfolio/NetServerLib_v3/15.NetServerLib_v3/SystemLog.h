#pragma once

/* ======================
* date : 2024- 08-13
* writer : 박성빈
* content : SystemLog
* - 프로그래머들이 개발 과정에서 편하게 파일로 로그를 남길 수 있도록
*  SystemLog 기능을 제공하는 싱글톤 클래스입니다.
* 
* main에서 한 번 SetLogDirectory와 SetLogLevel을 호출해줘야 한다.
*
========================*/
#include <mutex>
#include <Windows.h>

constexpr int ASYNC_LOG_COL = 256;
constexpr int ASYNC_LOG_ROW = 10000;

class SystemLog
{
public:
	enum LogLevel
	{
		LOG_LEVEL_DEBUG = 0,
		LOG_LEVEL_ERROR,
		LOG_LEVEL_SYSTEM
	};

	static SystemLog* GetInstance();

	void WriteLog(const WCHAR* type, LogLevel logLevel, const WCHAR* stringFormat, ...);
	void WriteLogHex(const WCHAR* type, LogLevel logLevel, const WCHAR* log, BYTE* pByte, int byteLen);
	void WriteLogConsole(LogLevel logLevel, const WCHAR* stringFormat, ...);

	void SetLogDirectory(const WCHAR* directoryName);
	void SetLogLevel(LogLevel logLevel);

	//비동기 로깅 관련 함수
	void InitAsyncLog();
	void CloseAsyncLog();
	HANDLE StartAsyncLogThread();
	void AsyncLogThreadProc();
	void WriteAsyncLog(const WCHAR* stringFormat, ...);


	~SystemLog();

	//명시적으로 복사 생성자 및 대입 연산자를 제거
	SystemLog(const SystemLog&) = delete;
	SystemLog& operator=(const SystemLog&) = delete;

private:
	SystemLog();

private:
	static SystemLog* _pInstance;
	static std::mutex _lock;	//초기화의 편의를 위해 mutex 사용

	WCHAR** _asyncLogArray;
	HANDLE _hAsyncLogThread = INVALID_HANDLE_VALUE;	//비동기로 로그를 찍는 스레드 핸들
	HANDLE _hAsyncEvent = INVALID_HANDLE_VALUE;		//우아한 종료를 위한 이벤트
	HANDLE _hHeap = INVALID_HANDLE_VALUE;


	long long _logCount = 0;
	LogLevel _logLevel = LogLevel::LOG_LEVEL_DEBUG;
	const WCHAR* _directoryName = nullptr;
	FILE* _pFile = nullptr;

	unsigned long long _curAsyncLogIndex = -1;
	unsigned long long _startAsyncLogIndex = 0;
	unsigned long long _endAsyncLogIndex = 0;
	long	_isAsyncArrayEmpty = TRUE;
};




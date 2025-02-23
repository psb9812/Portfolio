#pragma once

/* ======================
* date : 2024- 08-13
* writer : �ڼ���
* content : SystemLog
* - ���α׷��ӵ��� ���� �������� ���ϰ� ���Ϸ� �α׸� ���� �� �ֵ���
*  SystemLog ����� �����ϴ� �̱��� Ŭ�����Դϴ�.
* 
* main���� �� �� SetLogDirectory�� SetLogLevel�� ȣ������� �Ѵ�.
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

	//�񵿱� �α� ���� �Լ�
	void InitAsyncLog();
	void CloseAsyncLog();
	HANDLE StartAsyncLogThread();
	void AsyncLogThreadProc();
	void WriteAsyncLog(const WCHAR* stringFormat, ...);


	~SystemLog();

	//��������� ���� ������ �� ���� �����ڸ� ����
	SystemLog(const SystemLog&) = delete;
	SystemLog& operator=(const SystemLog&) = delete;

private:
	SystemLog();

private:
	static SystemLog* _pInstance;
	static std::mutex _lock;	//�ʱ�ȭ�� ���Ǹ� ���� mutex ���

	WCHAR** _asyncLogArray;
	HANDLE _hAsyncLogThread = INVALID_HANDLE_VALUE;	//�񵿱�� �α׸� ��� ������ �ڵ�
	HANDLE _hAsyncEvent = INVALID_HANDLE_VALUE;		//����� ���Ḧ ���� �̺�Ʈ
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




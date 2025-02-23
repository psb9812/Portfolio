#pragma once

/* ======================
* date : 2024- 08-13
* writer : �ڼ���
* content : SystemLog
* - ���α׷��ӵ��� ���� �������� ���ϰ� ���Ϸ� �α׸� ���� �� �ֵ���
*  SystemLog ����� �����ϴ� �̱��� Ŭ�����Դϴ�.
* 
========================*/ 
#include <mutex>
#include <Windows.h>
#include <strsafe.h>

namespace psbe_en
{
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

		~SystemLog();

		//��������� ���� ������ �� ���� �����ڸ� ����
		SystemLog(const SystemLog&) = delete;
		SystemLog& operator=(const SystemLog&) = delete;

	private:
		SystemLog();

	private:
		static SystemLog* _pInstance;
		static std::mutex _lock;	//�ʱ�ȭ�� ���Ǹ� ���� mutex ���

		long long _logCount = 0;
		LogLevel _logLevel = LogLevel::LOG_LEVEL_DEBUG;
		const WCHAR* _directoryName = nullptr;
		FILE* _pFile = nullptr;
	};

}




#pragma once

/* ======================
* date : 2024- 08-13
* writer : 박성빈
* content : SystemLog
* - 프로그래머들이 개발 과정에서 편하게 파일로 로그를 남길 수 있도록
*  SystemLog 기능을 제공하는 싱글톤 클래스입니다.
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

		//명시적으로 복사 생성자 및 대입 연산자를 제거
		SystemLog(const SystemLog&) = delete;
		SystemLog& operator=(const SystemLog&) = delete;

	private:
		SystemLog();

	private:
		static SystemLog* _pInstance;
		static std::mutex _lock;	//초기화의 편의를 위해 mutex 사용

		long long _logCount = 0;
		LogLevel _logLevel = LogLevel::LOG_LEVEL_DEBUG;
		const WCHAR* _directoryName = nullptr;
		FILE* _pFile = nullptr;
	};

}




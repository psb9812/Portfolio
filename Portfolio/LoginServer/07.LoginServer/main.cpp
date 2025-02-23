#include "pch.h"
#include "LoginServer.h"
#include "LanClient/MonitoringClient.h"
#include "SystemLog.h"
#include "Message.h"
#include "Dump.h"


void ServerControl();


bool g_mainLoopRun = true;
Dump dump;
LoginServer loginServer;
MonitoringClient monitoringClient;


int main()
{
	setlocale(LC_ALL, "");
	timeBeginPeriod(1);

	if (!loginServer.Start(L"NetServer.cnf"))
	{
		SystemLog::GetInstance()->WriteLogConsole(SystemLog::LOG_LEVEL_ERROR, L"서버 스타트 실패\n");
		timeEndPeriod(1);
		return 0;
	}

	if (!monitoringClient.Start(&loginServer))
	{
		SystemLog::GetInstance()->WriteLogConsole(SystemLog::LOG_LEVEL_ERROR, L"모니터링 클라이언트 스타트 실패\n");
		timeEndPeriod(1);
		return 0;
	}
	while (g_mainLoopRun)
	{
		Sleep(1000);
		//서버 컨트롤
		ServerControl();

	}
	timeEndPeriod(1);
	return 0;
}

void ServerControl()
{
	static bool controlMode = false;

	//--------------------------------------------------------
	//L : 컨트롤 Lock  /  U : 컨트롤 Unlock  /  Q : 서버 종료
	//--------------------------------------------------------
	if (_kbhit())
	{
		WCHAR controlKey = _getwch();

		//키보드 제어 허용
		if (L'u' == controlKey || L'U' == controlKey)
		{
			controlMode = true;
			//관련 키 도움말 출력
			wprintf(L"--------------------------------------------------\n");
			wprintf(L"# Control Mode : Press Q - Quit\n");
			wprintf(L"# Control Mode : Press L - Key Lock\n");
			wprintf(L"# Control Mode : Press 0 - Set log level debug\n");
			wprintf(L"# Control Mode : Press 1 - Set log level error\n");
			wprintf(L"# Control Mode : Press 2 - Set log level system\n");
			wprintf(L"--------------------------------------------------\n");
		}
		if ((L'l' == controlKey || L'L' == controlKey) && controlMode)
		{
			wprintf(L"Control Lock..! Press U - Control Unlock\n");
			controlMode = false;
		}
		if ((L'q' == controlKey || L'Q' == controlKey) && controlMode)
		{
			loginServer.Stop();
			g_mainLoopRun = false;
		}
		if (L'0' == controlKey && controlMode)
		{
			SystemLog::GetInstance()->SetLogLevel(SystemLog::LOG_LEVEL_DEBUG);
			wprintf(L"Current Log Level : LOG_LEVEL_DEBUG \n");
		}
		if (L'1' == controlKey && controlMode)
		{
			SystemLog::GetInstance()->SetLogLevel(SystemLog::LOG_LEVEL_ERROR);
			wprintf(L"Current Log Level : LOG_LEVEL_ERROR \n");
		}
		if (L'2' == controlKey && controlMode)
		{
			SystemLog::GetInstance()->SetLogLevel(SystemLog::LOG_LEVEL_SYSTEM);
			wprintf(L"Current Log Level : LOG_LEVEL_SYSTEM \n");
		}
		if ((L'w' == controlKey || L'W' == controlKey) && controlMode)
		{
			//ProfileManager::ProfileDataOutText(L"LoginServer_Single_Proflie.txt");
		}
		if ((L'r' == controlKey || L'R' == controlKey) && controlMode)
		{
			//ProfileManager::ProfileReset();
		}
	}
}

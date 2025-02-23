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
		SystemLog::GetInstance()->WriteLogConsole(SystemLog::LOG_LEVEL_ERROR, L"���� ��ŸƮ ����\n");
		timeEndPeriod(1);
		return 0;
	}

	if (!monitoringClient.Start(&loginServer))
	{
		SystemLog::GetInstance()->WriteLogConsole(SystemLog::LOG_LEVEL_ERROR, L"����͸� Ŭ���̾�Ʈ ��ŸƮ ����\n");
		timeEndPeriod(1);
		return 0;
	}
	while (g_mainLoopRun)
	{
		Sleep(1000);
		//���� ��Ʈ��
		ServerControl();

	}
	timeEndPeriod(1);
	return 0;
}

void ServerControl()
{
	static bool controlMode = false;

	//--------------------------------------------------------
	//L : ��Ʈ�� Lock  /  U : ��Ʈ�� Unlock  /  Q : ���� ����
	//--------------------------------------------------------
	if (_kbhit())
	{
		WCHAR controlKey = _getwch();

		//Ű���� ���� ���
		if (L'u' == controlKey || L'U' == controlKey)
		{
			controlMode = true;
			//���� Ű ���� ���
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

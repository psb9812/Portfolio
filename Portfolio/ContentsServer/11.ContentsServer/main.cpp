#include "pch.h"
#include "EchoServer.h"
#include <locale>
#include "MonitoringClient.h"

Dump dump;
EchoServer echoServer;
MonitoringClient monitoringClient;

int main()
{
	setlocale(LC_ALL, "");
	timeBeginPeriod(1);

	if (!echoServer.Start(L"NetServer.cnf"))
	{
		SystemLog::GetInstance()->WriteLogConsole(SystemLog::LOG_LEVEL_ERROR, L"에코 서버 스타트 실패\n");
		timeEndPeriod(1);
		return 0;
	}
	
	if (!monitoringClient.Start(&echoServer))
	{
		SystemLog::GetInstance()->WriteLogConsole(SystemLog::LOG_LEVEL_ERROR, L"모니터링 클라이언트 스타트 실패\n");
		timeEndPeriod(1);
		return 0;
	}
	while (true)
	{
		if (!echoServer.ServerControl())
		{
			break;
		}

		Sleep(100);
	}

	timeEndPeriod(1);
	return 0;
}
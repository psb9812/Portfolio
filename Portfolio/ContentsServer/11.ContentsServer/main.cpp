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
		SystemLog::GetInstance()->WriteLogConsole(SystemLog::LOG_LEVEL_ERROR, L"���� ���� ��ŸƮ ����\n");
		timeEndPeriod(1);
		return 0;
	}
	
	if (!monitoringClient.Start(&echoServer))
	{
		SystemLog::GetInstance()->WriteLogConsole(SystemLog::LOG_LEVEL_ERROR, L"����͸� Ŭ���̾�Ʈ ��ŸƮ ����\n");
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
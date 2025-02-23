#include "MonitoringNetServer.h"
#include "MonitoringLanServer.h"
#include "Tools/Dump.h"
#pragma comment(lib, "Winmm.lib")

Dump dump;
MonitoringNetServer monitorNetServer;
MonitoringLanServer monitorLanServer(&monitorNetServer);

void Monitor();

bool g_mainLoopRun = true;
int main()
{
	setlocale(LC_ALL, "");
	timeBeginPeriod(1);

	if (!monitorNetServer.Start(L"NetServer.cnf"))
	{
		wprintf(L"모니터 넷 서버 스타트 실패\n");
		return 0;
	}
	if (!monitorLanServer.Start(L"LanServer.cnf"))
	{
		wprintf(L"모니터 랜 서버 스타트 실패\n");
		return 0;
	}

	while (true)
	{
		Monitor();
		Sleep(1000);
	}

	return 0;
}

void Monitor()
{
	wprintf(L"====================== 모니터링 서버 ============================\n"
		L"LanSession Num : %d\n"
		L"NetSession Num : %d\n"
		L"RecvTPS To Servers : %lld\n"
		L"SendTPS To MonitoringTool : %lld\n"
		, monitorLanServer.GetSessionCount(), monitorNetServer.GetSessionCount(),
		monitorLanServer.GetRecvMessageTPS(), monitorNetServer.GetSendMessageTPS()
	);

}
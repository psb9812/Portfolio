#include <iostream>
#include <Pdh.h>
#include "HWPerformanceCounter.h"
#include "ProcessPerformanceCounter.h"
#include <conio.h>

using namespace std;
#pragma comment(lib, "Pdh.lib")

//int main()
//{
//    setlocale(LC_ALL, "");
//
//    //PDH쿼리 핸들 생성
//    PDH_HQUERY cpuQuery;
//    PdhOpenQuery(NULL, NULL, &cpuQuery);
//
//    //PDH 리소스 카운터 생성 (여러 개 수집 시 이를 여러 개 생성)
//    PDH_HCOUNTER cpuTotal;
//    PdhAddCounter(cpuQuery, L"\\Processor(_Total)\\% Processor Time", NULL, &cpuTotal);
//
//    //첫 갱신
//    PdhCollectQueryData(cpuQuery);
//
//    while (true)
//    {
//        Sleep(1000);
//
//        //1초마다 갱신
//        PdhCollectQueryData(cpuQuery);
//
//        //갱신 데이터 얻음
//        PDH_FMT_COUNTERVALUE counterVal;
//        PdhGetFormattedCounterValue(cpuTotal, PDH_FMT_DOUBLE, NULL, &counterVal);
//        wprintf(L"CPU Usage : %f%%\n", counterVal.doubleValue);
//    }
//
//    return 0;
//}

int main()
{
	setlocale(LC_ALL, "");

	HWPerformanceCounter HWCounter;
	ProcessPerformanceCounter pCounter;

	while (true)
	{
		Sleep(1000);
		cout << "CPUUsage : " << HWCounter.GetCPUUsage() << "\n"
			<< "AvailableMBytes : " << HWCounter.GetAvilableMBytes() << '\n'
			<< "NPPoolMemory : " << HWCounter.GetNPPoolMemoryBytes() << endl;
		cout << "Process CPU Usage : " << pCounter.GetProcessCPUUsage() << "\n"
			<< "PrivateMemory : " << pCounter.GetPrivateMemoryBytes() << "\n";
		cout << "============================" << endl;

		if (_kbhit())
		{
			char input = _getch();

			if (input == 'o')
			{
				cout << "메모리 할당" << endl;
				//의도적인 메모리 누수
				int* k = new int[1000000];
			}
		}
	}
}

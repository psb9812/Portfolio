#pragma once
#include <Pdh.h>

#pragma comment(lib, "Pdh.lib")

class ProcessPerformanceCounter
{
public:
	ProcessPerformanceCounter();
	~ProcessPerformanceCounter();

	double GetProcessCPUUsage();
	INT64 GetPrivateMemoryBytes();

private:
	PDH_HQUERY _processCPUUsageQuery;
	PDH_HQUERY _privateMemoryQuery;

	// * 프로세스 CPU 사용률 (초당 사용률)
	// 한 프로세스의 CPU 사용률이다. 
	PDH_HCOUNTER _processCPUUsageCounter;

	// * private Memory (byte 단위)
	// 커널이 사용하는 항상 물리 메모리에 상주하는 메모리이다.
	// TCP나 IOCP가 사용하는 Non_Paged Pool이 이 수치에 해당함.
	PDH_HCOUNTER _privateMemoryBytesCounter;
};


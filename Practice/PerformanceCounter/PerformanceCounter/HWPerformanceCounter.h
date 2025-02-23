#pragma once
#include <Pdh.h>

#pragma comment(lib, "Pdh.lib")

class HWPerformanceCounter
{
public:
	HWPerformanceCounter();
	~HWPerformanceCounter();

	double GetCPUUsage();
	INT64 GetNPPoolMemoryBytes();
	double GetAvilableMBytes();

private:
	PDH_HQUERY _cpuUsageQuery;
	PDH_HQUERY _poolNPBytesQuery;
	PDH_HQUERY _availableMBytesQuery;
	
	// * total CPU 사용률 (초당 사용률)
	// 전체 프로세서의 사용률이다. 
	// 이 값은 프로세서가 유저 모드와 커널 모드에서 작업을 처리하는 데 소요된 시간을 기준으로,
	// 전체 시간 대비 CPU가 얼마나 사용되고 있는지를 나타낸다.
	PDH_HCOUNTER _cpuUsageCounter;

	// * 논페이지드 메모리 (byte 단위)
	// 커널이 사용하는 항상 물리 메모리에 상주하는 메모리이다.
	// TCP나 IOCP가 사용하는 Non_Paged Pool이 이 수치에 해당함.
	PDH_HCOUNTER _poolNPBytesCounter;

	// * 시스템에서 자유롭게 사용 가능한 물리 메모리 (MByte 단위)
	PDH_HCOUNTER _availableMBytesCounter;		
};
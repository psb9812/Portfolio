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

	// * ���μ��� CPU ���� (�ʴ� ����)
	// �� ���μ����� CPU �����̴�. 
	PDH_HCOUNTER _processCPUUsageCounter;

	// * private Memory (byte ����)
	// Ŀ���� ����ϴ� �׻� ���� �޸𸮿� �����ϴ� �޸��̴�.
	// TCP�� IOCP�� ����ϴ� Non_Paged Pool�� �� ��ġ�� �ش���.
	PDH_HCOUNTER _privateMemoryBytesCounter;
};


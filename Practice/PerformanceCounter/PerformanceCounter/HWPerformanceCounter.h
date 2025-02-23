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
	
	// * total CPU ���� (�ʴ� ����)
	// ��ü ���μ����� �����̴�. 
	// �� ���� ���μ����� ���� ���� Ŀ�� ��忡�� �۾��� ó���ϴ� �� �ҿ�� �ð��� ��������,
	// ��ü �ð� ��� CPU�� �󸶳� ���ǰ� �ִ����� ��Ÿ����.
	PDH_HCOUNTER _cpuUsageCounter;

	// * ���������� �޸� (byte ����)
	// Ŀ���� ����ϴ� �׻� ���� �޸𸮿� �����ϴ� �޸��̴�.
	// TCP�� IOCP�� ����ϴ� Non_Paged Pool�� �� ��ġ�� �ش���.
	PDH_HCOUNTER _poolNPBytesCounter;

	// * �ý��ۿ��� �����Ӱ� ��� ������ ���� �޸� (MByte ����)
	PDH_HCOUNTER _availableMBytesCounter;		
};
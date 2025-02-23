#include "HWPerformanceCounter.h"

HWPerformanceCounter::HWPerformanceCounter()
{
	PdhOpenQuery(NULL, NULL, &_cpuUsageQuery);
	PdhOpenQuery(NULL, NULL, &_availableMBytesQuery);
	PdhOpenQuery(NULL, NULL, &_poolNPBytesQuery);

	PdhAddCounter(_cpuUsageQuery, L"\\Processor(_Total)\\% Processor Time", NULL, &_cpuUsageCounter);
	PdhAddCounter(_availableMBytesQuery, L"\\Memory\\Available MBytes", NULL, &_availableMBytesCounter);
	PdhAddCounter(_poolNPBytesQuery, L"\\Memory\\Pool Nonpaged Bytes", NULL, &_poolNPBytesCounter);
}

HWPerformanceCounter::~HWPerformanceCounter()
{
	
}

double HWPerformanceCounter::GetCPUUsage()
{
	PdhCollectQueryData(_cpuUsageQuery);

	//갱신 데이터 얻음
	PDH_FMT_COUNTERVALUE counterVal;
	PdhGetFormattedCounterValue(_cpuUsageCounter, PDH_FMT_DOUBLE, NULL, &counterVal);
	return counterVal.doubleValue;
}

INT64 HWPerformanceCounter::GetNPPoolMemoryBytes()
{
	PdhCollectQueryData(_poolNPBytesQuery);

	//갱신 데이터 얻음
	PDH_FMT_COUNTERVALUE counterVal;
	PdhGetFormattedCounterValue(_poolNPBytesCounter, PDH_FMT_LARGE, NULL, &counterVal);
	return counterVal.largeValue;
}

double HWPerformanceCounter::GetAvilableMBytes()
{
	PdhCollectQueryData(_availableMBytesQuery);

	//갱신 데이터 얻음
	PDH_FMT_COUNTERVALUE counterVal;
	PdhGetFormattedCounterValue(_availableMBytesCounter, PDH_FMT_DOUBLE, NULL, &counterVal);
	return counterVal.doubleValue;
}

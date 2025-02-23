#include "ProcessPerformanceCounter.h"
#include <string>

ProcessPerformanceCounter::ProcessPerformanceCounter()
{
	//���� ���� ���α׷� �̸� ���ϱ�
	WCHAR programPath[MAX_PATH];
	GetModuleFileName(NULL, programPath, MAX_PATH);
	
	std::wstring path(programPath);
	size_t pos = path.find_last_of(L"\\");
	std::wstring processName = path.substr(pos + 1);
	//.exe �����ϱ�
	size_t dotPos = processName.find_last_of(L".");
	processName.erase(dotPos, 4);	

	//���� �����
	WCHAR processCPUQueryWStr[MAX_PATH];
	wsprintf(processCPUQueryWStr, L"\\Process(%s)\\%% Processor Time", processName.c_str());
	WCHAR processPrivateMemoryQueryWStr[MAX_PATH];
	wsprintf(processPrivateMemoryQueryWStr, L"\\Process(%s)\\Private Bytes", processName.c_str());

	//���� ����
	PdhOpenQuery(NULL, NULL, &_processCPUUsageQuery);
	PdhOpenQuery(NULL, NULL, &_privateMemoryQuery);
	//ī���� ���
	if (ERROR_SUCCESS != PdhAddCounter(_processCPUUsageQuery, processCPUQueryWStr, NULL, &_processCPUUsageCounter))
		__debugbreak();
	if(ERROR_SUCCESS != PdhAddCounter(_privateMemoryQuery, processPrivateMemoryQueryWStr, NULL, &_privateMemoryBytesCounter))
		__debugbreak();
}

ProcessPerformanceCounter::~ProcessPerformanceCounter()
{
	PdhCloseQuery(_processCPUUsageQuery);
	PdhCloseQuery(_privateMemoryQuery);
}

double ProcessPerformanceCounter::GetProcessCPUUsage()
{
	PdhCollectQueryData(_processCPUUsageQuery);

	//���� ������ ����
	PDH_FMT_COUNTERVALUE counterVal;
	PdhGetFormattedCounterValue(_processCPUUsageCounter, PDH_FMT_DOUBLE, NULL, &counterVal);
	return counterVal.doubleValue;
}

INT64 ProcessPerformanceCounter::GetPrivateMemoryBytes()
{
	PdhCollectQueryData(_privateMemoryQuery);

	//���� ������ ����
	PDH_FMT_COUNTERVALUE counterVal;
	PdhGetFormattedCounterValue(_privateMemoryBytesCounter, PDH_FMT_LARGE, NULL, &counterVal);
	return counterVal.largeValue;
}
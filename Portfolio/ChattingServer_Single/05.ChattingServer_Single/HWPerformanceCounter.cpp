#include "HWPerformanceCounter.h"
#include "wchar.h"

HWPerformanceCounter::HWPerformanceCounter()
{
	InitEthernetInfo();

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

void HWPerformanceCounter::InitEthernetInfo()
{
	WCHAR* counters = nullptr;
	DWORD counterSize = 0;
	WCHAR* interfaces = nullptr;
	DWORD interfaceSize = 0;
	WCHAR query[1024] = { 0, };

	// 첫 호출은 필요한 버퍼의 크기를 구하는 용도
	PdhEnumObjectItems(NULL, NULL, L"Network Interface",
		NULL, &counterSize, NULL, &interfaceSize, PERF_DETAIL_WIZARD, 0);
	counters = new WCHAR[counterSize];
	interfaces = new WCHAR[interfaceSize];

	// 두 번째 호출에서 
	// counters에는 사용할 수 있는 카운터 이름들의 목록이 문자열로 쭉 들어가고
	// interfaces에는 네트워크 인터페이스 이름들의 목록이 문자열로 쭉 들어간다.
	// 각 항목은 널 문자로 구분되어 있음.
	if(PdhEnumObjectItems(NULL, NULL, L"Network Interface", 
		counters, &counterSize, interfaces, &interfaceSize, PERF_DETAIL_WIZARD, 0) != ERROR_SUCCESS)
	{
		delete[] counters;
		delete[] interfaces;
		__debugbreak();
		return;
	}
	
	//하나의 문자열 버퍼에 받은 카운터와 인터페이스 정보를 잘라서 멤버 변수에 저장 및 쿼리 등록
	int index = 0;
	WCHAR* pCur = interfaces;
	for (int idx = 0; 
		*pCur != L'\0' && idx < ETHERNET_MAX_NUM; 
		pCur += wcslen(pCur) + 1, idx++)
	{
		//이름 저장
		_ethernetInfos[idx]._isUsed = true;
		_ethernetInfos[idx]._deviceName[0] = L'\0';
		wcscpy_s(_ethernetInfos[idx]._deviceName, pCur);

		//recv 쿼리 등록
		query[0] = L'\0';
		PdhOpenQuery(NULL, NULL, &_ethernetInfos[idx]._recvQuery);
		swprintf_s(query, L"\\Network Interface(%s)\\Bytes Received/sec", pCur);
		PdhAddCounter(_ethernetInfos[idx]._recvQuery, query,
			NULL, &_ethernetInfos[idx]._recvCounter);

		//send 쿼리 등록
		query[0] = L'\0';
		PdhOpenQuery(NULL, NULL, &_ethernetInfos[idx]._sendQuery);
		swprintf_s(query, L"\\Network Interface(%s)\\Bytes Sent/sec", pCur);
		PdhAddCounter(_ethernetInfos[idx]._sendQuery, query,
			NULL, &_ethernetInfos[idx]._sendCounter);
	}
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

int HWPerformanceCounter::GetEthernetRecvBytes()
{
	int totalRecvBytes = 0;
	for (int i = 0; i < ETHERNET_MAX_NUM; i++)
	{
		if (!_ethernetInfos[i]._isUsed) 
			continue;

		PDH_FMT_COUNTERVALUE counterValue;
		PdhCollectQueryData(_ethernetInfos[i]._recvQuery);
		int ret = PdhGetFormattedCounterValue(_ethernetInfos[i]._recvCounter, PDH_FMT_LONG, NULL, &counterValue);
		if (ret == ERROR_SUCCESS)
			totalRecvBytes += counterValue.longValue;
	}
	return totalRecvBytes;
}

int HWPerformanceCounter::GetEthernetSendBytes()
{
	int totalSendBytes = 0;
	for (int i = 0; i < ETHERNET_MAX_NUM; i++)
	{
		if (!_ethernetInfos[i]._isUsed)
			continue;

		PDH_FMT_COUNTERVALUE counterValue;
		PdhCollectQueryData(_ethernetInfos[i]._sendQuery);
		int ret = PdhGetFormattedCounterValue(_ethernetInfos[i]._sendCounter, PDH_FMT_LONG, NULL, &counterValue);
		if (ret == ERROR_SUCCESS)
			totalSendBytes += counterValue.longValue;
	}
	return totalSendBytes;
}

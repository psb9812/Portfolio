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

	// ù ȣ���� �ʿ��� ������ ũ�⸦ ���ϴ� �뵵
	PdhEnumObjectItems(NULL, NULL, L"Network Interface",
		NULL, &counterSize, NULL, &interfaceSize, PERF_DETAIL_WIZARD, 0);
	counters = new WCHAR[counterSize];
	interfaces = new WCHAR[interfaceSize];

	// �� ��° ȣ�⿡�� 
	// counters���� ����� �� �ִ� ī���� �̸����� ����� ���ڿ��� �� ����
	// interfaces���� ��Ʈ��ũ �������̽� �̸����� ����� ���ڿ��� �� ����.
	// �� �׸��� �� ���ڷ� ���еǾ� ����.
	if(PdhEnumObjectItems(NULL, NULL, L"Network Interface", 
		counters, &counterSize, interfaces, &interfaceSize, PERF_DETAIL_WIZARD, 0) != ERROR_SUCCESS)
	{
		delete[] counters;
		delete[] interfaces;
		__debugbreak();
		return;
	}
	
	//�ϳ��� ���ڿ� ���ۿ� ���� ī���Ϳ� �������̽� ������ �߶� ��� ������ ���� �� ���� ���
	int index = 0;
	WCHAR* pCur = interfaces;
	for (int idx = 0; 
		*pCur != L'\0' && idx < ETHERNET_MAX_NUM; 
		pCur += wcslen(pCur) + 1, idx++)
	{
		//�̸� ����
		_ethernetInfos[idx]._isUsed = true;
		_ethernetInfos[idx]._deviceName[0] = L'\0';
		wcscpy_s(_ethernetInfos[idx]._deviceName, pCur);

		//recv ���� ���
		query[0] = L'\0';
		PdhOpenQuery(NULL, NULL, &_ethernetInfos[idx]._recvQuery);
		swprintf_s(query, L"\\Network Interface(%s)\\Bytes Received/sec", pCur);
		PdhAddCounter(_ethernetInfos[idx]._recvQuery, query,
			NULL, &_ethernetInfos[idx]._recvCounter);

		//send ���� ���
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

	//���� ������ ����
	PDH_FMT_COUNTERVALUE counterVal;
	PdhGetFormattedCounterValue(_cpuUsageCounter, PDH_FMT_DOUBLE, NULL, &counterVal);
	return counterVal.doubleValue;
}


INT64 HWPerformanceCounter::GetNPPoolMemoryBytes()
{
	PdhCollectQueryData(_poolNPBytesQuery);

	//���� ������ ����
	PDH_FMT_COUNTERVALUE counterVal;
	PdhGetFormattedCounterValue(_poolNPBytesCounter, PDH_FMT_LARGE, NULL, &counterVal);
	return counterVal.largeValue;
}

double HWPerformanceCounter::GetAvilableMBytes()
{
	PdhCollectQueryData(_availableMBytesQuery);

	//���� ������ ����
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

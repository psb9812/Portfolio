#pragma once
#include <Pdh.h>
#include <iphlpapi.h>

#pragma comment(lib, "Pdh.lib")
#pragma comment(lib, "iphlpapi.lib")

class HWPerformanceCounter
{
	static constexpr int ETHERNET_MAX_NUM = 16;
public:
	HWPerformanceCounter();
	~HWPerformanceCounter();

	void InitEthernetInfo();

	double GetCPUUsage();
	INT64 GetNPPoolMemoryBytes();
	double GetAvilableMBytes();
	//�ý��ۿ� �����ϴ� ��� �̴����� ���� ����Ʈ�� ��ȯ�Ѵ�.
	int GetEthernetRecvBytes();
	//�ý��ۿ� �����ϴ� ��� �̴����� �۽� ����Ʈ�� ��ȯ�Ѵ�.
	int GetEthernetSendBytes();

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

	//�̴��� ����
	struct EthernetInfo
	{
		bool _isUsed = false;
		WCHAR _deviceName[128];
		PDH_HQUERY _recvQuery;
		PDH_HCOUNTER _recvCounter;
		PDH_HQUERY _sendQuery;
		PDH_HCOUNTER _sendCounter;
	};

	EthernetInfo _ethernetInfos[ETHERNET_MAX_NUM];
};
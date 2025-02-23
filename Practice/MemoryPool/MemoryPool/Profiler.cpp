#include <iostream>
#include "Profiler.h"

std::list<Profile_Element*> ProfileManager::_totalProfileSampleContainer;
ProfileManager Profile::profileManager;
LONG ProfileManager::_resetFlag = FALSE;
LONG ProfileManager::_resetCount = 0;

int g_profilePtrTLSIndex = 0;

Profile::Profile(const TCHAR* tag)
{
	_tag = tag;
	ProfileBegin(tag);
}

Profile::~Profile()
{
	ProfileEnd(_tag);
}

void Profile::ProfileBegin(const TCHAR* szName)
{
	LONG resetCount = profileManager.GetResetCount();
	//���� ������ �˻�
	if (profileManager.GetResetFlag() == TRUE)
	{
		return;
	}

	// ȣ�� �����忡�� ���� ���������� ���� TLS ������ ���� ���� �ʱ�ȭ�� ���� �ʾҴٸ�
	// ProfileManager���� �Ҵ�޾� �ʱ�ȭ �Ѵ�.
	if (TlsGetValue(g_profilePtrTLSIndex) == NULL)
	{
		BOOL ret = TlsSetValue(g_profilePtrTLSIndex, (LPVOID)profileManager.Alloc());
		if (ret == 0)
		{
			__debugbreak();
		}
	}

	LARGE_INTEGER _startTime;
	LARGE_INTEGER _freq;
	QueryPerformanceFrequency(&_freq);
	QueryPerformanceCounter(&_startTime);

	Profile_Element* pProfileElement = (Profile_Element*)TlsGetValue(g_profilePtrTLSIndex);

	Profile_Sample* profileArray = pProfileElement->_pSamples;
	//������ ���� �±װ� �־����� Ȯ��
	for (int i = 0; i < pProfileElement->_vaildIndex + 1; i++)
	{
		if (_tcscmp(szName, profileArray[i].szName) == 0 && resetCount == profileManager.GetResetCount())
		{
			//�ִ� -> �� �±��� ���� ����

			//�̹� Begin ���ε� ���� �±��� Begin�� �� �� �� ���� �Ǵ� ��� -> ������..
			if (profileArray[i].lFlag == 1)
			{
				_tprintf(_T("Error: Begin called double.\n"));
				return;
			}

			profileArray[i].lFlag = 1;
			profileArray[i].lStartTime = _startTime.QuadPart * 1000000 / _freq.QuadPart;
			profileArray[i].resetCount = resetCount;
			return;
		}
	}

	//���� -> g_profileArray�� �߰��ϰ� �� ����
	(pProfileElement->_vaildIndex)++;
	int curIndex = pProfileElement->_vaildIndex;
	if (curIndex >= MAX_PROFILE_SAMPLE)
	{
		_tprintf(_T("Error: Index over maximum\n"));
		(pProfileElement->_vaildIndex)--;
		return;
	}
	//����ü �� �ʱ�ȭ
	_tcscpy_s(profileArray[curIndex].szName, _countof(Profile_Sample::szName), szName);
	profileArray[curIndex].lFlag = 1;
	profileArray[curIndex].lStartTime = _startTime.QuadPart * 1000000 / _freq.QuadPart;
	profileArray[curIndex].iMin[0] = MAXINT64;
	profileArray[curIndex].iMin[1] = MAXINT64 - 1;
	profileArray[curIndex].iMax[0] = -MAXINT64;
	profileArray[curIndex].iMax[1] = -MAXINT64 + 1;
	profileArray[curIndex].iCall = 0;
	profileArray[curIndex].iTotalTime = 0;
	profileArray[curIndex].resetCount = resetCount;
	return;
}
void Profile::ProfileEnd(const TCHAR* szName)
{
	__int64 deltaTime = 0;
	LARGE_INTEGER _endTime;
	LARGE_INTEGER _freq;
	QueryPerformanceFrequency(&_freq);
	QueryPerformanceCounter(&_endTime);
	Profile_Element* pProfileElement = (Profile_Element*)TlsGetValue(g_profilePtrTLSIndex);
	Profile_Sample* profileArray = pProfileElement->_pSamples;

	for (int i = 0; i < pProfileElement->_vaildIndex + 1; i++)
	{
		if (profileArray[i].resetCount != profileManager.GetResetCount())
			continue;

		if (_tcscmp(szName, profileArray[i].szName) == 0)
		{
			deltaTime = (_endTime.QuadPart * 1000000 / _freq.QuadPart) - profileArray[i].lStartTime;

			profileArray[i].iCall++;
			profileArray[i].lFlag = 0;
			profileArray[i].iTotalTime += deltaTime;

			if (profileArray[i].iMin[0] > deltaTime)
			{
				profileArray[i].iMin[1] = profileArray[i].iMin[0];
				profileArray[i].iMin[0] = deltaTime;

			}
			else if (profileArray[i].iMin[1] > deltaTime)
			{
				profileArray[i].iMin[1] = deltaTime;
			}

			if (profileArray[i].iMax[0] < deltaTime)
			{
				profileArray[i].iMax[1] = profileArray[i].iMax[0];
				profileArray[i].iMax[0] = deltaTime;
			}
			else if (profileArray[i].iMax[1] < deltaTime)
			{
				profileArray[i].iMax[1] = deltaTime;
			}

			break;
		}
	}
}

ProfileManager::ProfileManager()
{
	InitializeSRWLock(&_containerLock);
	//���� ������ �� ���� �ε����� �ʱ�ȭ ���̶�� �ʱ�ȭ ��Ų��.
	if (g_profilePtrTLSIndex == 0)
	{
		g_profilePtrTLSIndex = TlsAlloc();
		if (g_profilePtrTLSIndex == TLS_OUT_OF_INDEXES)
			__debugbreak();
	}
}

ProfileManager::~ProfileManager()
{
	for (auto element : _totalProfileSampleContainer)
	{
		delete element;
	}

	_totalProfileSampleContainer.clear();
}

Profile_Element* ProfileManager::Alloc()
{
	Profile_Element* pProfileElement = new Profile_Element;

	AcquireSRWLockExclusive(&_containerLock);
	_totalProfileSampleContainer.push_back(pProfileElement);
	ReleaseSRWLockExclusive(&_containerLock);

	return pProfileElement;
}

void ProfileManager::ProfileDataOutText(const TCHAR* szFileName)
{
	//g_profileArray �迭�� �������� ���Ϸ� ����Ѵ�.
	FILE* pfile;
	errno_t err = _tfopen_s(&pfile, szFileName, _T("w"));
	if (err != 0)
	{
		_tprintf(_T("FileOpen Error\n"));
		return;
	}

	_ftprintf(pfile, _T("------------------------------------------------------------------------------------------------------\n"));
	_ftprintf(pfile, _T("| %-15s | %-15s | %-15s | %-15s | %-10s |\n"),
		_T("Name"), _T("Average (us)"), _T("Min (us)"), _T("Max (us)"), _T("Call"));
	_ftprintf(pfile, _T("------------------------------------------------------------------------------------------------------\n"));


	for (auto element : _totalProfileSampleContainer)
	{
		Profile_Sample* samples = element->_pSamples;
		for (int i = 0; i < element->_vaildIndex + 1; i++)
		{
			__int64 _average = (samples[i].iTotalTime
				- (samples[i].iMax[0] + samples[i].iMax[1] + samples[i].iMin[0] + samples[i].iMin[1]))
				/ (samples[i].iCall - 4);

			_ftprintf(pfile, _T("| %-15s | %-15lld | %-15lld | %-15lld | %-10lld |\n"),
				samples[i].szName, _average, samples[i].iMin[0], samples[i].iMax[0], samples[i].iCall);
		}
		_ftprintf(pfile, _T("------------------------------------------------------------------------------------------------------\n"));
	}

	fclose(pfile);
}

void ProfileManager::ProfileReset()
{
	InterlockedExchange(&_resetFlag, TRUE);
	InterlockedIncrement(&_resetCount);

	//���� �۾�
	for (auto element : _totalProfileSampleContainer)
	{
		element->_vaildIndex = -1;
	}

	InterlockedExchange(&_resetFlag, FALSE);
}

LONG ProfileManager::GetResetFlag() const
{
	return _resetFlag;
}

LONG ProfileManager::GetResetCount() const
{
	return _resetCount;
}

Profile_Sample::Profile_Sample()
	:lFlag(0), lStartTime(0), iTotalTime(0), iCall(0), resetCount(-1)
{
}

Profile_Element::Profile_Element()
	:_vaildIndex(-1)
{
	_pSamples = new Profile_Sample[MAX_PROFILE_SAMPLE];
}

Profile_Element::~Profile_Element()
{
	delete[] _pSamples;
}

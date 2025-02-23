#include <iostream>
#include "Profiler.h"

PROFILE_SAMPLE g_profileArray[MAX_PROFILE_SAMPLE];
extern int g_profileArrayIdx = -1;


void Profile::ProfileBegin(const TCHAR* szName)
{
	LARGE_INTEGER _startTime;
	LARGE_INTEGER _freq;
	QueryPerformanceFrequency(&_freq);
	QueryPerformanceCounter(&_startTime);

	//기존에 같은 태그가 있었는지 확인
	for (int i = 0; i < g_profileArrayIdx + 1; i++)
	{
		if (_tcscmp(szName, g_profileArray[i].szName) == 0)
		{
			//있다 -> 그 태그의 정보 갱신

			//이미 Begin 중인데 같은 태그의 Begin이 한 번 더 실행 되는 경우 -> 뻑낸다..
			if (g_profileArray[i].lFlag == 1)
			{
				_tprintf(_T("Error: Begin called double."));
				return;
			}

			g_profileArray[i].lFlag = 1;
			g_profileArray[i].lStartTime = _startTime.QuadPart * 1000000 / _freq.QuadPart;
			return;
		}
	}

	//없다 -> g_profileArray에 추가하고 값 갱신
	g_profileArrayIdx++;
	if (g_profileArrayIdx >= MAX_PROFILE_SAMPLE)
	{
		_tprintf(_T("Error: Index over maximum"));
		g_profileArrayIdx--;
		return;
	}
	//구조체 값 등록
	_tcscpy_s(g_profileArray[g_profileArrayIdx].szName, _countof(PROFILE_SAMPLE::szName), szName);
	g_profileArray[g_profileArrayIdx].lFlag = 1;
	g_profileArray[g_profileArrayIdx].lStartTime = _startTime.QuadPart * 1000000 / _freq.QuadPart;
	g_profileArray[g_profileArrayIdx].iMin[0] = MAXINT64;
	g_profileArray[g_profileArrayIdx].iMin[1] = MAXINT64 - 1;
	g_profileArray[g_profileArrayIdx].iMax[0] = -MAXINT64;
	g_profileArray[g_profileArrayIdx].iMax[1] = -MAXINT64 + 1;
	return;
}
void Profile::ProfileEnd(const TCHAR* szName)
{
	__int64 deltaTime = 0;
	LARGE_INTEGER _endTime;
	LARGE_INTEGER _freq;
	QueryPerformanceFrequency(&_freq);
	QueryPerformanceCounter(&_endTime);

	for (int i = 0; i < g_profileArrayIdx + 1; i++)
	{
		if (_tcscmp(szName, g_profileArray[i].szName) == 0)
		{
			deltaTime = (_endTime.QuadPart * 1000000 / _freq.QuadPart) - g_profileArray[i].lStartTime;

			g_profileArray[i].iCall++;
			g_profileArray[i].lFlag = 0;
			g_profileArray[i].iTotalTime += deltaTime;

			if (g_profileArray[i].iMin[0] > deltaTime)
			{
				g_profileArray[i].iMin[0] = deltaTime;
			}
			else if (g_profileArray[i].iMin[1] > deltaTime)
			{
				g_profileArray[i].iMin[1] = deltaTime;
			}

			if (g_profileArray[i].iMax[0] < deltaTime)
			{
				g_profileArray[i].iMax[0] = deltaTime;
			}
			else if (g_profileArray[i].iMax[1] < deltaTime)
			{
				g_profileArray[i].iMax[1] = deltaTime;
			}

		}
	}
}

void ProfileDataOutText(const TCHAR* szFileName)
{
	//g_profileArray 배열의 정보들을 파일로 출력한다.
	FILE* pfile;
	errno_t err = _tfopen_s(&pfile, szFileName, _T("w"));
	if (err != 0)
	{
		_tprintf(_T("FileOpen Error"));
		return;
	}

	_ftprintf(pfile, _T("------------------------------------------------------------------------------------------------------\n"));
	_ftprintf(pfile, _T("\t\tName | \tAverage | \t\tMin | \t\tMax | \t\tCall |\n"));
	_ftprintf(pfile, _T("------------------------------------------------------------------------------------------------------\n"));

	for (int i = 0; i < g_profileArrayIdx + 1; i++)
	{
		double _average = (g_profileArray[i].iTotalTime
			- (g_profileArray[i].iMax[0] + g_profileArray[i].iMax[1] + g_profileArray[i].iMin[0] + g_profileArray[i].iMin[1]))
			/ (g_profileArray[i].iCall - 4);

		_ftprintf(pfile, _T("\t\t%s | \t\t%lld us| \t\t%lld us| \t\t%lld us| \t\t%lld |\n"),
			g_profileArray[i].szName, _average, g_profileArray[i].iMin[0], g_profileArray[i].iMax[0], g_profileArray[i].iCall);
	}
	_ftprintf(pfile, _T("------------------------------------------------------------------------------------------------------\n"));

	fclose(pfile);
}

void ProfileReset(void)
{
	//g_profileArray 배열을 초기화 한다.
	g_profileArrayIdx = -1;
}
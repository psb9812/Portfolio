/*
	2024.04.19
	프로파일러

	- 사용법
	1. PROFILE을 define해줍니다. (이 헤더 보다 위에 해줘야 합니다)
	2. 측정하고 싶은 영역을 {}로 잡습니다.
	3. START_PROFILEING(객체 변수 이름, 파일에 저장될 태그이름)로 프로파일러 객체를 생성하면서 시간을 측정합니다.
	4. ProfileDataOutText(결과 파일 이름) 함수로 결과를 출력합니다.
*/


#pragma once
#include <Windows.h>
#include <tchar.h>

#ifdef PROFILE
#define START_PROFILEING(varName, TagName) Profile varName(_T(TagName))
#else // PROFILE
#define START_PROFILEING(varName, TagName) 
#endif


#define MAX_PROFILE_SAMPLE 15		//최대 측정할 수 있는 요소 수

struct PROFILE_SAMPLE
{
	long			lFlag;				// 프로파일의 사용 여부. (배열시에만)
	TCHAR			szName[64];			// 프로파일 샘플 이름.

	__int64	        lStartTime;			// 프로파일 샘플 실행 시간.

	__int64			iTotalTime;			// 전체 사용시간 카운터 Time.	(출력시 호출회수로 나누어 평균 구함)
	__int64			iMin[2];			// 최소 사용시간 카운터 Time.	(초단위로 계산하여 저장 / [0] 가장최소 [1] 다음 최소 [2])
	__int64			iMax[2];			// 최대 사용시간 카운터 Time.	(초단위로 계산하여 저장 / [0] 가장최대 [1] 다음 최대 [2])

	__int64			iCall;				// 누적 호출 횟수.
};

extern PROFILE_SAMPLE g_profileArray[MAX_PROFILE_SAMPLE];
extern int g_profileArrayIdx;

void ProfileDataOutText(const TCHAR* szFileName);
void ProfileReset(void);

class Profile
{
public:
	Profile(const TCHAR* tag)
	{
		ProfileBegin(tag);
		_tag = tag;
	}
	~Profile()
	{
		ProfileEnd(_tag);
	}

	const TCHAR* _tag;

private:
	void ProfileBegin(const TCHAR* szName);
	void ProfileEnd(const TCHAR* szName);
};
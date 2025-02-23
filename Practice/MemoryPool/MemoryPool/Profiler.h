/*
	2024.10.08
	프로파일러

	- 사용법
	1. PROFILE을 define해줍니다. (이 헤더 보다 위에 해줘야 합니다)
	2. 측정하고 싶은 영역을 {}로 잡습니다.
	3. START_PROFILEING(객체 변수 이름, "파일에 저장될 태그이름")로 프로파일러 객체를 생성하면서 시간을 측정합니다.
	4. ProfileManager::ProfileDataOutText(결과 파일 이름) 함수로 결과를 출력합니다.

	최소 4번 이상은 돌려야 합니다. 최소, 최소+1, 최대, 최대 -1의 경우를 빼고 측정하기 때문입니다.
*/

#pragma once
#include <Windows.h>
#include <tchar.h>
#include <list>
#include <vector>

#ifdef PROFILE
#define START_PROFILEING(varName, TagName) Profile varName(_T(TagName))
#else // PROFILE
#define START_PROFILEING(varName, TagName)
#endif

constexpr int MAX_THREAD_NUM = 20;		//프로파일링 할 수 있는 최대 스레드 수
constexpr int MAX_PROFILE_SAMPLE = 15;		//최대 측정할 수 있는 샘플 수

struct Profile_Sample
{
	long			lFlag;				// 프로파일의 사용 여부. (배열시에만)
	TCHAR			szName[64];			// 프로파일 샘플 이름.

	__int64	        lStartTime;			// 프로파일 샘플 실행 시간.

	__int64			iTotalTime;			// 전체 사용시간 카운터 Time.	(출력시 호출회수로 나누어 평균 구함)
	__int64			iMin[2];			// 최소 사용시간 카운터 Time.	(초단위로 계산하여 저장 / [0] 가장최소 [1] 다음 최소 [2])
	__int64			iMax[2];			// 최대 사용시간 카운터 Time.	(초단위로 계산하여 저장 / [0] 가장최대 [1] 다음 최대 [2])

	__int64			iCall;				// 누적 호출 횟수.
	int				resetCount;			// 이 샘플이 씌어진 시점의 리셋 카운트

	Profile_Sample();
};

struct Profile_Element
{
	Profile_Sample* _pSamples;
	int _vaildIndex;

	Profile_Element();
	~Profile_Element();
};

extern int g_profilePtrTLSIndex;

class ProfileManager
{
private:
	//모든 스레드가 수집한 ProfileSample 정보를 담는 컨테이너
	static std::list<Profile_Element*> _totalProfileSampleContainer;
	SRWLOCK	_containerLock;
	static LONG _resetFlag;
	static LONG _resetCount;
public:
	ProfileManager();
	~ProfileManager();

	Profile_Element* Alloc();
	static void ProfileDataOutText(const TCHAR* szFileName);
	static void ProfileReset();
	LONG GetResetFlag() const;
	LONG GetResetCount() const;
};



class Profile
{
public:
	Profile(const TCHAR* tag);

	~Profile();


	const TCHAR* _tag;
	static ProfileManager profileManager;

private:
	void ProfileBegin(const TCHAR* szName);
	void ProfileEnd(const TCHAR* szName);
};
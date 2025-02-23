#pragma once
#include <Windows.h>
#include <tchar.h>

#ifdef PROFILE
#define START_PROFILEING(varName, TagName) Profile varName(_T(TagName))
#else // PROFILE
#define START_PROFILEING(varName, TagName) 
#endif


#define MAX_PROFILE_SAMPLE 15

struct PROFILE_SAMPLE
{
	long			lFlag;				// ���������� ��� ����. (�迭�ÿ���)
	TCHAR			szName[64];			// �������� ���� �̸�.

	__int64	        lStartTime;			// �������� ���� ���� �ð�.

	__int64			iTotalTime;			// ��ü ���ð� ī���� Time.	(��½� ȣ��ȸ���� ������ ��� ����)
	__int64			iMin[2];			// �ּ� ���ð� ī���� Time.	(�ʴ����� ����Ͽ� ���� / [0] �����ּ� [1] ���� �ּ� [2])
	__int64			iMax[2];			// �ִ� ���ð� ī���� Time.	(�ʴ����� ����Ͽ� ���� / [0] �����ִ� [1] ���� �ִ� [2])

	__int64			iCall;				// ���� ȣ�� Ƚ��.
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
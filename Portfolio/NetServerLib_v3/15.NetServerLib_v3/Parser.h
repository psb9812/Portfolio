#pragma once
#include <Windows.h>
#include <tchar.h>

class Parser
{
public:

	Parser();
	~Parser();
	bool LoadFile(const TCHAR* fileName);
	bool GetValue(const TCHAR* name, int* value);
	bool GetValue(const TCHAR* name, float* value);
	bool GetValue(const TCHAR* name, TCHAR* value, int valueLength);

private:
	TCHAR* fileBuffer;
	TCHAR* cursor;

	//다음 단어를 얻어오는 함수
	bool GetNextWord(TCHAR** charBuffer, int* length);

	//띄어쓰기, tab같은 필요없는 부분을 스킵한다.
	//이 함수를 호출하면 바로 의미있는 단어로 커서가 간다.
	bool SkipNoneCommand(void);
};
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

	//���� �ܾ ������ �Լ�
	bool GetNextWord(TCHAR** charBuffer, int* length);

	//����, tab���� �ʿ���� �κ��� ��ŵ�Ѵ�.
	//�� �Լ��� ȣ���ϸ� �ٷ� �ǹ��ִ� �ܾ�� Ŀ���� ����.
	bool SkipNoneCommand(void);
};
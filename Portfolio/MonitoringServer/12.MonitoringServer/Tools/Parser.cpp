#include <iostream>
#include "Parser.h"

Parser::Parser()
{
	fileBuffer = nullptr;
	cursor = nullptr;
}

Parser::~Parser()
{
	delete[] fileBuffer;
}

bool Parser::LoadFile(const TCHAR* fileName)
{
	FILE* filePtr;
	//errno_t err = _tfopen_s(&filePtr, fileName, _T("rt,ccs=UTF-16LE"));
	errno_t err = _tfopen_s(&filePtr, fileName, _T("rt,ccs=UTF-8"));
	if (err != 0)
	{
		return false;
	}

	//���Ͽ� �ִ� �� �� �濡 �о����
	int stringLength = 0;
	TCHAR ch;
	while ((ch = _fgettc(filePtr)) != _TEOF)
	{
		stringLength++;
	}
	stringLength++;
	fseek(filePtr, 0, SEEK_SET);

	fileBuffer = new TCHAR[stringLength + 1];

	int readCount = fread_s(fileBuffer, stringLength * sizeof(TCHAR), stringLength * sizeof(TCHAR), 1, filePtr);

	fileBuffer[stringLength] = '\0';

	fclose(filePtr);
	return true;
}
bool Parser::GetValue(const TCHAR* name, int* value)
{
	TCHAR* targetPtr;
	TCHAR wordBuffer[256];
	int length = 0;
	//Ŀ���� �� �տ��� �� ĭ ������ �д�.
	//�� �տ��� ���ڵ� ����� ��Ÿ���� �����Ͱ� ����ֱ� �����̴�.
	cursor = fileBuffer + 1;


	while (GetNextWord(&targetPtr, &length))
	{
		memset(wordBuffer, 0, 256);
		memcpy(wordBuffer, targetPtr, length * sizeof(TCHAR));

		if (0 == _tcscmp(wordBuffer, name))
		{
			if (GetNextWord(&targetPtr, &length))
			{
				memset(wordBuffer, 0, 256);
				memcpy(wordBuffer, targetPtr, length * sizeof(TCHAR));

				if (0 == _tcscmp(wordBuffer, _T("=")))
				{
					if (GetNextWord(&targetPtr, &length))
					{
						memset(wordBuffer, 0, 256);
						memcpy(wordBuffer, targetPtr, length * sizeof(TCHAR));
						*value = _tstoi(wordBuffer);
						return true;
					}
					return false;
				}
				return false;
			}
			return false;
		}
	}
	return false;
}
bool Parser::GetValue(const TCHAR* name, float* value)
{
	TCHAR* targetPtr;
	TCHAR wordBuffer[256];
	int length = 0;
	//Ŀ���� �� �տ��� �� ĭ ������ �д�.
	//�� �տ��� ���ڵ� ����� ��Ÿ���� �����Ͱ� ����ֱ� �����̴�.
	cursor = fileBuffer + 1;


	while (GetNextWord(&targetPtr, &length))
	{
		memset(wordBuffer, 0, 256);
		memcpy(wordBuffer, targetPtr, length * sizeof(TCHAR));

		if (0 == _tcscmp(wordBuffer, name))
		{
			if (GetNextWord(&targetPtr, &length))
			{
				memset(wordBuffer, 0, 256);
				memcpy(wordBuffer, targetPtr, length * sizeof(TCHAR));

				if (0 == _tcscmp(wordBuffer, _T("=")))
				{
					if (GetNextWord(&targetPtr, &length))
					{
						memset(wordBuffer, 0, 256);
						memcpy(wordBuffer, targetPtr, length * sizeof(TCHAR));
						*value = static_cast<float>(_wtof(wordBuffer));
						return true;
					}
					return false;
				}
				return false;
			}
			return false;
		}
	}
	return false;
}
bool Parser::GetValue(const TCHAR* name, TCHAR* value, int valueLength)
{
	TCHAR* targetPtr;
	TCHAR wordBuffer[256];
	int length = 0;
	//Ŀ���� �� �տ��� �� ĭ ������ �д�.
	//�� �տ��� ���ڵ� ����� ��Ÿ���� �����Ͱ� ����ֱ� �����̴�.
	cursor = fileBuffer + 1;


	while (GetNextWord(&targetPtr, &length))
	{
		memset(wordBuffer, 0, 256);
		memcpy(wordBuffer, targetPtr, length * sizeof(TCHAR));

		if (0 == _tcscmp(wordBuffer, name))
		{
			if (GetNextWord(&targetPtr, &length))
			{
				memset(wordBuffer, 0, 256);
				memcpy(wordBuffer, targetPtr, length * sizeof(TCHAR));

				if (0 == _tcscmp(wordBuffer, _T("=")))
				{
					if (GetNextWord(&targetPtr, &length))
					{
						memset(wordBuffer, 0, 256);
						memcpy(wordBuffer, targetPtr, length * sizeof(TCHAR));
						_tcscpy_s(value, valueLength, wordBuffer);
						return true;
					}
					return false;
				}
				return false;
			}
			return false;
		}
	}
	return false;
}

bool Parser::GetNextWord(TCHAR** targetPPtr, int* length)
{
	SkipNoneCommand();
	*targetPPtr = cursor;
	*length = 0;
	while (1)
	{
		if (*cursor == _T(' ') || *cursor == _T('\t') || *cursor == _T('\n') || *cursor == _T(',') || *cursor == _T('\0') || *cursor == _T('\"'))
		{
			break;
		}
		cursor++;
		(*length)++;
	}
	return true;
}
bool Parser::SkipNoneCommand(void)
{
	while (1)
	{
		if (*cursor == _T(' ') || *cursor == _T('\t') || *cursor == _T('\n') || *cursor == _T(',') || *cursor == _T('0x0d') || *cursor == _T('\"'))
		{
			cursor++;
			if (*cursor == _T('\0'))
				return false;
		}
		else
		{
			if (*cursor == _T('/'))
			{
				cursor++;
				if (*cursor == _T('/'))
				{
					//�� �� �ּ����� �Ǵ��ϰ� \n�� ���� ������ �Ѿ
					while (*cursor != _T('\n'))
					{
						cursor++;
						if (*cursor == _T('\0'))
							return false;
					}
					cursor++;
				}
				else if (*cursor == _T('*'))
				{
					//���� �� �ּ��� ��� üũ
					while (!(*cursor == _T('*') && *(cursor + 1) == _T('/')))
					{
						cursor++;
						if (*cursor == _T('\0'))
							return false;
					}
					cursor += 2;
				}
				else
				{
					cursor--;
					break;
				}
			}
			else
				break;
		}
	}
	return true;
}
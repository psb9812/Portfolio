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

	//파일에 있는 값 한 방에 읽어오기
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
	//커서를 맨 앞에서 한 칸 앞으로 둔다.
	//맨 앞에는 인코딩 방식을 나타내는 데이터가 들어있기 때문이다.
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
	//커서를 맨 앞에서 한 칸 앞으로 둔다.
	//맨 앞에는 인코딩 방식을 나타내는 데이터가 들어있기 때문이다.
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
	//커서를 맨 앞에서 한 칸 앞으로 둔다.
	//맨 앞에는 인코딩 방식을 나타내는 데이터가 들어있기 때문이다.
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
					//한 줄 주석임을 판단하고 \n을 만날 때까지 넘어감
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
					//여러 줄 주석인 경우 체크
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
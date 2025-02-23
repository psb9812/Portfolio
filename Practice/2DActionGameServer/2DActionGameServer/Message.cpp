#include "Message.h"
#include <cassert>

Message::Message()
	:_bufferSize(en_Message::BUFFER_DEFAULT_SIZE), _dataSize(0)
{
	_pBuffer = new char[_bufferSize];
	_pWritePos = _pBuffer;
	_pReadPos = _pBuffer;
	_pEnd = _pBuffer + _bufferSize;

}
Message::Message(int bufferSize)
	:_bufferSize(bufferSize), _dataSize(0)
{
	_pBuffer = new char[_bufferSize];
	_pWritePos = _pBuffer;
	_pReadPos = _pBuffer;
	_pEnd = _pBuffer + _bufferSize;

}

Message::~Message()
{
	delete[] _pBuffer;
}

void Message::Clear(void)
{
	_pWritePos = _pBuffer;
	_pReadPos = _pBuffer;
	_dataSize = 0;
}

bool Message::Resize(int size)
{
	if (size <= _bufferSize || _bufferSize >= BUFFER_MAX_SIZE)
	{
		return false;
	}

	if (size > BUFFER_MAX_SIZE)
	{
		_bufferSize = BUFFER_MAX_SIZE;
	}
	else
	{
		_bufferSize = size;
	}

	char* tempBuffer = new char[_bufferSize];
	memcpy_s(tempBuffer, _bufferSize, _pBuffer, _bufferSize - size);
	_pWritePos = tempBuffer + (_pWritePos - _pBuffer);
	_pReadPos = tempBuffer + (_pReadPos - _pBuffer);
	_pEnd = tempBuffer + _bufferSize;
	delete[] _pBuffer;
	_pBuffer = tempBuffer;
	return true;
}

//////////////////////////////////////////////////////////////////////////
	// 버퍼 Pos 이동. (음수이동은 안됨)
	// GetBufferPtr 함수를 이용하여 외부에서 강제로 버퍼 내용을 수정할 경우 사용. 
	//
	// Parameters: (int) 이동 사이즈.
	// Return: (int) 이동된 사이즈.
	//////////////////////////////////////////////////////////////////////////
int	Message::MoveWritePos(int size)
{
	if (size < 0 || _pWritePos + size > _pEnd)
		return 0;

	_pWritePos += size;
	_dataSize += size;
	return size;

}
int	Message::MoveReadPos(int size)
{
	if (size < 0 || _pReadPos + size > _pReadPos + _dataSize)
		return 0;

	_pReadPos += size;
	_dataSize -= size;
	return size;
}


Message& Message::operator = (Message& SrMessage)
{
	_bufferSize = SrMessage._bufferSize;
	_dataSize = SrMessage._dataSize;
	_pBuffer = new char[SrMessage._bufferSize];
	_pEnd = _pBuffer + _bufferSize;
	_pWritePos = _pBuffer + (SrMessage._pWritePos - SrMessage._pBuffer);
	_pReadPos = _pBuffer + (SrMessage._pReadPos - SrMessage._pBuffer);
	return *this;
}

//////////////////////////////////////////////////////////////////////////
// 넣기.	각 변수 타입마다 모두 만듬.
//////////////////////////////////////////////////////////////////////////
Message& Message::operator << (unsigned char value)
{
	int valueSize = sizeof(decltype(value));
	//넣을 수 있는지 체크
	if (GetBufferFreeSize() < valueSize)
	{
		//리사이즈
		if (!Resize(_bufferSize * 2))
		{
			//로그 찍기
		}
		else
		{
			//로그 찍기
		}
	}

	auto castPointer = (decltype(value)*)_pWritePos;
	*castPointer = value;

	_pWritePos += valueSize;
	_dataSize += valueSize;
	return *this;
}
Message& Message::operator << (char value)
{
	int valueSize = sizeof(decltype(value));
	//넣을 수 있는지 체크
	if (GetBufferFreeSize() < valueSize)
	{
		//리사이즈
		if (!Resize(_bufferSize * 2))
		{
			//로그 찍기
		}
		else
		{
			//로그 찍기
		}
	}

	auto castPointer = (decltype(value)*)_pWritePos;
	*castPointer = value;

	_pWritePos += valueSize;
	_dataSize += valueSize;
	return *this;
}
Message& Message::operator << (short value)
{
	int valueSize = sizeof(decltype(value));
	//넣을 수 있는지 체크
	if (GetBufferFreeSize() < valueSize)
	{
		//리사이즈
		if (!Resize(_bufferSize * 2))
		{
			//로그 찍기
		}
		else
		{
			//로그 찍기
		}
	}

	auto castPointer = (decltype(value)*)_pWritePos;
	*castPointer = value;

	_pWritePos += valueSize;
	_dataSize += valueSize;
	return *this;
}
Message& Message::operator << (unsigned short value)
{
	int valueSize = sizeof(decltype(value));
	//넣을 수 있는지 체크
	if (GetBufferFreeSize() < valueSize)
	{
		//리사이즈
		if (!Resize(_bufferSize * 2))
		{
			//로그 찍기
		}
		else
		{
			//로그 찍기
		}
	}

	auto castPointer = (decltype(value)*)_pWritePos;
	*castPointer = value;

	_pWritePos += valueSize;
	_dataSize += valueSize;
	return *this;
}
Message& Message::operator << (int value)
{
	int valueSize = sizeof(decltype(value));
	//넣을 수 있는지 체크
	if (GetBufferFreeSize() < valueSize)
	{
		//리사이즈
		if (!Resize(_bufferSize * 2))
		{
			//로그 찍기
		}
		else
		{
			//로그 찍기
		}
	}

	auto castPointer = (decltype(value)*)_pWritePos;
	*castPointer = value;

	_pWritePos += valueSize;
	_dataSize += valueSize;
	return *this;
}
Message& Message::operator << (long value)
{
	int valueSize = sizeof(decltype(value));
	//넣을 수 있는지 체크
	if (GetBufferFreeSize() < valueSize)
	{
		//리사이즈
		if (!Resize(_bufferSize * 2))
		{
			//로그 찍기
		}
		else
		{
			//로그 찍기
		}
	}

	auto castPointer = (decltype(value)*)_pWritePos;
	*castPointer = value;

	_pWritePos += valueSize;
	_dataSize += valueSize;
	return *this;
}
Message& Message::operator << (float value)
{
	int valueSize = sizeof(decltype(value));
	//넣을 수 있는지 체크
	if (GetBufferFreeSize() < valueSize)
	{
		//리사이즈
		if (!Resize(_bufferSize * 2))
		{
			//로그 찍기
		}
		else
		{
			//로그 찍기
		}
	}

	auto castPointer = (decltype(value)*)_pWritePos;
	*castPointer = value;

	_pWritePos += valueSize;
	_dataSize += valueSize;
	return *this;
}
Message& Message::operator << (__int64 value)
{
	int valueSize = sizeof(decltype(value));
	//넣을 수 있는지 체크
	if (GetBufferFreeSize() < valueSize)
	{
		//리사이즈
		if (!Resize(_bufferSize * 2))
		{
			//로그 찍기
		}
		else
		{
			//로그 찍기
		}
	}

	auto castPointer = (decltype(value)*)_pWritePos;
	*castPointer = value;

	_pWritePos += valueSize;
	_dataSize += valueSize;
	return *this;
}
Message& Message::operator << (double value)
{
	int valueSize = sizeof(decltype(value));
	//넣을 수 있는지 체크
	if (GetBufferFreeSize() < valueSize)
	{
		//리사이즈
		if (!Resize(_bufferSize * 2))
		{
			//로그 찍기
		}
		else
		{
			//로그 찍기
		}
	}

	auto castPointer = (decltype(value)*)_pWritePos;
	*castPointer = value;

	_pWritePos += valueSize;
	_dataSize += valueSize;
	return *this;
}


//////////////////////////////////////////////////////////////////////////
// 빼기.	각 변수 타입마다 모두 만듬.
//////////////////////////////////////////////////////////////////////////
Message& Message::operator >> (BYTE& value)
{
	int valueSize = sizeof(decltype(value));
	if (_dataSize < valueSize)
	{
		//꺼낼 수 없는데 꺼내려고 시도하면 바로 예외를 던져서 문제를 파악한다.
		//프로토콜 대로 꺼내려고 할 텐데 없다면 프로그래머의 실수이므로 빠르게 파악하기 위함이다.
		throw 0;
	}

	auto castPointer = (BYTE*)_pReadPos;
	value = *castPointer;

	_pReadPos += valueSize;
	_dataSize -= valueSize;
	return *this;
}
Message& Message::operator >> (char& value)
{
	int valueSize = sizeof(decltype(value));
	if (_dataSize < valueSize)
	{
		//예외 던지기
		throw 0;
	}

	auto castPointer = (char*)_pReadPos;
	value = *castPointer;

	_pReadPos += valueSize;
	_dataSize -= valueSize;
	return *this;
}
Message& Message::operator >> (short& value)
{
	int valueSize = sizeof(decltype(value));
	if (_dataSize < valueSize)
	{
		//예외 던지기
		throw 0;
	}

	auto castPointer = (short*)_pReadPos;
	value = *castPointer;

	_pReadPos += valueSize;
	_dataSize -= valueSize;
	return *this;
}
Message& Message::operator >> (WORD& value)
{
	int valueSize = sizeof(decltype(value));
	if (_dataSize < valueSize)
	{
		//예외 던지기
		throw 0;
	}

	auto castPointer = (WORD*)_pReadPos;
	value = *castPointer;

	_pReadPos += valueSize;
	_dataSize -= valueSize;
	return *this;
}
Message& Message::operator >> (int& value)
{
	int valueSize = sizeof(decltype(value));
	if (_dataSize < valueSize)
	{
		//예외 던지기
		throw 0;
	}

	auto castPointer = (int*)_pReadPos;
	value = *castPointer;

	_pReadPos += valueSize;
	_dataSize -= valueSize;
	return *this;
}
Message& Message::operator >> (DWORD& value)
{
	int valueSize = sizeof(decltype(value));
	if (_dataSize < valueSize)
	{
		//예외 던지기
		throw 0;
	}

	auto castPointer = (DWORD*)_pReadPos;
	value = *castPointer;

	_pReadPos += valueSize;
	_dataSize -= valueSize;
	return *this;
}
Message& Message::operator >> (float& value)
{
	int valueSize = sizeof(decltype(value));
	if (_dataSize < valueSize)
	{
		//예외 던지기
		throw 0;
	}

	auto castPointer = (float*)_pReadPos;
	value = *castPointer;

	_pReadPos += valueSize;
	_dataSize -= valueSize;
	return *this;
}
Message& Message::operator >> (__int64& value)
{
	int valueSize = sizeof(decltype(value));
	if (_dataSize < valueSize)
	{
		//예외 던지기
		throw 0;
	}

	auto castPointer = (__int64*)_pReadPos;
	value = *castPointer;

	_pReadPos += valueSize;
	_dataSize -= valueSize;
	return *this;
}
Message& Message::operator >> (double& value)
{
	int valueSize = sizeof(decltype(value));
	if (_dataSize < valueSize)
	{
		//예외 던지기
		throw 0;
	}

	auto castPointer = (double*)_pReadPos;
	value = *castPointer;

	_pReadPos += valueSize;
	_dataSize -= valueSize;
	return *this;
}



//////////////////////////////////////////////////////////////////////////
// 데이타 얻기.
//
// Parameters: (char *)Dest 포인터. (int)Size.
// Return: (int)복사한 사이즈.
//////////////////////////////////////////////////////////////////////////
int	Message::GetData(char* pDest, int requestSize)
{
	if (_dataSize < requestSize)
		return 0;

	memcpy_s(pDest, requestSize, _pReadPos, requestSize);
	_pReadPos += requestSize;
	_dataSize -= requestSize;
	return requestSize;
}


int	Message::PutData(char* pSrc, int srcSize)
{
	int spaceOfWrite = GetBufferFreeSize();

	if (srcSize > spaceOfWrite)
	{
		//리사이즈
		if (!Resize(_bufferSize * 2))
		{
			//로그 찍기
		}
		//로그 찍기
	}
	memcpy_s(_pWritePos, spaceOfWrite, pSrc, srcSize);

	_pWritePos += srcSize;
	_dataSize += srcSize;
	return srcSize;
}

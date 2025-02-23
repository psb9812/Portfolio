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
	// ���� Pos �̵�. (�����̵��� �ȵ�)
	// GetBufferPtr �Լ��� �̿��Ͽ� �ܺο��� ������ ���� ������ ������ ��� ���. 
	//
	// Parameters: (int) �̵� ������.
	// Return: (int) �̵��� ������.
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
// �ֱ�.	�� ���� Ÿ�Ը��� ��� ����.
//////////////////////////////////////////////////////////////////////////
Message& Message::operator << (unsigned char value)
{
	int valueSize = sizeof(decltype(value));
	//���� �� �ִ��� üũ
	if (GetBufferFreeSize() < valueSize)
	{
		//��������
		if (!Resize(_bufferSize * 2))
		{
			//�α� ���
		}
		else
		{
			//�α� ���
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
	//���� �� �ִ��� üũ
	if (GetBufferFreeSize() < valueSize)
	{
		//��������
		if (!Resize(_bufferSize * 2))
		{
			//�α� ���
		}
		else
		{
			//�α� ���
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
	//���� �� �ִ��� üũ
	if (GetBufferFreeSize() < valueSize)
	{
		//��������
		if (!Resize(_bufferSize * 2))
		{
			//�α� ���
		}
		else
		{
			//�α� ���
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
	//���� �� �ִ��� üũ
	if (GetBufferFreeSize() < valueSize)
	{
		//��������
		if (!Resize(_bufferSize * 2))
		{
			//�α� ���
		}
		else
		{
			//�α� ���
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
	//���� �� �ִ��� üũ
	if (GetBufferFreeSize() < valueSize)
	{
		//��������
		if (!Resize(_bufferSize * 2))
		{
			//�α� ���
		}
		else
		{
			//�α� ���
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
	//���� �� �ִ��� üũ
	if (GetBufferFreeSize() < valueSize)
	{
		//��������
		if (!Resize(_bufferSize * 2))
		{
			//�α� ���
		}
		else
		{
			//�α� ���
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
	//���� �� �ִ��� üũ
	if (GetBufferFreeSize() < valueSize)
	{
		//��������
		if (!Resize(_bufferSize * 2))
		{
			//�α� ���
		}
		else
		{
			//�α� ���
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
	//���� �� �ִ��� üũ
	if (GetBufferFreeSize() < valueSize)
	{
		//��������
		if (!Resize(_bufferSize * 2))
		{
			//�α� ���
		}
		else
		{
			//�α� ���
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
	//���� �� �ִ��� üũ
	if (GetBufferFreeSize() < valueSize)
	{
		//��������
		if (!Resize(_bufferSize * 2))
		{
			//�α� ���
		}
		else
		{
			//�α� ���
		}
	}

	auto castPointer = (decltype(value)*)_pWritePos;
	*castPointer = value;

	_pWritePos += valueSize;
	_dataSize += valueSize;
	return *this;
}


//////////////////////////////////////////////////////////////////////////
// ����.	�� ���� Ÿ�Ը��� ��� ����.
//////////////////////////////////////////////////////////////////////////
Message& Message::operator >> (BYTE& value)
{
	int valueSize = sizeof(decltype(value));
	if (_dataSize < valueSize)
	{
		//���� �� ���µ� �������� �õ��ϸ� �ٷ� ���ܸ� ������ ������ �ľ��Ѵ�.
		//�������� ��� �������� �� �ٵ� ���ٸ� ���α׷����� �Ǽ��̹Ƿ� ������ �ľ��ϱ� �����̴�.
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
		//���� ������
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
		//���� ������
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
		//���� ������
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
		//���� ������
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
		//���� ������
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
		//���� ������
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
		//���� ������
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
		//���� ������
		throw 0;
	}

	auto castPointer = (double*)_pReadPos;
	value = *castPointer;

	_pReadPos += valueSize;
	_dataSize -= valueSize;
	return *this;
}



//////////////////////////////////////////////////////////////////////////
// ����Ÿ ���.
//
// Parameters: (char *)Dest ������. (int)Size.
// Return: (int)������ ������.
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
		//��������
		if (!Resize(_bufferSize * 2))
		{
			//�α� ���
		}
		//�α� ���
	}
	memcpy_s(_pWritePos, spaceOfWrite, pSrc, srcSize);

	_pWritePos += srcSize;
	_dataSize += srcSize;
	return srcSize;
}

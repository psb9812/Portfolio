#include "Message.h"
#include "Windows.h"

TLSMemoryPool<Message> Message::s_messagePool(50, false);

Message::Message()
	:_bufferSize(BUFFER_DEFAULT_SIZE), _dataSize(0), _refCount(0), _isEncode(false)
{
	_pBuffer = new char[_bufferSize + HEADER_SIZE];
	if (_pBuffer == nullptr)
	{
		__debugbreak();
	}
	_pBegin = _pBuffer + HEADER_SIZE;	//���� �������� ���� �����ϴ� ����
	_pWritePos = _pBegin;
	_pReadPos = _pBegin;
	_pEnd = _pBegin + _bufferSize;
}
Message::Message(int bufferSize)
	:_bufferSize(bufferSize), _dataSize(0), _refCount(0), _isEncode(false)
{
	_pBuffer = new char[_bufferSize + HEADER_SIZE];
	if (_pBuffer == nullptr)
	{
		__debugbreak();
	}
	_pBegin = _pBuffer + HEADER_SIZE;	//���� �������� ���� �����ϴ� ����
	_pWritePos = _pBegin;
	_pReadPos = _pBegin;
	_pEnd = _pBegin + _bufferSize;
}

Message::~Message()
{
	delete[] _pBuffer;
}

void Message::Clear(void)
{
	_pWritePos = _pBegin;
	_pReadPos = _pBegin;
	_dataSize = 0;
	_isEncode = false;
	_refCount = 0;
}

bool Message::Resize(int size)
{
	if (size <= _bufferSize || _bufferSize >= BUFFER_MAX_SIZE)
	{
		return false;
	}

	int prevTotalBufferSize = _bufferSize + HEADER_SIZE;

	if (size > BUFFER_MAX_SIZE)
	{
		_bufferSize = BUFFER_MAX_SIZE;
	}
	else
	{
		_bufferSize = size;
	}

	int totalBufferSize = _bufferSize + HEADER_SIZE;

	char* tempBuffer = new char[totalBufferSize];
	memcpy_s(tempBuffer, totalBufferSize, _pBuffer, prevTotalBufferSize);

	char* pNewBegin = tempBuffer + HEADER_SIZE;
	_pWritePos = pNewBegin + (_pWritePos - _pBegin);
	_pReadPos = pNewBegin + (_pReadPos - _pBegin);
	_pEnd = pNewBegin + _bufferSize;
	_pBegin = pNewBegin;
	delete[] _pBuffer;
	_pBuffer = tempBuffer;

	//�α� �����
	//SystemLog::GetInstance()->WriteLog(L"SYSTEM", SystemLog::LOG_LEVEL_SYSTEM, L"SerializeBuffer ReSize!! size : %d", size);

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

LONG Message::AddRefCount()
{
	return InterlockedIncrement(&_refCount);
}

LONG Message::SubRefCount()
{
	LONG ret = InterlockedDecrement(&_refCount);

	//_refCount�� 0�̶�� Ǯ�� ��ȯ�Ѵ�.
	if (ret == 0)
	{
		Message::Free(this);
		return -1;
	}
	else
	{
		return ret;
	}
}

unsigned char Message::GetCheckSum()
{
	UINT64 sum = 0;

	//���̷ε带 1byte�� sum�� ���Ѵ�.
	for (int i = 0; i < _dataSize; i++)
	{
		sum += (UINT64)_pBegin[i];
	}

	return sum % 256;
}

void Message::Encode(int staticKey)
{
	//üũ���� ���̷ε� �κ��� ���ڵ� �Ѵ�.
	char* pStart = _pBegin - 1;		//üũ�� ��ġ���� ���ڵ� ����
	unsigned char randKey = *(pStart - 1);
	unsigned char oldP = 0;
	unsigned char oldE = 0;

	for (int i = 0; i < _dataSize + 1; i++)
	{
		oldP = pStart[i] ^ (oldP + randKey + (i + 1));
		oldE = oldP ^ (oldE + staticKey + (i + 1));
		pStart[i] = oldE;
	}
	_isEncode = true;
}

bool Message::Decode(unsigned char randKey, int staticKey)
{
	//���ڵ� �Ǿ� ���� �ʴٸ� ���ڵ带 �õ����� �ʴ´�.
	if (!_isEncode)
		return false;

	//üũ���� ���̷ε� �κ��� ���ڵ� �Ѵ�.
	char* pStart = _pBegin - 1;				//üũ�� ��ġ���� ���ڵ� ����
	unsigned char prevOrigin = 0;
	unsigned char oldP = 0;
	unsigned char oldE = 0;

	for (int i = 0; i < _dataSize + 1; i++)
	{
		prevOrigin = pStart[i] ^ (oldE + staticKey + (i + 1)) ^ (oldP + randKey + (i + 1));
		oldP = prevOrigin ^ (oldP + randKey + (i + 1));
		oldE = pStart[i];
		pStart[i] = prevOrigin;
	}

	_isEncode = false;
	//���ڵ� �� �� üũ���� �ٽ� ���ؼ� ������ üũ���� ������ üũ �� �� ����� ��ȯ�Ѵ�.
	BYTE checkSum = GetCheckSum();
	return (BYTE)pStart[0] == checkSum;
}

Message* Message::Alloc()
{
	Message* pMessage = s_messagePool.Alloc();
	pMessage->Clear();				//���빰 ����
	return pMessage;
}

void Message::Free(Message* pMessage)
{
	s_messagePool.Free(pMessage);
}

int Message::GetMemoryPoolCapacity()
{
	return s_messagePool.GetCapacityCount();
}

int Message::GetMemoryPoolUseSize()
{
	return s_messagePool.GetUseCount();
}

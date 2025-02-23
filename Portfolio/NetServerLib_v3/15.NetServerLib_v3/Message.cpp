#include "Message.h"
#include "Windows.h"

TLSMemoryPool<Message> Message::s_messagePool(50, false);

Message::Message()
	:_bufferSize(BUFFER_DEFAULT_SIZE), _payloadSize(0), _refCount(0), _isEncode(false),
	_writePos(NET_HEADER_LEN), _readPos(NET_HEADER_LEN)
{
	_pBuffer = new char[_bufferSize + NET_HEADER_LEN];
}
Message::Message(int bufferSize)
	:_bufferSize(bufferSize), _payloadSize(0), _refCount(0), _isEncode(false),
	_writePos(NET_HEADER_LEN), _readPos(NET_HEADER_LEN)
{
	_pBuffer = new char[_bufferSize + NET_HEADER_LEN];
}

Message::~Message()
{
	delete[] _pBuffer;
}

void Message::Clear(void)
{
	_writePos = NET_HEADER_LEN;
	_readPos = NET_HEADER_LEN;
	_payloadSize = 0;
	_isEncode = false;
	_refCount = 0;
}

bool Message::Resize(int size)
{
	if (size <= _bufferSize || _bufferSize >= BUFFER_MAX_SIZE)
	{
		return false;
	}

	int prevTotalBufferSize = _bufferSize + NET_HEADER_LEN;

	if (size > BUFFER_MAX_SIZE)
	{
		_bufferSize = BUFFER_MAX_SIZE;
	}
	else
	{
		_bufferSize = size;
	}

	int totalBufferSize = _bufferSize + NET_HEADER_LEN;

	char* tempBuffer = new char[totalBufferSize];
	memcpy_s(tempBuffer, totalBufferSize, _pBuffer, prevTotalBufferSize);
	delete[] _pBuffer;
	_pBuffer = tempBuffer;

	//�α� �����
	SystemLog::GetInstance()->WriteLog(L"SYSTEM", SystemLog::LOG_LEVEL_SYSTEM, L"SerializeBuffer ReSize!! size : %d", size);

	return true;
}

//////////////////////////////////////////////////////////////////////////
	// ���� Pos �̵�. (�����̵��� �ȵ�)
	// GetPayloadPtr �Լ��� �̿��Ͽ� �ܺο��� ������ ���� ������ ������ ��� ���. 
	//
	// Parameters: (int) �̵� ������.
	// Return: (int) �̵��� ������.
	//////////////////////////////////////////////////////////////////////////
int	Message::MoveWritePos(int size)
{
	if (size < 0 || _writePos + size > _bufferSize)
		return 0;

	_writePos += size;
	_payloadSize += size;
	return size;
}
int	Message::MoveReadPos(int size)
{
	if (size < 0 || size > _payloadSize)
		return 0;

	_readPos += size;
	_payloadSize -= size;
	return size;
}


Message& Message::operator = (Message& SrMessage)
{
	_bufferSize = SrMessage._bufferSize;
	_payloadSize = SrMessage._payloadSize;
	delete[] _pBuffer;
	_pBuffer = new char[SrMessage._bufferSize];
	_writePos = SrMessage._writePos;
	_readPos = SrMessage._readPos;
	return *this;
}


LONG Message::SubRefCount()
{
	LONG ret = InterlockedDecrement(&_refCount);

	//_refCount�� 0�̶�� �޸� Ǯ�� ��ȯ�Ѵ�.
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

void Message::Encode(int staticKey)
{
	//üũ���� ���̷ε� �κ��� ���ڵ� �Ѵ�.
	char* pStart = GetPayloadPtr() - 1;		//üũ�� ��ġ���� ���ڵ� ����
	unsigned char randKey = *(pStart - 1);
	unsigned char oldP = 0;
	unsigned char oldE = 0;

	for (int i = 0; i < _payloadSize + 1; i++)
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
	{
		SystemLog::GetInstance()->WriteLog(L"ERROR", SystemLog::LOG_LEVEL_ERROR, L"already encode");
		return false;
	}

	//üũ���� ���̷ε� �κ��� ���ڵ� �Ѵ�.
	BYTE* pStart = reinterpret_cast<BYTE*>(GetPayloadPtr() - 1);		//üũ�� ��ġ���� ���ڵ� ����
	char prevOrigin = 0;
	char oldP = 0;
	char oldE = 0;
	INT64 checkSum = 0;

	for (int i = 0; i < _payloadSize + 1; i++)
	{
		prevOrigin = pStart[i] ^ (oldE + staticKey + (i + 1)) ^ (oldP + randKey + (i + 1));
		oldP = prevOrigin ^ (oldP + randKey + (i + 1));
		oldE = pStart[i];
		pStart[i] = prevOrigin;
		if (i != 0)
		{
			checkSum += pStart[i];
		}
	}
	checkSum %= 256;

	_isEncode = false;

	return pStart[0] == static_cast<BYTE>(checkSum);
}

unsigned char Message::GetCheckSum()
{
	INT64 sum = 0;
	char* pBegin = GetPayloadPtr();

	for (int i = 0; i < _payloadSize; i++)
	{
		sum += pBegin[i];
	}

	return sum % 256;
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

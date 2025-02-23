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

	//로깅 남기기
	SystemLog::GetInstance()->WriteLog(L"SYSTEM", SystemLog::LOG_LEVEL_SYSTEM, L"SerializeBuffer ReSize!! size : %d", size);

	return true;
}

//////////////////////////////////////////////////////////////////////////
	// 버퍼 Pos 이동. (음수이동은 안됨)
	// GetPayloadPtr 함수를 이용하여 외부에서 강제로 버퍼 내용을 수정할 경우 사용. 
	//
	// Parameters: (int) 이동 사이즈.
	// Return: (int) 이동된 사이즈.
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

	//_refCount가 0이라면 메모리 풀에 반환한다.
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
	//체크섬과 페이로드 부분을 인코딩 한다.
	char* pStart = GetPayloadPtr() - 1;		//체크섬 위치부터 인코딩 시작
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
	//인코드 되어 있지 않다면 디코드를 시도하지 않는다.
	if (!_isEncode)
	{
		SystemLog::GetInstance()->WriteLog(L"ERROR", SystemLog::LOG_LEVEL_ERROR, L"already encode");
		return false;
	}

	//체크섬과 페이로드 부분을 디코딩 한다.
	BYTE* pStart = reinterpret_cast<BYTE*>(GetPayloadPtr() - 1);		//체크섬 위치부터 인코딩 시작
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
	pMessage->Clear();				//내용물 비우기
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

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
	_pBegin = _pBuffer + HEADER_SIZE;	//실제 컨텐츠가 담기기 시작하는 지점
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
	_pBegin = _pBuffer + HEADER_SIZE;	//실제 컨텐츠가 담기기 시작하는 지점
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

	//로깅 남기기
	//SystemLog::GetInstance()->WriteLog(L"SYSTEM", SystemLog::LOG_LEVEL_SYSTEM, L"SerializeBuffer ReSize!! size : %d", size);

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

LONG Message::AddRefCount()
{
	return InterlockedIncrement(&_refCount);
}

LONG Message::SubRefCount()
{
	LONG ret = InterlockedDecrement(&_refCount);

	//_refCount가 0이라면 풀에 반환한다.
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

	//페이로드를 1byte씩 sum에 더한다.
	for (int i = 0; i < _dataSize; i++)
	{
		sum += (UINT64)_pBegin[i];
	}

	return sum % 256;
}

void Message::Encode(int staticKey)
{
	//체크섬과 페이로드 부분을 인코딩 한다.
	char* pStart = _pBegin - 1;		//체크섬 위치부터 인코딩 시작
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
	//인코드 되어 있지 않다면 디코드를 시도하지 않는다.
	if (!_isEncode)
		return false;

	//체크섬과 페이로드 부분을 디코딩 한다.
	char* pStart = _pBegin - 1;				//체크섬 위치부터 인코딩 시작
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
	//디코딩 한 후 체크섬을 다시 구해서 기존의 체크섬과 같은지 체크 한 후 결과를 반환한다.
	BYTE checkSum = GetCheckSum();
	return (BYTE)pStart[0] == checkSum;
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

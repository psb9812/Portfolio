#include "RingBuffer.h"

RingBuffer::RingBuffer()
	:_capacity(DEFAULT_CAPACITY), _size(0)
{
	_pBegin = new char[_capacity];
	_pEnd = _pBegin + _capacity;
	_pWritePos = _pBegin;
	_pReadPos = _pBegin;
}

RingBuffer::RingBuffer(int bufferSize)
	:_capacity(bufferSize), _size(0)
{
	_pBegin = new char[_capacity];
	_pEnd = _pBegin + _capacity;
	_pWritePos = _pBegin;
	_pReadPos = _pBegin;
}

RingBuffer::~RingBuffer()
{
	delete[] _pBegin;
}

void RingBuffer::Resize(int size)
{
	//지금 사용 중인 버퍼의 크기보다 작게 리사이즈는 허용하지 않음. (데이터 유실)
	if (size <= _size)
		return;

	char* tempBegin = new char[size];
	int directDequeueSize = DirectDequeueSize();
	if (_pReadPos + _size > _pEnd)	//끝단에서 데이터가 잘린 경우
	{
		int over = _size - directDequeueSize;
		memcpy_s(tempBegin, size, _pReadPos, directDequeueSize);
		memcpy_s(tempBegin + directDequeueSize, size, _pBegin, over);
	}
	else //한 방에 카피가 가능한 경우
	{
		memcpy_s(tempBegin, size, _pReadPos, _size);
	}

	delete[] _pBegin;
	_pBegin = tempBegin;
	_pEnd = _pBegin + size;
	_pWritePos = _pBegin + _size;
	_pReadPos = _pBegin;
	_capacity = size;
}

int	RingBuffer::GetBufferSize() const
{
	return _capacity;
}

int RingBuffer::GetUseSize() const
{
	return _size;
}

int	RingBuffer::GetFreeSize() const
{
	return _capacity - _size;
}

int RingBuffer::Enqueue(char* pSrc, int size)
{
	//남아있는 공간보다 큰 데이터를 넣으려는 경우 바로 차단한다.
	if (size > GetFreeSize())
		return 0;

	int distanceWToE = DirectEnqueueSize();

	if (size > distanceWToE) //링의 끝에 데이터가 들어가야 해서 중간에 잘린 경우
	{
		memcpy_s(_pWritePos, distanceWToE, pSrc, distanceWToE);
		_pWritePos = _pBegin;
		int remainingSize = size - distanceWToE;
		memcpy_s(_pWritePos, remainingSize, pSrc + distanceWToE, remainingSize);
		_pWritePos += remainingSize;
	}
	else //일반적인 Enqueue 상황
	{
		memcpy_s(_pWritePos, distanceWToE, pSrc, size);
		_pWritePos += size;
	}

	_size += size;
	return size;
}

int	RingBuffer::Dequeue(char* pDest, int size)
{
	//현재 가지고 있는 데이터보다 큰 크기의 데이터를 뽑을 수 없으니 차단한다.
	if (size > GetBufferSize())
		return 0;

	//끝과 현재 ReadPos 사이의 거리
	int distanceRToE = DirectDequeueSize();

	if (size > distanceRToE)	//링의 끝이어서 잘린 데이터를 읽어야 하는 경우
	{
		memcpy_s(pDest, size, _pReadPos, distanceRToE);
		_pReadPos = _pBegin;
		memcpy_s(pDest + distanceRToE, size - distanceRToE, _pReadPos, size - distanceRToE);
		_pReadPos += size - distanceRToE;
	}
	else    //일반적인 경우
	{
		memcpy_s(pDest, size, _pReadPos, size);
		_pReadPos += size;
	}

	_size -= size;
	return size;
}

int	RingBuffer::Peek(char* pDest, int size)	const
{
	if (size > GetBufferSize())
		return 0;

	if (size > DirectDequeueSize())	//링의 끝이어서 잘린 데이터를 읽어야 하는 경우
	{
		memcpy_s(pDest, size, _pReadPos, DirectDequeueSize());
		memcpy_s(pDest + DirectDequeueSize(), size - DirectDequeueSize(), _pBegin, size - DirectDequeueSize());
	}
	else    //일반적인 경우
	{
		memcpy_s(pDest, size, _pReadPos, size);
	}
	return size;
}

void RingBuffer::ClearBuffer(void)
{
	_pWritePos = _pBegin;
	_pReadPos = _pBegin;
	_size = 0;
}

int	RingBuffer::DirectEnqueueSize(void) const
{
	return static_cast<int>(_pEnd - _pWritePos);
}
int	RingBuffer::DirectDequeueSize(void) const
{
	return static_cast<int>(_pEnd - _pReadPos);
}

int	RingBuffer::MoveWrite(int size)
{
	_pWritePos += size;
	if (_pWritePos > _pEnd - 1)
	{
		int over = _pWritePos - _pEnd;
		_pWritePos = _pBegin + over;
	}
	_size += size;
	return size;
}
int	RingBuffer::MoveRead(int size)
{
	if (size > _size)
		return 0;

	_pReadPos += size;
	if (_pReadPos > _pEnd - 1)
	{
		int over = static_cast<int>(_pReadPos - _pEnd);
		_pReadPos = _pBegin + over;
	}
	_size -= size;
	return size;
}

char* RingBuffer::GetReadBufferPtr(void) const
{
	return _pReadPos;
}


char* RingBuffer::GetWriteBufferPtr(void) const
{
	return _pWritePos;
}
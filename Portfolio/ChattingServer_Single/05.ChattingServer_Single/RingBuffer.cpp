#include "RingBuffer.h"

RingBuffer::RingBuffer()
	:_capacity(DEFAULT_CAPACITY + 1)
{
	_pBegin = new char[_capacity];
	_pEnd = _pBegin + _capacity;
	_rear = _front = 0;
	InitializeSRWLock(&_lock);
}

RingBuffer::RingBuffer(int bufferSize)
	:_capacity(bufferSize + 1)
{
	_pBegin = new char[_capacity];
	_pEnd = _pBegin + _capacity;
	_rear = _front = 0;
	InitializeSRWLock(&_lock);
}

RingBuffer::~RingBuffer()
{
	delete[] _pBegin;
}

void RingBuffer::Resize(int size)
{
	int useSize = GetUseSize();
	//지금 사용 중인 버퍼의 크기보다 작게 리사이즈는 허용하지 않음. (데이터 유실)
	if (size <= useSize)
		return;

	char* tempBegin = new char[size];
	int directDequeueSize = DirectDequeueSize();
	if ((_pBegin + _front) + useSize > _pEnd)	//끝단에서 데이터가 잘린 경우
	{
		int over = useSize - directDequeueSize;
		memcpy_s(tempBegin, size, _pBegin + _front, directDequeueSize);
		memcpy_s(tempBegin + directDequeueSize, size, _pBegin, over);
	}
	else //한 방에 카피가 가능한 경우
	{
		memcpy_s(tempBegin, size, _pBegin + _front, useSize);
	}

	delete[] _pBegin;
	_pBegin = tempBegin;
	_pEnd = _pBegin + size;
	_rear = useSize;
	_front = 0;
	_capacity = size;
}

int	RingBuffer::GetBufferSize() const
{
	return _capacity - 1;
}

int RingBuffer::GetUseSize() const
{
	int tempRear = _rear;
	int tempFront = _front;

	return (tempRear >= tempFront) ? tempRear - tempFront : tempRear + (_capacity - tempFront);
}

int	RingBuffer::GetFreeSize() const
{
	return GetBufferSize() - GetUseSize();
}

int	RingBuffer::DirectEnqueueSize()
{
	int directEnqueueSize;
	//로직 중간에 값이 바뀔 수 있으니 지역변수에 받아놓고 그 값으로 로직을 돌린다.
	int tempRear = _rear;
	int tempFront = _front;


	if (tempRear >= tempFront)  //순서대로
		directEnqueueSize = _capacity - tempRear - (tempFront == 0 ? 1 : 0); //맨 앞에 ReadPos가 있으면 맨 뒤 한 칸을 못 쓴다.
	else  //역순
		directEnqueueSize = tempFront - tempRear - 1;

	return directEnqueueSize;

}

int	RingBuffer::DirectDequeueSize()
{
	int directDnqueueSize;
	int tempRear = _rear;
	int tempFront = _front;

	if (tempRear >= tempFront)	//순서일 때
		directDnqueueSize = tempRear - tempFront;
	else    //역순
		directDnqueueSize = _capacity - tempFront;

	return directDnqueueSize;
}

int RingBuffer::Enqueue(char* pSrc, int enqueueSize)
{
	//남아있는 공간보다 큰 데이터를 넣으려는 경우 바로 차단한다.
	int freeSize = GetFreeSize();
	if (enqueueSize > freeSize)
	{
		return 0;
	}

	int directEnqueueSize = DirectEnqueueSize();
	int spareSize = enqueueSize - directEnqueueSize;

	if (enqueueSize > directEnqueueSize) //링의 끝에 데이터가 들어가야 해서 중간에 잘린 경우
	{
		memcpy_s(_pBegin + _rear, directEnqueueSize, pSrc, directEnqueueSize);

		memcpy_s(_pBegin, spareSize, pSrc + directEnqueueSize, spareSize);

	}
	else //일반적인 Enqueue 상황
	{
		memcpy_s(_pBegin + _rear, directEnqueueSize, pSrc, enqueueSize);
	}

	_rear = (_rear + enqueueSize) % _capacity;

	return enqueueSize;
}

int	RingBuffer::Dequeue(char* pDest, int dequeueSize)
{
	int useSize = GetUseSize();
	//현재 가지고 있는 데이터보다 큰 크기의 데이터를 뽑을 수 없으니 차단한다.
	if (dequeueSize > useSize)
	{
		return 0;
	}

	//끝과 현재 ReadPos 사이의 거리
	int directDequeueSize = DirectDequeueSize();
	int spareSize = dequeueSize - directDequeueSize;

	if (dequeueSize > directDequeueSize)	//링의 끝이어서 잘린 데이터를 읽어야 하는 경우
	{
		memcpy_s(pDest, dequeueSize, _pBegin + _front, directDequeueSize);

		memcpy_s(pDest + directDequeueSize, spareSize, _pBegin, spareSize);
	}
	else    //일반적인 경우
	{
		memcpy_s(pDest, dequeueSize, _pBegin + _front, dequeueSize);
	}

	_front = (_front + dequeueSize) % _capacity;
	return dequeueSize;
}

int	RingBuffer::Peek(char* pDest, int peekSize)
{
	int useSize = GetUseSize();
	if (peekSize > useSize)
		return 0;

	int directDequeueSize = DirectDequeueSize();
	int spareSize = peekSize - directDequeueSize;
	if (peekSize >= directDequeueSize)	//링의 끝이어서 잘린 데이터를 읽어야 하는 경우
	{
		memcpy_s(pDest, peekSize, _pBegin + _front, directDequeueSize);
		memcpy_s(pDest + directDequeueSize, spareSize, _pBegin, spareSize);
	}
	else    //일반적인 경우
	{
		memcpy_s(pDest, peekSize, _pBegin + _front, peekSize);
	}
	return peekSize;
}

int RingBuffer::PeekAt(char* pDest, int readOffset, int peekSize)
{
	int useSize = GetUseSize();
	int readAmount = useSize - readOffset;
	if (peekSize  > readAmount || readOffset - _front > useSize)
		return 0;

	int peekFront = (_front + readOffset) %_capacity;
	int directPeekAtSize = _capacity - peekFront;
	int spareSize = peekSize - directPeekAtSize;

	if (directPeekAtSize < peekSize)	//링의 끝이어서 잘린 데이터를 읽어야 하는 경우
	{
		memcpy_s(pDest, peekSize, _pBegin + peekFront, directPeekAtSize);
		memcpy_s(pDest + directPeekAtSize, spareSize, _pBegin, spareSize);
		return directPeekAtSize + spareSize;
	}
	else    //일반적인 경우
	{
		memcpy_s(pDest, peekSize, _pBegin + peekFront, peekSize);
		return peekSize;
	}
}

void RingBuffer::ClearBuffer()
{
	_rear = 0;
	_front = 0;
	//resize의 경우 사이즈 정상으로 돌림
}

int	RingBuffer::MoveRear(int moveSize)
{
	//공간 초과 방지
	int freeSize = GetFreeSize();
	if (freeSize < moveSize)
		return 0;

	_rear = (_rear + moveSize) % _capacity;
	return moveSize;
}

int	RingBuffer::MoveFront(int moveSize)
{
	//UseSize보다 큰 값을 읽으려는 시도를 차단한다.
	int useSize = GetUseSize();
	if (moveSize > useSize)
		return 0;

	_front = (_front + moveSize) % _capacity;
	return moveSize;
}

char* RingBuffer::GetBeginBufferPtr() const
{
	return _pBegin;
}

char* RingBuffer::GetFrontBufferPtr() const
{
	return _pBegin + _front;
}


char* RingBuffer::GetRearBufferPtr() const
{
	return _pBegin + _rear;
}

void RingBuffer::SharedLock()
{
	AcquireSRWLockShared(&_lock);
}

void RingBuffer::SharedUnlock()
{
	ReleaseSRWLockShared(&_lock);
}

void RingBuffer::ExclusiveLock()
{
	AcquireSRWLockExclusive(&_lock);
}

void RingBuffer::ExclusiveUnlock()
{
	ReleaseSRWLockExclusive(&_lock);
}
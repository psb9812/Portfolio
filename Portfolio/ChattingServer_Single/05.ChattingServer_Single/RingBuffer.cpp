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
	//���� ��� ���� ������ ũ�⺸�� �۰� ��������� ������� ����. (������ ����)
	if (size <= useSize)
		return;

	char* tempBegin = new char[size];
	int directDequeueSize = DirectDequeueSize();
	if ((_pBegin + _front) + useSize > _pEnd)	//���ܿ��� �����Ͱ� �߸� ���
	{
		int over = useSize - directDequeueSize;
		memcpy_s(tempBegin, size, _pBegin + _front, directDequeueSize);
		memcpy_s(tempBegin + directDequeueSize, size, _pBegin, over);
	}
	else //�� �濡 ī�ǰ� ������ ���
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
	//���� �߰��� ���� �ٲ� �� ������ ���������� �޾Ƴ��� �� ������ ������ ������.
	int tempRear = _rear;
	int tempFront = _front;


	if (tempRear >= tempFront)  //�������
		directEnqueueSize = _capacity - tempRear - (tempFront == 0 ? 1 : 0); //�� �տ� ReadPos�� ������ �� �� �� ĭ�� �� ����.
	else  //����
		directEnqueueSize = tempFront - tempRear - 1;

	return directEnqueueSize;

}

int	RingBuffer::DirectDequeueSize()
{
	int directDnqueueSize;
	int tempRear = _rear;
	int tempFront = _front;

	if (tempRear >= tempFront)	//������ ��
		directDnqueueSize = tempRear - tempFront;
	else    //����
		directDnqueueSize = _capacity - tempFront;

	return directDnqueueSize;
}

int RingBuffer::Enqueue(char* pSrc, int enqueueSize)
{
	//�����ִ� �������� ū �����͸� �������� ��� �ٷ� �����Ѵ�.
	int freeSize = GetFreeSize();
	if (enqueueSize > freeSize)
	{
		return 0;
	}

	int directEnqueueSize = DirectEnqueueSize();
	int spareSize = enqueueSize - directEnqueueSize;

	if (enqueueSize > directEnqueueSize) //���� ���� �����Ͱ� ���� �ؼ� �߰��� �߸� ���
	{
		memcpy_s(_pBegin + _rear, directEnqueueSize, pSrc, directEnqueueSize);

		memcpy_s(_pBegin, spareSize, pSrc + directEnqueueSize, spareSize);

	}
	else //�Ϲ����� Enqueue ��Ȳ
	{
		memcpy_s(_pBegin + _rear, directEnqueueSize, pSrc, enqueueSize);
	}

	_rear = (_rear + enqueueSize) % _capacity;

	return enqueueSize;
}

int	RingBuffer::Dequeue(char* pDest, int dequeueSize)
{
	int useSize = GetUseSize();
	//���� ������ �ִ� �����ͺ��� ū ũ���� �����͸� ���� �� ������ �����Ѵ�.
	if (dequeueSize > useSize)
	{
		return 0;
	}

	//���� ���� ReadPos ������ �Ÿ�
	int directDequeueSize = DirectDequeueSize();
	int spareSize = dequeueSize - directDequeueSize;

	if (dequeueSize > directDequeueSize)	//���� ���̾ �߸� �����͸� �о�� �ϴ� ���
	{
		memcpy_s(pDest, dequeueSize, _pBegin + _front, directDequeueSize);

		memcpy_s(pDest + directDequeueSize, spareSize, _pBegin, spareSize);
	}
	else    //�Ϲ����� ���
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
	if (peekSize >= directDequeueSize)	//���� ���̾ �߸� �����͸� �о�� �ϴ� ���
	{
		memcpy_s(pDest, peekSize, _pBegin + _front, directDequeueSize);
		memcpy_s(pDest + directDequeueSize, spareSize, _pBegin, spareSize);
	}
	else    //�Ϲ����� ���
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

	if (directPeekAtSize < peekSize)	//���� ���̾ �߸� �����͸� �о�� �ϴ� ���
	{
		memcpy_s(pDest, peekSize, _pBegin + peekFront, directPeekAtSize);
		memcpy_s(pDest + directPeekAtSize, spareSize, _pBegin, spareSize);
		return directPeekAtSize + spareSize;
	}
	else    //�Ϲ����� ���
	{
		memcpy_s(pDest, peekSize, _pBegin + peekFront, peekSize);
		return peekSize;
	}
}

void RingBuffer::ClearBuffer()
{
	_rear = 0;
	_front = 0;
	//resize�� ��� ������ �������� ����
}

int	RingBuffer::MoveRear(int moveSize)
{
	//���� �ʰ� ����
	int freeSize = GetFreeSize();
	if (freeSize < moveSize)
		return 0;

	_rear = (_rear + moveSize) % _capacity;
	return moveSize;
}

int	RingBuffer::MoveFront(int moveSize)
{
	//UseSize���� ū ���� �������� �õ��� �����Ѵ�.
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
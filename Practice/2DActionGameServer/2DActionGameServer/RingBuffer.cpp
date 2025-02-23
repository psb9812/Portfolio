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
	//���� ��� ���� ������ ũ�⺸�� �۰� ��������� ������� ����. (������ ����)
	if (size <= _size)
		return;

	char* tempBegin = new char[size];
	int directDequeueSize = DirectDequeueSize();
	if (_pReadPos + _size > _pEnd)	//���ܿ��� �����Ͱ� �߸� ���
	{
		int over = _size - directDequeueSize;
		memcpy_s(tempBegin, size, _pReadPos, directDequeueSize);
		memcpy_s(tempBegin + directDequeueSize, size - directDequeueSize, _pBegin, over);
	}
	else //�� �濡 ī�ǰ� ������ ���
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
	//�����ִ� �������� ū �����͸� �������� ��� �ٷ� �����Ѵ�.
	if (size > GetFreeSize())
		return 0;

	int distanceWToE = DirectEnqueueSize();

	if (size >= distanceWToE) //���� ���� �����Ͱ� ���� �ؼ� �߰��� �߸� ���
	{
		memcpy_s(_pWritePos, distanceWToE, pSrc, distanceWToE);
		_pWritePos = _pBegin;

		memcpy_s(_pWritePos, _pReadPos - _pWritePos, pSrc + distanceWToE, size - distanceWToE);
		_pWritePos += size - distanceWToE;
	}
	else //�Ϲ����� Enqueue ��Ȳ
	{
		memcpy_s(_pWritePos, distanceWToE, pSrc, size);
		_pWritePos += size;
	}

	_size += size;
	return size;
}

int	RingBuffer::Dequeue(char* pDest, int size)
{
	//���� ������ �ִ� �����ͺ��� ū ũ���� �����͸� ���� �� ������ �����Ѵ�.
	if (size > _size)
		return 0;

	//���� ���� ReadPos ������ �Ÿ�
	int distanceRToE = DirectDequeueSize();

	if (size >= distanceRToE)	//���� ���̾ �߸� �����͸� �о�� �ϴ� ���
	{
		memcpy_s(pDest, size, _pReadPos, distanceRToE);
		_pReadPos = _pBegin;
		memcpy_s(pDest + distanceRToE, size - distanceRToE, _pReadPos, size - distanceRToE);
		_pReadPos += size - distanceRToE;
	}
	else    //�Ϲ����� ���
	{
		memcpy_s(pDest, size, _pReadPos, size);
		_pReadPos += size;
	}

	_size -= size;
	return size;
}

int	RingBuffer::Peek(char* pDest, int size)	const
{
	if (size > _size)
		return 0;

	int directDequeueSize = DirectDequeueSize();
	if (size > directDequeueSize)	//���� ���̾ �߸� �����͸� �о�� �ϴ� ���
	{
		memcpy_s(pDest, size, _pReadPos, directDequeueSize);
		memcpy_s(pDest + directDequeueSize, size - directDequeueSize, _pBegin, size - directDequeueSize);
	}
	else    //�Ϲ����� ���
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
	return _pEnd - _pWritePos;
}
int	RingBuffer::DirectDequeueSize(void) const
{
	return _pEnd - _pReadPos;
}

int	RingBuffer::MoveRear(int size)
{
	//���� �ʰ� ����
	if (GetFreeSize() < size)
		return -1;

	_pWritePos += size;
	if (_pWritePos >= _pEnd)
	{
		_pWritePos -= _capacity;
	}
	_size += size;
	return size;
}
int	RingBuffer::MoveFront(int size)
{
	//UseSize���� ū ���� �������� �õ��� �����Ѵ�.
	if (size > _size)
		return -1;

	_pReadPos += size;
	if (_pReadPos >= _pEnd)
	{
		_pReadPos -= _capacity;
	}
	_size -= size;
	return size;
}

char* RingBuffer::GetFrontBufferPtr(void) const
{
	return _pReadPos;
}


char* RingBuffer::GetRearBufferPtr(void) const
{
	return _pWritePos;
}

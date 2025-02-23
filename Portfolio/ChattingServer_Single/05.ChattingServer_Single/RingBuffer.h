#pragma once
/*
* date : 2024.08.23 08:52
* writer : �ڼ���
*
* - ��Ʈ��ũ �ۼ��� �� ���۸��� �ϱ� ���� �� ����(���� ť) �ڷᱸ���Դϴ�.
* - front�� rear�� Enque/Deque �۾��� �� �� ���� �ε����� �̵��ϴ� ���
	�� ���� front�� rear�� ����Ű�� ���� �۾��� �ؾ� �� ���̴�.
* - SRWLock ��� �߰�
* # �� ���� ��ť �� ���� ��ť�ϴ� �� ������ ���̿��� �� ���� �� �۵��ϰ� ����
* #
*/
#include <memory>
#include <Windows.h>


class RingBuffer
{
	const int DEFAULT_CAPACITY = 500;
private:
	char* _pBegin;
	int		_front;
	int		_rear;
	char* _pEnd;
	int		_capacity;

	SRWLOCK _lock;

public:
	RingBuffer();
	RingBuffer(int bufferSize);
	~RingBuffer();

	//���� ũ�� ����
	void Resize(int size);

	//������ ��ü ������ ���ϱ�
	int	GetBufferSize() const;
	//���� ��� ���� ũ�� ���
	__inline int GetUseSize() const;
	//���� ������ ���� ���� ũ�� ���
	int	GetFreeSize() const;

	// WritePos �� ������ ����.
	// Parameters: (char *)������ ������. (int)ũ��. 
	// Return: (int)���� ũ��.
	int Enqueue(char* pSrc, int size);

	// ReadPos ���� ����Ÿ ������. ReadPos �̵�.
	// Parameters: (char *)������ ������. (int)ũ��.
	// Return: (int)������ ũ��.
	int	Dequeue(char* pDest, int size);

	// ReadPos ���� ����Ÿ �о��. ReadPos ����.
	// Parameters: (char *)������ ������. (int)ũ��.
	// Return: (int)������ ũ��.
	int	Peek(char* pDest, int size);

	// ReadPos ���� ����Ÿ �о��. ReadPos ���� ����.
	// Parameters: (char *)������ ������. (int)�б� ������������ ������ (int)ũ��.
	// Return: (int)������ ũ��.
	int PeekAt(char* pDest, int readOffset, int peekSize);

	// ������ ��� ����Ÿ ����.
	void ClearBuffer();

	// ���� �����ͷ� �ܺο��� �ѹ濡 �а�, �� �� �ִ� ����.
	// (������ ���� ����)
	//
	// ���� ť�� ������ ������ ���ܿ� �ִ� �����ʹ� �� -> ó������ ���ư���
	// 2���� �����͸� ��ų� ���� �� ����. �� �κп��� �������� ���� ���̸� �ǹ�
	//
	// Parameters: ����.
	// Return: (int)��밡�� �뷮.
	int	DirectEnqueueSize();
	int	DirectDequeueSize();

	// ���ϴ� ���̸�ŭ �б���ġ ���� ���� / ���� ��ġ �̵�
	int	MoveRear(int iSize);
	int	MoveFront(int iSize);

	char* GetBeginBufferPtr() const;

	// ������ Front ������ ����.
	//
	// Return: (char *) ���� ������.
	char* GetFrontBufferPtr() const;


	// ������ RearPos ������ ����.
	//
	// Return: (char *) ���� ������.
	char* GetRearBufferPtr() const;

	//�� ���� �Լ�
	void SharedLock();
	void SharedUnlock();
	void ExclusiveLock();
	void ExclusiveUnlock();
};
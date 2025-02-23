/*
* date : 2024.05.30 9:44
* writer : �ڼ���
*
* ��Ʈ��ũ �ۼ��� �� ���۸��� �ϱ� ���� �� ����(���� ť) �ڷᱸ���Դϴ�.
*/
#pragma once
#include <memory>

class RingBuffer
{
	const int DEFAULT_CAPACITY = 10000;
private:
	char* _pBegin;
	char* _pEnd;
	char* _pWritePos;
	char* _pReadPos;
	int _capacity;
	int _size;

public:
	RingBuffer();
	RingBuffer(int bufferSize);
	~RingBuffer();

	//���� ũ�� ����
	void Resize(int size);

	int	GetBufferSize() const;
	//���� ��� ���� �뷮 ũ�� ���
	int GetUseSize() const;
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
	int	Peek(char* pDest, int size) const;

	// ������ ��� ����Ÿ ����.
	// Parameters: ����.
	// Return: ����.
	void ClearBuffer(void);

	// ���� �����ͷ� �ܺο��� �ѹ濡 �а�, �� �� �ִ� ����.
	// (������ ���� ����)
	//
	// ���� ť�� ������ ������ ���ܿ� �ִ� �����ʹ� �� -> ó������ ���ư���
	// 2���� �����͸� ��ų� ���� �� ����. �� �κп��� �������� ���� ���̸� �ǹ�
	//
	// Parameters: ����.
	// Return: (int)��밡�� �뷮.
	int	DirectEnqueueSize(void) const;
	int	DirectDequeueSize(void) const;

	// ���ϴ� ���̸�ŭ �б���ġ ���� ���� / ���� ��ġ �̵�
	//
	// Parameters: ����.
	// Return: (int)�̵�ũ��
	int	MoveWrite(int iSize);
	int	MoveRead(int iSize);

	// ������ Front ������ ����.
	//
	// Parameters: ����.
	// Return: (char *) ���� ������.
	char* GetReadBufferPtr(void) const;


	// ������ RearPos ������ ����.
	//
	// Parameters: ����.
	// Return: (char *) ���� ������.
	char* GetWriteBufferPtr(void) const;
};
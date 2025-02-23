/*
* date : 2024.05.30 9:44
* writer : 박성빈
*
* 네트워크 송수신 시 버퍼링을 하기 위한 링 버퍼(원형 큐) 자료구조입니다.
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

	//버퍼 크기 조절
	void Resize(int size);

	int	GetBufferSize() const;
	//현재 사용 중인 용량 크기 얻기
	int GetUseSize() const;
	//현재 버퍼의 남은 공간 크기 얻기
	int	GetFreeSize() const;

	// WritePos 에 데이터 넣음.
	// Parameters: (char *)데이터 포인터. (int)크기. 
	// Return: (int)넣은 크기.
	int Enqueue(char* pSrc, int size);

	// ReadPos 에서 데이타 가져옴. ReadPos 이동.
	// Parameters: (char *)데이터 포인터. (int)크기.
	// Return: (int)가져온 크기.
	int	Dequeue(char* pDest, int size);

	// ReadPos 에서 데이타 읽어옴. ReadPos 고정.
	// Parameters: (char *)데이터 포인터. (int)크기.
	// Return: (int)가져온 크기.
	int	Peek(char* pDest, int size) const;

	// 버퍼의 모든 데이타 삭제.
	// Parameters: 없음.
	// Return: 없음.
	void ClearBuffer(void);

	// 버퍼 포인터로 외부에서 한방에 읽고, 쓸 수 있는 길이.
	// (끊기지 않은 길이)
	//
	// 원형 큐의 구조상 버퍼의 끝단에 있는 데이터는 끝 -> 처음으로 돌아가서
	// 2번에 데이터를 얻거나 넣을 수 있음. 이 부분에서 끊어지지 않은 길이를 의미
	//
	// Parameters: 없음.
	// Return: (int)사용가능 용량.
	int	DirectEnqueueSize(void) const;
	int	DirectDequeueSize(void) const;

	// 원하는 길이만큼 읽기위치 에서 삭제 / 쓰기 위치 이동
	//
	// Parameters: 없음.
	// Return: (int)이동크기
	int	MoveWrite(int iSize);
	int	MoveRead(int iSize);

	// 버퍼의 Front 포인터 얻음.
	//
	// Parameters: 없음.
	// Return: (char *) 버퍼 포인터.
	char* GetReadBufferPtr(void) const;


	// 버퍼의 RearPos 포인터 얻음.
	//
	// Parameters: 없음.
	// Return: (char *) 버퍼 포인터.
	char* GetWriteBufferPtr(void) const;
};
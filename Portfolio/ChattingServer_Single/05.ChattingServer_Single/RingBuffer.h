#pragma once
/*
* date : 2024.08.23 08:52
* writer : 박성빈
*
* - 네트워크 송수신 시 버퍼링을 하기 위한 링 버퍼(원형 큐) 자료구조입니다.
* - front와 rear는 Enque/Deque 작업을 한 후 다음 인덱스로 이동하는 방식
	즉 현재 front와 rear가 가리키는 곳은 작업을 해야 할 곳이다.
* - SRWLock 기능 추가
* # 한 쪽은 인큐 한 쪽은 디큐하는 두 스레드 사이에서 락 없이 잘 작동하게 구현
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

	//버퍼 크기 조절
	void Resize(int size);

	//버퍼의 전체 사이즈 구하기
	int	GetBufferSize() const;
	//현재 사용 중인 크기 얻기
	__inline int GetUseSize() const;
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
	int	Peek(char* pDest, int size);

	// ReadPos 에서 데이타 읽어옴. ReadPos 지정 가능.
	// Parameters: (char *)데이터 포인터. (int)읽기 지점에서부터 오프셋 (int)크기.
	// Return: (int)가져온 크기.
	int PeekAt(char* pDest, int readOffset, int peekSize);

	// 버퍼의 모든 데이타 삭제.
	void ClearBuffer();

	// 버퍼 포인터로 외부에서 한방에 읽고, 쓸 수 있는 길이.
	// (끊기지 않은 길이)
	//
	// 원형 큐의 구조상 버퍼의 끝단에 있는 데이터는 끝 -> 처음으로 돌아가서
	// 2번에 데이터를 얻거나 넣을 수 있음. 이 부분에서 끊어지지 않은 길이를 의미
	//
	// Parameters: 없음.
	// Return: (int)사용가능 용량.
	int	DirectEnqueueSize();
	int	DirectDequeueSize();

	// 원하는 길이만큼 읽기위치 에서 삭제 / 쓰기 위치 이동
	int	MoveRear(int iSize);
	int	MoveFront(int iSize);

	char* GetBeginBufferPtr() const;

	// 버퍼의 Front 포인터 얻음.
	//
	// Return: (char *) 버퍼 포인터.
	char* GetFrontBufferPtr() const;


	// 버퍼의 RearPos 포인터 얻음.
	//
	// Return: (char *) 버퍼 포인터.
	char* GetRearBufferPtr() const;

	//락 관련 함수
	void SharedLock();
	void SharedUnlock();
	void ExclusiveLock();
	void ExclusiveUnlock();
};
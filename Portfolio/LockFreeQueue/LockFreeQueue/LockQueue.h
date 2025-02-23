#pragma once
#include <Windows.h>
#include <optional>
#include "MemoryPool.h"

template<typename T>
class LockQueue
{
public:
	struct Node
	{
		T _data;
		Node* _pNext;

		Node()
			:_data(0), _pNext(nullptr) {}
		Node(T data, Node* pNext)
			:_data(data), _pNext(pNext) {}
	};

	LockQueue();
	~LockQueue();
public:
	void Enqueue(T data);
	std::optional<T> Dequeue();
	int GetSize();
private:
	void SpinLock();
	void SpinUnlock();
	void SRWLock();
	void SRWUnlock();
	void MutexLock();
	void MutexUnlock();
private:
	Node* _pHead;
	Node* _pTail;
	LONG _lockFlag;
	int _size;
	
	SRWLOCK _srwLock;
	HANDLE _mutex;
	//MemoryPool<Node> _nodePool;
	TLSMemoryPool<Node> _nodePool;
};

template<typename T>
inline LockQueue<T>::LockQueue()
	:_pHead(nullptr), _pTail(nullptr), _lockFlag(0), _size(0), _nodePool(1000, false)
{
	InitializeSRWLock(&_srwLock);
	_mutex = CreateMutex(NULL, FALSE, NULL);
}

template<typename T>
inline LockQueue<T>::~LockQueue()
{
	CloseHandle(_mutex);
}

template<typename T>
inline void LockQueue<T>::Enqueue(T data)
{
	Node* pNewNode = _nodePool.Alloc();
	//SpinLock();
	SRWLock();
	//MutexLock();
	pNewNode->_data = data;
	pNewNode->_pNext = nullptr;

	// 큐가 빈 경우
	if (_pTail == nullptr)
	{
		_pHead = pNewNode;
		_pTail = pNewNode;
	}
	else
	{
		_pTail->_pNext = pNewNode;
		_pTail = pNewNode;
	}
	_size++;

	//SpinUnlock();
	SRWUnlock();
	//MutexUnlock();
}

template<typename T>
inline std::optional<T> LockQueue<T>::Dequeue()
{
	//SpinLock();
	SRWLock();
	//MutexLock();
	
	Node* pDelNode = nullptr;
	T retData;
	// 큐가 빈 경우
	if (_pHead == nullptr)
	{
		//SpinUnlock();
		SRWUnlock();
		//MutexUnlock();
		return std::nullopt;
	}
	else
	{
		pDelNode = _pHead;
		retData = _pHead->_data;
		_pHead = _pHead->_pNext;
		//빈 경우 두 포인터 다 nullptr
		if (_pHead == nullptr)
		{
			_pTail = nullptr;
		}
	}
	_size--;
	//SpinUnlock();
	SRWUnlock();
	//MutexUnlock();
	_nodePool.Free(pDelNode);
	return retData;
}

template<typename T>
inline int LockQueue<T>::GetSize()
{
	return _size;
}

template<typename T>
inline void LockQueue<T>::SpinLock()
{
	while (InterlockedExchange(&_lockFlag, 1) == 1)
	{
		YieldProcessor();
	}
}

template<typename T>
inline void LockQueue<T>::SpinUnlock()
{
	InterlockedExchange(&_lockFlag, 0);
}

template<typename T>
inline void LockQueue<T>::SRWLock()
{
	AcquireSRWLockExclusive(&_srwLock);
}

template<typename T>
inline void LockQueue<T>::SRWUnlock()
{
	ReleaseSRWLockExclusive(&_srwLock);
}

template<typename T>
inline void LockQueue<T>::MutexLock()
{
	WaitForSingleObject(_mutex, INFINITE);
}

template<typename T>
inline void LockQueue<T>::MutexUnlock()
{
	ReleaseMutex(_mutex);
}

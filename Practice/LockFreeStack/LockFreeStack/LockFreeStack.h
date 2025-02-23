#pragma once
#include <Windows.h>
#include <iostream>
#include "MemoryLog.h"
#include "LockFreeMemoryPool.h"
#include "AddressConverter.h"

template<typename T>
class LockFreeStack
{
public:
	struct Node
	{
		T data;
		Node* pNext;

		Node()
			:data(0), pNext(nullptr) {}
		Node(T value, Node* next)
			:data(value), pNext(next) {}
	};

public:
	LockFreeStack();
	~LockFreeStack();

	void Push(T data);
	T Pop();

private:
	LockFreeMemoryPool<Node> _memoryPool;
	MemoryLog memoryLog;
	alignas(64) UINT_PTR _pTop;
	alignas(64) LONG _size;
	alignas(64) UINT64 _addrIDCount;

};

template<typename T>
inline LockFreeStack<T>::LockFreeStack()
	:_pTop(NULL), _size(0), _memoryPool(0, true), _addrIDCount(0)
{
}

template<typename T>
inline LockFreeStack<T>::~LockFreeStack()
{
}

template<typename T>
inline void LockFreeStack<T>::Push(T data)
{
	//New 노드 할당
	// Pop에서 메모리를 해제 했는데, 
	// 지역에 저장해둔 해제된 메모리에 접근하는문제를 해결하기 위해 메모리 풀에서 할당 받아서 사용한다.
	Node* pNewNode = _memoryPool.Alloc(data, nullptr);

	while (true)
	{
		//지역 변수에 현재 Top 노드 주소를 저장
		UINT_PTR pLocalTopMetaAddr = _pTop;

		//새로운 노드의 next에 지역 변수에 저장한 Top노드의 주소를 저장
		pNewNode->pNext = reinterpret_cast<Node*>(pLocalTopMetaAddr);

		//addressID를 발급 받아서 NewNode의 메모리 주소 상위 16비트에 그 값을 저장 (ABA 문제 방지)
		UINT64 addrID = AddressConverter::GetAddressID(&_addrIDCount);
		UINT_PTR pNewNodeMetaAddr = AddressConverter::InsertMetaIDToAddress((UINT_PTR)pNewNode, addrID);

		//현재 Top노드가 지역 변수에 저장한 Top노드와 여전히 같다면 새로운 노드를 스택의 Top으로 저장
		if ((UINT_PTR)InterlockedCompareExchange64((__int64*)&_pTop, pNewNodeMetaAddr, pLocalTopMetaAddr) == pLocalTopMetaAddr)
		{
			LONG localSize = InterlockedIncrement(&_size);

			//로그찍기
			//UINT64 ret = memoryLog.WriteMemoryLog(PUSH, newNode, newNode->pNext, pLocalTop, 
				//(pLocalTop == nullptr) ? nullptr : pLocalTop->pNext, nullptr, localSize);

			break;
		}
	}
}

template<typename T>
inline T LockFreeStack<T>::Pop()
{
	Node* pRealTopMetaAddr = nullptr;

	if (_pTop == NULL)
		__debugbreak();

	while (true)
	{
		UINT_PTR pLocalTopMetaAddr = _pTop;
		Node* pLocalTopRealAddr = reinterpret_cast<Node*>(AddressConverter::DeleteMetaIDToAddress(pLocalTopMetaAddr));
		UINT_PTR pNewTopMetaAddr = reinterpret_cast<UINT_PTR>(pLocalTopRealAddr->pNext);

		if ((pRealTopMetaAddr = (Node*)InterlockedCompareExchange64((__int64*) & _pTop, pNewTopMetaAddr, pLocalTopMetaAddr)) == (Node*)pLocalTopMetaAddr)
		{
			LONG localSize = InterlockedDecrement(&_size);

			//로그찍기
			/*UINT64 ret = memoryLog.WriteMemoryLog(POP, newTop, 
				(newTop == nullptr) ? nullptr : newTop->pNext, pLocalTop, pLocalTop->pNext, pRealTopMetaAddr->pNext, localSize);*/

			T retData = pLocalTopRealAddr->data;
		
			_memoryPool.Free(pLocalTopRealAddr);
			return retData;
		}
	}
}
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
	//New ��� �Ҵ�
	// Pop���� �޸𸮸� ���� �ߴµ�, 
	// ������ �����ص� ������ �޸𸮿� �����ϴ¹����� �ذ��ϱ� ���� �޸� Ǯ���� �Ҵ� �޾Ƽ� ����Ѵ�.
	Node* pNewNode = _memoryPool.Alloc(data, nullptr);

	while (true)
	{
		//���� ������ ���� Top ��� �ּҸ� ����
		UINT_PTR pLocalTopMetaAddr = _pTop;

		//���ο� ����� next�� ���� ������ ������ Top����� �ּҸ� ����
		pNewNode->pNext = reinterpret_cast<Node*>(pLocalTopMetaAddr);

		//addressID�� �߱� �޾Ƽ� NewNode�� �޸� �ּ� ���� 16��Ʈ�� �� ���� ���� (ABA ���� ����)
		UINT64 addrID = AddressConverter::GetAddressID(&_addrIDCount);
		UINT_PTR pNewNodeMetaAddr = AddressConverter::InsertMetaIDToAddress((UINT_PTR)pNewNode, addrID);

		//���� Top��尡 ���� ������ ������ Top���� ������ ���ٸ� ���ο� ��带 ������ Top���� ����
		if ((UINT_PTR)InterlockedCompareExchange64((__int64*)&_pTop, pNewNodeMetaAddr, pLocalTopMetaAddr) == pLocalTopMetaAddr)
		{
			LONG localSize = InterlockedIncrement(&_size);

			//�α����
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

			//�α����
			/*UINT64 ret = memoryLog.WriteMemoryLog(POP, newTop, 
				(newTop == nullptr) ? nullptr : newTop->pNext, pLocalTop, pLocalTop->pNext, pRealTopMetaAddr->pNext, localSize);*/

			T retData = pLocalTopRealAddr->data;
		
			_memoryPool.Free(pLocalTopRealAddr);
			return retData;
		}
	}
}
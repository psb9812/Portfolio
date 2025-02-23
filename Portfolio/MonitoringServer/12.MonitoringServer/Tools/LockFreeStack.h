#pragma once
#include <Windows.h>
#include <optional>
#include "TLSMemoryPool.h"
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
	std::optional<T> Pop();
	void Clear();

private:
	TLSMemoryPool<Node> _memoryPool;
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
	Node* pNewNode = _memoryPool.Alloc(data, nullptr);

	while (true)
	{
		UINT_PTR pLocalTopMetaAddr = _pTop;

		pNewNode->pNext = reinterpret_cast<Node*>(pLocalTopMetaAddr);

		UINT64 addrID = AddressConverter::GetAddressID(&_addrIDCount);
		UINT_PTR pNewNodeMetaAddr = AddressConverter::InsertMetaIDToAddress((UINT_PTR)pNewNode, addrID);

		if ((UINT_PTR)InterlockedCompareExchange64((__int64*)&_pTop, pNewNodeMetaAddr, pLocalTopMetaAddr) == pLocalTopMetaAddr)
		{
			LONG localSize = InterlockedIncrement(&_size);
			break;
		}
	}
}

template<typename T>
inline std::optional<T> LockFreeStack<T>::Pop()
{
	Node* pRealTopMetaAddr = nullptr;

	if (_pTop == NULL)
	{
		return std::nullopt;
	}

	while (true)
	{
		UINT_PTR pLocalTopMetaAddr = _pTop;
		Node* pLocalTopRealAddr = reinterpret_cast<Node*>(AddressConverter::DeleteMetaIDToAddress(pLocalTopMetaAddr));
		UINT_PTR pNewTopMetaAddr = reinterpret_cast<UINT_PTR>(pLocalTopRealAddr->pNext);

		if ((pRealTopMetaAddr = (Node*)InterlockedCompareExchange64((__int64*)&_pTop, pNewTopMetaAddr, pLocalTopMetaAddr)) == (Node*)pLocalTopMetaAddr)
		{
			LONG localSize = InterlockedDecrement(&_size);

			T retData = pLocalTopRealAddr->data;

			_memoryPool.Free(pLocalTopRealAddr);
			return retData;
		}
	}
}

template<typename T>
inline void LockFreeStack<T>::Clear()
{
	while (Pop().has_value()) {}
}

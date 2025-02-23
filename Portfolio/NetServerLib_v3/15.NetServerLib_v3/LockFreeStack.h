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
			break;
		}
	}
}

template<typename T>
inline std::optional<T> LockFreeStack<T>::Pop()
{
	Node* pRealTopMetaAddr = nullptr;

	//Pop할 노드가 없으면 바로 반환
	if (_pTop == NULL)
	{
		return std::nullopt;
	}

	while (true)
	{
		//현재 Top노드 주소를 지역에 저장
		UINT_PTR pLocalTopMetaAddr = _pTop;
		//Top노드는 Meta주소(상위 17비트에 addressID가 저장된 주소) 이므로 주소 참조를 위해 이를 제거한다.
		Node* pLocalTopRealAddr = reinterpret_cast<Node*>(AddressConverter::DeleteMetaIDToAddress(pLocalTopMetaAddr));
		//실제 Top노드 next를 참조해서 Pop 이후에 TopNode가 될 노드의 Meta 주소를 얻어낸다.
		UINT_PTR pNewTopMetaAddr = reinterpret_cast<UINT_PTR>(pLocalTopRealAddr->pNext);

		//현재 Top의 주소가 지역 Top의 주소와 같다면, CAS 연산이 성공해서 Top의 메모리 주소를 바꾼다. 
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

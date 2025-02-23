/*---------------------------------------------------------------
	LockFreeMemoryPool.

	락프리 메모리 풀 클래스 (오브젝트 풀 / 프리리스트)
	특정 데이터(구조체,클래스,변수)를 일정량 할당 후 나눠쓴다.

	- 사용법.

	CMemoryPool<DATA> MemPool(300, FALSE);
	DATA *pData = MemPool.Alloc();

	pData 사용

	MemPool.Free(pData);

	- 안전 장치
	안전장치를 넣고 싶다면 #define MEMORY_POOL_SAFEMODE 매크로를 정의하자
	그러면 Free 할 때 해당 풀의 메모리였는지와 메모리 블록 앞 뒤로 변조된 부분은 없는지 검사하는 작업이 추가된다.
	- _pTopNode은 MetaAddress를 가진다.
	여기서 MetaAddress는 ABA 문제 해결을 위한 메모리 주소의 상위 17바이트에 Meta 정보를 삽입한 주소를 말한다.
----------------------------------------------------------------*/
#pragma once
#include <Windows.h>
#include "AddressConverter.h"

template <typename DATA>
class LockFreeMemoryPool
{
public:


#ifdef MEMORY_POOL_SAFEMODE
	struct Node
	{
		void* forwardSafeArea;	//앞쪽 안전 영역
		DATA data;
		Node* next;
		void* backwardSafeArea;	//뒤쪽 안전 영역
	};
#else
	struct Node
	{
		DATA data;
		Node* next;
	};
#endif // MEMORY_POOL_SAFEMODE


	//////////////////////////////////////////////////////////////////////////
	// 생성자, 파괴자.
	//
	// Parameters:	(int) 초기 블럭 개수.
	//				(bool) Alloc 시 생성자 / Free 시 파괴자 호출 여부
	//				(Args...) 생성자 호출 시 생성자의 인자로 넘겨줄 가변 인자.
	// Return:
	//////////////////////////////////////////////////////////////////////////
	template<typename... Args>
	LockFreeMemoryPool(int blockNum, bool placementNew = false, Args... args)
		:_capacity(blockNum), _useCount(0), _usePlacementNew(placementNew), _pTopNode(NULL), _addrIDCount(0)
	{

		if (!AddressConverter::CheckSpareBitNum())
		{
			__debugbreak();
		}

		Node* prevNode = nullptr;

		//blockNum값 만큼 메모리를 할당해준다.
		for (int i = 0; i < _capacity; i++)
		{
			Node* curNode = nullptr;
			curNode = (Node*)malloc(sizeof(Node));
			if (curNode == nullptr)
			{
				__debugbreak();
			}


#ifdef MEMORY_POOL_SAFEMODE
			//안전 모드의 경우 앞뒤로 현재 메모리 풀의 인스턴스 주소값을 넣어준다.
			curNode->forwardSafeArea = this;
			curNode->backwardSafeArea = this;
#endif // MEMORY_POOL_SAFEMODE


			if (prevNode == nullptr)
			{
				//_pTopNode에 대입할 때는 가공된 주소를 넣어야 한다.
				UINT64 addrID = AddressConverter::GetAddressID(&_addrIDCount);
				UINT_PTR curNodeMetaAddr = AddressConverter::InsertAddressIDToAddress(reinterpret_cast<UINT_PTR>(curNode), addrID);

				_pTopNode = curNodeMetaAddr;
			}
			else
			{
				prevNode->next = curNode;
			}

			curNode->next = nullptr;

			prevNode = curNode;

			//Placement New를 쓰지 않기로 했다면 최초 할당할 때 생성자를 호출해준다.
			if (!_usePlacementNew)
			{
				new (&(curNode->data)) DATA(args...);
			}
		}

	}
	~LockFreeMemoryPool()
	{
		Node* pRealTopNode = reinterpret_cast<Node*>(AddressConverter::DeleteAddressIDFromAddress(_pTopNode));
		//현재 내가 들고 있는 메모리만 해제해준다.
		while (pRealTopNode != nullptr)
		{
			Node* delNode = pRealTopNode;
			pRealTopNode = reinterpret_cast<Node*>(AddressConverter::DeleteAddressIDFromAddress((UINT_PTR)(pRealTopNode->next)));
			free(delNode);
		}
	}


	//////////////////////////////////////////////////////////////////////////
	// 블럭 하나를 할당받는다.  
	//
	// Parameters: 없음.
	// Return: (DATA *) 데이타 블럭 포인터.
	//////////////////////////////////////////////////////////////////////////
	template<typename... Args>
	DATA* Alloc(Args... args)
	{
		UINT_PTR pRealTop = NULL;

		while (true)
		{
			UINT_PTR pLocalTopMetaAddr = _pTopNode;
			//가지고 있는 블록을 다 Alloc 해줬는데 더 할당을 요청할 경우(_pTopNode == nullptr) 새로 만들어준다.
			if (pLocalTopMetaAddr == NULL)
			{
				Node* newNode = (Node*)malloc(sizeof(Node));
				if (newNode == nullptr)
				{
					__debugbreak();
				}
#ifdef MEMORY_POOL_SAFEMODE
				//안전 모드의 경우 앞뒤로 현재 메모리 풀의 인스턴스 주소값을 넣어준다.
				newNode->forwardSafeArea = this;
				newNode->backwardSafeArea = this;
#endif // MEMORY_POOL_SAFEMODE

				newNode->next = nullptr;

				new(&(newNode->data)) DATA(args...);

				InterlockedIncrement(&_capacity);
				InterlockedIncrement(&_useCount);

				DATA* ret = nullptr;
#ifdef MEMORY_POOL_SAFEMODE
				char* pointOfData = (char*)newNode;
				pointOfData += sizeof(char*);
				ret = reinterpret_cast<DATA*>(pointOfData);
#else
				ret = reinterpret_cast<DATA*>(newNode);
#endif // MEMORY_POOL_SAFEMODE

				return ret;
			}

			Node* pLocalTopRealAddr = reinterpret_cast<Node*>(AddressConverter::DeleteAddressIDFromAddress(pLocalTopMetaAddr));
			UINT_PTR pNewTopMetaAddr = reinterpret_cast<UINT_PTR>(pLocalTopRealAddr->next);

			if ((pRealTop = (UINT_PTR)InterlockedCompareExchange64((__int64*)&_pTopNode, pNewTopMetaAddr, pLocalTopMetaAddr)) == pLocalTopMetaAddr)
			{
				DATA* ret = nullptr;
#ifdef MEMORY_POOL_SAFEMODE
				char* pointOfData = (char*)pLocalTopRealAddr;
				pointOfData += sizeof(char*);
				ret = reinterpret_cast<DATA*>(pointOfData);
#else
				ret = reinterpret_cast<DATA*>(pLocalTopRealAddr);
#endif // MEMORY_POOL_SAFEMODE

				if (_usePlacementNew)
				{
					new (&(pLocalTopRealAddr->data)) DATA(args...);
				}

				InterlockedIncrement(&_useCount);

				return ret;
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////
	// 사용중이던 블럭을 해제한다.
	//
	// Parameters: (DATA *) 블럭 포인터.
	// Return: (BOOL) TRUE, FALSE.
	//////////////////////////////////////////////////////////////////////////
	bool Free(DATA* pData)
	{
		//아직 Alloc 한 적이 없다면 바로 false 반환
		if (_useCount == 0)
		{
			return false;
		}

#ifdef MEMORY_POOL_SAFEMODE
		// 안전장치로 넣은 앞뒤의 인스턴스의 주소가 Free해 왔을 때도 여전히 같은 값인지 검사한다.
		__int64 forwardAreaValue;
		__int64 backwardAreaValue;

		char* safeArea = (char*)pData;
		safeArea -= sizeof(void*);
		forwardAreaValue = *((__int64*)safeArea);
		safeArea = safeArea + (sizeof(void*) + sizeof(DATA) + sizeof(Node*));
		backwardAreaValue = *((__int64*)safeArea);

		if (forwardAreaValue != (__int64)this || backwardAreaValue != (__int64)this)
		{
			return false;
		}
#endif // MEMORY_POOL_SAFEMODE


		Node* newNode = nullptr;
#ifdef MEMORY_POOL_SAFEMODE
		char* cursor = reinterpret_cast<char*>(pData);
		cursor -= sizeof(char*);
		newNode = reinterpret_cast<Node*>(cursor);
#else
		newNode = reinterpret_cast<Node*>(pData);
#endif // MEMORY_POOL_SAFEMODE

		while (true)
		{
			UINT_PTR pLocalTopMetaAddr = _pTopNode;
			newNode->next = reinterpret_cast<Node*>(pLocalTopMetaAddr);

			//newNode의 주소를 가공한다.
			UINT64 addrID = AddressConverter::GetAddressID(&_addrIDCount);
			UINT_PTR newNodeMetaAddr = AddressConverter::InsertAddressIDToAddress((UINT_PTR)newNode, addrID);

			if (InterlockedCompareExchange64((__int64*)&_pTopNode, newNodeMetaAddr, pLocalTopMetaAddr) == (INT64)pLocalTopMetaAddr)
			{
				InterlockedDecrement(&_useCount);

				// PlacementNew를 한다면 다시 Alloc 할 때 생성자를 호출할 것이므로 소멸자를 호출시켜야 한다.
				if (_usePlacementNew)
				{
					pData->~DATA();
				}

				return true;
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////
	// 메모리 풀을 비운다.
	// 선행 조건 : Alloc한 모든 노드가 되돌아 와야 한다. (_useCount == 0)
	//
	// Parameters: 없음.
	// Return: (bool) Clear 가능 여부
	//////////////////////////////////////////////////////////////////////////
	bool Clear()
	{
		if (_useCount != 0)
		{
			return false;
		}

		Node* pRealTopNode = reinterpret_cast<Node*>(AddressConverter::DeleteAddressIDFromAddress(_pTopNode));
		//현재 내가 들고 있는 메모리만 해제해준다.
		while (pRealTopNode != nullptr)
		{
			Node* delNode = pRealTopNode;
			pRealTopNode = pRealTopNode->next;
			free(delNode);
			InterlockedDecrement(&_capacity);
		}

		return true;
	}

	//////////////////////////////////////////////////////////////////////////
	// 현재 확보 된 노드 개수를 얻는다. (메모리풀 내부의 전체 개수)
	//
	// Parameters: 없음.
	// Return: (int) 메모리 풀 내부 전체 개수
	//////////////////////////////////////////////////////////////////////////
	int	GetCapacityCount()
	{
		return _capacity;
	}

	//////////////////////////////////////////////////////////////////////////
	// 현재 사용중인 노드 개수를 얻는다.
	//
	// Parameters: 없음.
	// Return: (int) 사용중인 블럭 개수.
	//////////////////////////////////////////////////////////////////////////
	int	GetUseCount()
	{
		return _useCount;
	}

private:
	// 스택의 최상단 노드
	UINT_PTR _pTopNode;

	// 메모리 풀에서 생성한 노드의 수
	LONG _capacity;

	// 사용 중인 노드의 수
	LONG _useCount;

	// Alloc 함수를 호출할 때 PlacementNew 사용 여부 플래그
	bool _usePlacementNew;

	UINT64 _addrIDCount;
};
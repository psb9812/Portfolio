/*---------------------------------------------------------------
	TLSMemoryPool.

	락프리 메모리 풀 클래스 (오브젝트 풀 / 프리리스트)
	특정 데이터(구조체,클래스,변수)를 일정량 할당 후 나눠쓴다.

	- 사용법.

	MemoryPool<DATA> MemPool(300, FALSE);
	DATA *pData = MemPool.Alloc();

	pData 사용

	MemPool.Free(pData);

	- _pTopNodeBucket은 MetaAddress를 가진다.
	여기서 MetaAddress는 ABA 문제 해결을 위한 메모리 주소의 상위 17바이트에 Meta 정보를 삽입한 주소를 말한다.
----------------------------------------------------------------*/
#pragma once
#include <Windows.h>
#include "AddressConverter.h"
#include "MemoryLog.h"

template <typename DATA>
class TLSMemoryPool
{
public:
	static constexpr int BUCKET_SIZE = 100;

	struct Node
	{
		DATA data;
		void* pBucket = nullptr;		//사용할 때 NodeBucket* 타입으로 캐스팅해야 한다.
	};

	struct NodeBucket
	{
		LONG allocCount;
		Node nodeArr[BUCKET_SIZE];
		LONG freeCount;
		NodeBucket* next;

		NodeBucket()
			:allocCount(0), freeCount(0), next(nullptr)
		{
			//모든 노드들에게 돌아와야할 버킷의 포인터를 지정해둔다.
			for (int i = 0; i < BUCKET_SIZE; i++)
			{
				nodeArr[i].pBucket = this;
			}
		}

		template<typename... Args>
		NodeBucket(Args... args)
			:allocCount(0), freeCount(0), next(nullptr)
		{
			//모든 노드들에게 돌아와야할 버킷의 포인터를 지정해둔다.
			for (int i = 0; i < BUCKET_SIZE; i++)
			{
				nodeArr[i].pBucket = this;
			}
		}


		//Node의 생성자는 호출되지 않도록 Init 함수를 따로 만들다.
		void Init()
		{
			allocCount = 0;
			freeCount = 0;
			next = nullptr;

			for (int i = 0; i < BUCKET_SIZE; i++)
			{
				nodeArr[i].pBucket = this;
			}
		}
	};


	//////////////////////////////////////////////////////////////////////////
	// 생성자, 파괴자.
	//
	// Parameters:	(int) 초기 블럭 개수.
	//				(bool) Alloc 시 생성자 / Free 시 파괴자 호출 여부
	//				(Args...) 생성자 호출 시 생성자의 인자로 넘겨줄 가변 인자.
	// Return:
	//////////////////////////////////////////////////////////////////////////
	template<typename... Args>
	TLSMemoryPool(int bucketNum, bool placementNew = false, Args... args)
		:_capacity(bucketNum), _useCount(0), _usePlacementNew(placementNew), _pTopNodeBucket(NULL), _addrIDCount(0)
	{

		if (!AddressConverter::CheckSpareBitNum())
		{
			__debugbreak();
		}

		//메모리 풀 인스턴스가 사용할 TLS 인덱스를 할당받는다.
		_TLSIndex = TlsAlloc();
		if (_TLSIndex == TLS_OUT_OF_INDEXES)
		{
			__debugbreak();
		}

		NodeBucket* prevNodeBucket = nullptr;

		for (int i = 0; i < _capacity; i++)
		{
			//생성자 호출을 막기 위해 malloc과 Init 함수를 사용했다.
			NodeBucket* curNodeBucket = (NodeBucket*)malloc(sizeof(NodeBucket));
			if (curNodeBucket == nullptr)
			{
				__debugbreak();
			}

			curNodeBucket->Init();

			if (prevNodeBucket == nullptr)
			{
				//_pTopNodeBucket에 대입할 때는 가공된 주소를 넣어야 한다.
				UINT64 addrID = AddressConverter::GetAddressID(&_addrIDCount);
				UINT_PTR curNodeMetaAddr = AddressConverter::InsertMetaIDToAddress(reinterpret_cast<UINT_PTR>(curNodeBucket), addrID);

				_pTopNodeBucket = curNodeMetaAddr;
			}
			else
			{
				prevNodeBucket->next = curNodeBucket;
			}

			curNodeBucket->next = nullptr;

			prevNodeBucket = curNodeBucket;

			//Placement New를 쓰지 않기로 했다면 최초 할당할 때 생성자를 호출해준다.
			if (!_usePlacementNew)
			{
				for (int i = 0; i < BUCKET_SIZE; i++)
				{

					new (&(curNodeBucket->nodeArr[i].data)) DATA(args...);
				}
			}
		}
	}

	~TLSMemoryPool()
	{
		NodeBucket* pRealTopNode = reinterpret_cast<NodeBucket*>(AddressConverter::DeleteMetaIDToAddress(_pTopNodeBucket));
		//현재 내가 들고 있는 메모리만 해제해준다.
		while (pRealTopNode != nullptr)
		{
			NodeBucket* delNode = pRealTopNode;
			pRealTopNode = reinterpret_cast<NodeBucket*>(AddressConverter::DeleteMetaIDToAddress((UINT_PTR)(pRealTopNode->next)));
			delete delNode;
			InterlockedDecrement(&_capacity);
		}
	}


	//////////////////////////////////////////////////////////////////////////
	// 공용 풀에서 노드 버킷을 하나 할당 받는다.
	//
	// Parameters: 없음.
	// Return: (NodeBucket *) 노드 버킷 포인터.
	//////////////////////////////////////////////////////////////////////////
	template<typename... Args>
	NodeBucket* AllocFromPool(Args... args)
	{
		UINT_PTR pRealTop = NULL;

		while (true)
		{
			UINT_PTR pLocalTopMetaAddr = _pTopNodeBucket;
			//가지고 있는 블록을 다 Alloc 해줬는데 더 할당을 요청할 경우(_pTopNodeBucket == nullptr) 새로 만들어준다.
			if (pLocalTopMetaAddr == NULL)
			{
				NodeBucket* newNode = (NodeBucket*)malloc(sizeof(NodeBucket));
				newNode->Init();
				if (newNode == nullptr)
				{
					__debugbreak();
				}

				newNode->next = nullptr;

				InterlockedIncrement(&_capacity);
				InterlockedIncrement(&_useCount);

				if (!_usePlacementNew)
				{
					for (int i = 0; i < BUCKET_SIZE; i++)
					{
						new (&(newNode->nodeArr[i].data)) DATA(args...);
					}
				}

				return newNode;
			}

			NodeBucket* pLocalTopRealAddr = reinterpret_cast<NodeBucket*>(AddressConverter::DeleteMetaIDToAddress(pLocalTopMetaAddr));
			UINT_PTR pNewTopMetaAddr = reinterpret_cast<UINT_PTR>(pLocalTopRealAddr->next);

			if ((pRealTop = (UINT_PTR)InterlockedCompareExchange64((__int64*)&_pTopNodeBucket, pNewTopMetaAddr, pLocalTopMetaAddr)) == pLocalTopMetaAddr)
			{
				InterlockedIncrement(&_useCount);
				pLocalTopRealAddr->Init();

				return pLocalTopRealAddr;
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////
	// 사용 중이었던 노드 버킷을 공용 풀에 반환한다.
	//
	// Parameters: (NodeBucket *) 버킷 포인터.
	// Return: (BOOL) TRUE, FALSE.
	//////////////////////////////////////////////////////////////////////////
	bool FreeToPool(NodeBucket* pFreeBucket)
	{
		//아직 Alloc 한 적이 없다면 바로 false 반환
		if (_useCount == 0)
		{
			return false;
		}

		while (true)
		{
			UINT_PTR pLocalTopMetaAddr = _pTopNodeBucket;
			pFreeBucket->next = reinterpret_cast<NodeBucket*>(pLocalTopMetaAddr);

			//pFreeBucket의 주소를 가공한다.
			UINT64 addrID = AddressConverter::GetAddressID(&_addrIDCount);
			UINT_PTR pFreeBucketMetaAddr = AddressConverter::InsertMetaIDToAddress((UINT_PTR)pFreeBucket, addrID);

			if (InterlockedCompareExchange64((__int64*)&_pTopNodeBucket, pFreeBucketMetaAddr, pLocalTopMetaAddr) == (INT64)pLocalTopMetaAddr)
			{
				InterlockedDecrement(&_useCount);

				return true;
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////
	// TLS에 있는 노드 버킷에서 노드 하나를 할당받는다.
	//
	// Parameters: 노드의 생성자를 호출할 가변 인자.
	// Return: (DATA*) 데이터
	//////////////////////////////////////////////////////////////////////////
	template<typename... Args>
	DATA* Alloc(Args... args)
	{
		//TLS에 저장되어 있는 노드 버킷을 가져옴
		NodeBucket* pNodeBucket = (NodeBucket*)TlsGetValue(_TLSIndex);
		if (pNodeBucket == nullptr)
		{
			//유효한 값이 없으면 공용 풀에서 할당 받아오고 TLS에 셋팅
			pNodeBucket = AllocFromPool(args...);

			TlsSetValue(_TLSIndex, (LPVOID)pNodeBucket);
		}

		DATA* returnDataPtr = &(pNodeBucket->nodeArr[pNodeBucket->allocCount].data);

		pNodeBucket->allocCount++;

		//버킷의 모든 노드를 다 사용한 경우 TLS에서 버린다. 이후 이 버킷은 Free에서 반환해준다.
		if (pNodeBucket->allocCount == BUCKET_SIZE)
		{
			TlsSetValue(_TLSIndex, nullptr);
		}

		//생성자 호출 여부
		if (_usePlacementNew)
		{
			new (returnDataPtr) DATA(args...);
		}

		return returnDataPtr;
	}

	//////////////////////////////////////////////////////////////////////////
	// 할당 받은 노드를 반환한다.
	//
	// Parameters: 노드의 생성자를 호출할 가변 인자.
	// Return: (DATA*) 데이터
	//////////////////////////////////////////////////////////////////////////
	bool Free(DATA* pData)
	{
		NodeBucket* pNodeBucket = (NodeBucket*)(((Node*)pData)->pBucket);
		if (pNodeBucket == nullptr)
		{
			return false;
		}

		// PlacementNew를 한다면 다시 Alloc 할 때 생성자를 호출할 것이므로 소멸자를 호출시켜야 한다.
		if (_usePlacementNew)
		{
			pData->~DATA();
		}

		LONG ret = InterlockedIncrement(&(pNodeBucket->freeCount));
		if (ret == BUCKET_SIZE)
		{
			//중복 삭제 체크
			if (pNodeBucket->allocCount < BUCKET_SIZE)
			{
				__debugbreak();
			}
			//다 썼으면 반환해준다.
			FreeToPool(pNodeBucket);
		}
		return true;
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

		NodeBucket* pRealTopNode = reinterpret_cast<NodeBucket*>(AddressConverter::DeleteMetaIDToAddress(_pTopNodeBucket));
		//현재 내가 들고 있는 메모리만 해제해준다.
		while (pRealTopNode != nullptr)
		{
			NodeBucket* delNode = pRealTopNode;
			pRealTopNode = pRealTopNode->next;
			delete delNode;
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
	int	GetCapacityCount() const
	{
		return _capacity;
	}

	//////////////////////////////////////////////////////////////////////////
	// 현재 사용중인 노드 개수를 얻는다.
	//
	// Parameters: 없음.
	// Return: (int) 사용중인 블럭 개수.
	//////////////////////////////////////////////////////////////////////////
	int	GetUseCount() const
	{
		return _useCount;
	}

private:
	// 스택의 최상단 노드
	UINT_PTR _pTopNodeBucket;

	// 메모리 풀에서 생성한 청크의 개수
	LONG _capacity;

	// 사용 중인 노드의 수
	LONG _useCount;

	// Alloc 함수를 호출할 때 PlacementNew 사용 여부 플래그
	bool _usePlacementNew;

	UINT64 _addrIDCount;

	DWORD _TLSIndex;
};
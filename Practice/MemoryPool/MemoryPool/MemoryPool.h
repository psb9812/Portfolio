/*---------------------------------------------------------------

	MemoryPool.

	메모리 풀 클래스 (오브젝트 풀 / 프리리스트)
	특정 데이터(구조체,클래스,변수)를 일정량 할당 후 나눠쓴다.

	- 사용법.

	CMemoryPool<DATA> MemPool(300, FALSE);
	DATA *pData = MemPool.Alloc();

	pData 사용

	MemPool.Free(pData);

	- 안전 장치
	안전장치를 넣고 싶다면 #define MEMORY_POOL_SAFEMODE 매크로를 정의하자
	그러면 Free 할 때 내 풀의 메모리였는지와 메모리 블록 앞 뒤로 변조된 부분은 없는지 검사하는 작업이 추가된다.

----------------------------------------------------------------*/
#ifndef  __MEMORY_POOL__
#define  __MEMORY_POOL__

#include <iostream>

template <typename DATA>
class MemoryPool
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
	// Return:
	//////////////////////////////////////////////////////////////////////////
	MemoryPool(int blockNum, bool placementNew = false)
		:_capacity(blockNum), _useCount(0), _usePlacementNew(placementNew)
	{
		Node* prevNode = NULL;

		//blockNum값 만큼 메모리를 할당해준다.
		for (int i = 0; i < _capacity; i++)
		{
			Node* curNode;
			curNode = (Node*)malloc(sizeof(Node));
#ifdef MEMORY_POOL_SAFEMODE
			//안전 모드의 경우 앞뒤로 현재 메모리 풀의 인스턴스 주소값을 넣어준다.
			curNode->forwardSafeArea = this;
			curNode->backwardSafeArea = this;
#endif // MEMORY_POOL_SAFEMODE

			if (prevNode == NULL)
			{
				_freeNode = curNode;
			}
			else
			{
				prevNode->next = curNode;
			}

			curNode->next = NULL;

			prevNode = curNode;

			//Placement New를 쓰지 않기로 했다면 최초 할당할 때 생성자를 호출해준다.
			if (!_usePlacementNew)
			{
				new (curNode) Node;
			}
		}

	}
	virtual	~MemoryPool()
	{
		//현재 내가 들고 있는 메모리만 해제해준다.
		while (_freeNode != NULL)
		{
			Node* delNode = _freeNode;
			_freeNode = _freeNode->next;
			delete delNode;
		}
	}


	//////////////////////////////////////////////////////////////////////////
	// 블럭 하나를 할당받는다.  
	//
	// Parameters: 없음.
	// Return: (DATA *) 데이타 블럭 포인터.
	//////////////////////////////////////////////////////////////////////////
	DATA* Alloc(void)
	{
		//현재 가지고 있는 블록을 다 Alloc해줬는데 더 할당을 요청할 경우 새로 만들어준다.
		if (_freeNode == NULL)
		{
			_freeNode = (Node*)malloc(sizeof(Node));
#ifdef MEMORY_POOL_SAFEMODE
			//안전 모드의 경우 앞뒤로 현재 메모리 풀의 인스턴스 주소값을 넣어준다.
			_freeNode->forwardSafeArea = this;
			_freeNode->backwardSafeArea = this;
#endif // MEMORY_POOL_SAFEMODE

			_freeNode->next = NULL;

			if (!_usePlacementNew)
				new(_freeNode) Node;

			_capacity++;
		}

		DATA* ret = NULL;

#ifdef MEMORY_POOL_SAFEMODE
		char* pointOfData = (char*)_freeNode;
		pointOfData += sizeof(char*);
		ret = reinterpret_cast<DATA*>(pointOfData);
#else
		ret = reinterpret_cast<DATA*>(_freeNode);
#endif // MEMORY_POOL_SAFEMODE

		if (_usePlacementNew)
		{
			new (_freeNode) Node;
		}

		_freeNode = _freeNode->next;

		_useCount++;

		return ret;
	}

	//////////////////////////////////////////////////////////////////////////
	// 사용중이던 블럭을 해제한다.
	//
	// Parameters: (DATA *) 블럭 포인터.
	// Return: (BOOL) TRUE, FALSE.
	//////////////////////////////////////////////////////////////////////////
	bool	Free(DATA* pData)
	{
		//아직 Alloc 한 적이 없다면 바로 false 반환
		if (_useCount == 0) 
			return false;

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
			return false;
#endif // MEMORY_POOL_SAFEMODE

		Node* tempNode;
		//들어온 포인터를 다시 _freeNode에 연결해준다.
#ifdef MEMORY_POOL_SAFEMODE
		char* cursor = (char*)pData;
		cursor -= sizeof(char*);
		tempNode = (Node*)cursor;
#else
		tempNode = (Node*)pData;
#endif // MEMORY_POOL_SAFEMODE

		tempNode->next = _freeNode;
		_freeNode = tempNode;

		_useCount--;
		//사용하던 메모리를 정리하기 위해 소멸자를 호출해준다.
		pData->~DATA();
	
		return true;
	}

	//////////////////////////////////////////////////////////////////////////
	// 현재 확보 된 블럭 개수를 얻는다. (메모리풀 내부의 전체 개수)
	//
	// Parameters: 없음.
	// Return: (int) 메모리 풀 내부 전체 개수
	//////////////////////////////////////////////////////////////////////////
	int		GetCapacityCount(void) { return _capacity; }

	//////////////////////////////////////////////////////////////////////////
	// 현재 사용중인 블럭 개수를 얻는다.
	//
	// Parameters: 없음.
	// Return: (int) 사용중인 블럭 개수.
	//////////////////////////////////////////////////////////////////////////
	int		GetUseCount(void) { return _useCount; }


	// 스택 방식으로 반환된 (미사용) 오브젝트 블럭을 관리.

private:

	Node* _freeNode;
	int _capacity;
	int _useCount;
	bool _usePlacementNew;
};

#endif
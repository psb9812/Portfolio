/*---------------------------------------------------------------
	LockFreeMemoryPool.

	������ �޸� Ǯ Ŭ���� (������Ʈ Ǯ / ��������Ʈ)
	Ư�� ������(����ü,Ŭ����,����)�� ������ �Ҵ� �� ��������.

	- ����.

	CMemoryPool<DATA> MemPool(300, FALSE);
	DATA *pData = MemPool.Alloc();

	pData ���

	MemPool.Free(pData);

	- ���� ��ġ
	������ġ�� �ְ� �ʹٸ� #define MEMORY_POOL_SAFEMODE ��ũ�θ� ��������
	�׷��� Free �� �� �ش� Ǯ�� �޸𸮿������� �޸� ��� �� �ڷ� ������ �κ��� ������ �˻��ϴ� �۾��� �߰��ȴ�.
	- _pTopNode�� MetaAddress�� ������.
	���⼭ MetaAddress�� ABA ���� �ذ��� ���� �޸� �ּ��� ���� 17����Ʈ�� Meta ������ ������ �ּҸ� ���Ѵ�.
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
		void* forwardSafeArea;	//���� ���� ����
		DATA data;
		Node* next;
		void* backwardSafeArea;	//���� ���� ����
	};
#else
	struct Node
	{
		DATA data;
		Node* next;
	};
#endif // MEMORY_POOL_SAFEMODE


	//////////////////////////////////////////////////////////////////////////
	// ������, �ı���.
	//
	// Parameters:	(int) �ʱ� �� ����.
	//				(bool) Alloc �� ������ / Free �� �ı��� ȣ�� ����
	//				(Args...) ������ ȣ�� �� �������� ���ڷ� �Ѱ��� ���� ����.
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

		//blockNum�� ��ŭ �޸𸮸� �Ҵ����ش�.
		for (int i = 0; i < _capacity; i++)
		{
			Node* curNode = nullptr;
			curNode = (Node*)malloc(sizeof(Node));
			if (curNode == nullptr)
			{
				__debugbreak();
			}


#ifdef MEMORY_POOL_SAFEMODE
			//���� ����� ��� �յڷ� ���� �޸� Ǯ�� �ν��Ͻ� �ּҰ��� �־��ش�.
			curNode->forwardSafeArea = this;
			curNode->backwardSafeArea = this;
#endif // MEMORY_POOL_SAFEMODE


			if (prevNode == nullptr)
			{
				//_pTopNode�� ������ ���� ������ �ּҸ� �־�� �Ѵ�.
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

			//Placement New�� ���� �ʱ�� �ߴٸ� ���� �Ҵ��� �� �����ڸ� ȣ�����ش�.
			if (!_usePlacementNew)
			{
				new (&(curNode->data)) DATA(args...);
			}
		}

	}
	~LockFreeMemoryPool()
	{
		Node* pRealTopNode = reinterpret_cast<Node*>(AddressConverter::DeleteAddressIDFromAddress(_pTopNode));
		//���� ���� ��� �ִ� �޸𸮸� �������ش�.
		while (pRealTopNode != nullptr)
		{
			Node* delNode = pRealTopNode;
			pRealTopNode = reinterpret_cast<Node*>(AddressConverter::DeleteAddressIDFromAddress((UINT_PTR)(pRealTopNode->next)));
			free(delNode);
		}
	}


	//////////////////////////////////////////////////////////////////////////
	// �� �ϳ��� �Ҵ�޴´�.  
	//
	// Parameters: ����.
	// Return: (DATA *) ����Ÿ �� ������.
	//////////////////////////////////////////////////////////////////////////
	template<typename... Args>
	DATA* Alloc(Args... args)
	{
		UINT_PTR pRealTop = NULL;

		while (true)
		{
			UINT_PTR pLocalTopMetaAddr = _pTopNode;
			//������ �ִ� ����� �� Alloc ����µ� �� �Ҵ��� ��û�� ���(_pTopNode == nullptr) ���� ������ش�.
			if (pLocalTopMetaAddr == NULL)
			{
				Node* newNode = (Node*)malloc(sizeof(Node));
				if (newNode == nullptr)
				{
					__debugbreak();
				}
#ifdef MEMORY_POOL_SAFEMODE
				//���� ����� ��� �յڷ� ���� �޸� Ǯ�� �ν��Ͻ� �ּҰ��� �־��ش�.
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
	// ������̴� ���� �����Ѵ�.
	//
	// Parameters: (DATA *) �� ������.
	// Return: (BOOL) TRUE, FALSE.
	//////////////////////////////////////////////////////////////////////////
	bool Free(DATA* pData)
	{
		//���� Alloc �� ���� ���ٸ� �ٷ� false ��ȯ
		if (_useCount == 0)
		{
			return false;
		}

#ifdef MEMORY_POOL_SAFEMODE
		// ������ġ�� ���� �յ��� �ν��Ͻ��� �ּҰ� Free�� ���� ���� ������ ���� ������ �˻��Ѵ�.
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

			//newNode�� �ּҸ� �����Ѵ�.
			UINT64 addrID = AddressConverter::GetAddressID(&_addrIDCount);
			UINT_PTR newNodeMetaAddr = AddressConverter::InsertAddressIDToAddress((UINT_PTR)newNode, addrID);

			if (InterlockedCompareExchange64((__int64*)&_pTopNode, newNodeMetaAddr, pLocalTopMetaAddr) == (INT64)pLocalTopMetaAddr)
			{
				InterlockedDecrement(&_useCount);

				// PlacementNew�� �Ѵٸ� �ٽ� Alloc �� �� �����ڸ� ȣ���� ���̹Ƿ� �Ҹ��ڸ� ȣ����Ѿ� �Ѵ�.
				if (_usePlacementNew)
				{
					pData->~DATA();
				}

				return true;
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////
	// �޸� Ǯ�� ����.
	// ���� ���� : Alloc�� ��� ��尡 �ǵ��� �;� �Ѵ�. (_useCount == 0)
	//
	// Parameters: ����.
	// Return: (bool) Clear ���� ����
	//////////////////////////////////////////////////////////////////////////
	bool Clear()
	{
		if (_useCount != 0)
		{
			return false;
		}

		Node* pRealTopNode = reinterpret_cast<Node*>(AddressConverter::DeleteAddressIDFromAddress(_pTopNode));
		//���� ���� ��� �ִ� �޸𸮸� �������ش�.
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
	// ���� Ȯ�� �� ��� ������ ��´�. (�޸�Ǯ ������ ��ü ����)
	//
	// Parameters: ����.
	// Return: (int) �޸� Ǯ ���� ��ü ����
	//////////////////////////////////////////////////////////////////////////
	int	GetCapacityCount()
	{
		return _capacity;
	}

	//////////////////////////////////////////////////////////////////////////
	// ���� ������� ��� ������ ��´�.
	//
	// Parameters: ����.
	// Return: (int) ������� �� ����.
	//////////////////////////////////////////////////////////////////////////
	int	GetUseCount()
	{
		return _useCount;
	}

private:
	// ������ �ֻ�� ���
	UINT_PTR _pTopNode;

	// �޸� Ǯ���� ������ ����� ��
	LONG _capacity;

	// ��� ���� ����� ��
	LONG _useCount;

	// Alloc �Լ��� ȣ���� �� PlacementNew ��� ���� �÷���
	bool _usePlacementNew;

	UINT64 _addrIDCount;
};
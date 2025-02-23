/*---------------------------------------------------------------
	TLSMemoryPool.

	������ �޸� Ǯ Ŭ���� (������Ʈ Ǯ / ��������Ʈ)
	Ư�� ������(����ü,Ŭ����,����)�� ������ �Ҵ� �� ��������.

	- ����.

	MemoryPool<DATA> MemPool(300, FALSE);
	DATA *pData = MemPool.Alloc();

	pData ���

	MemPool.Free(pData);

	- _pTopNodeBucket�� MetaAddress�� ������.
	���⼭ MetaAddress�� ABA ���� �ذ��� ���� �޸� �ּ��� ���� 17����Ʈ�� Meta ������ ������ �ּҸ� ���Ѵ�.
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
		void* pBucket = nullptr;		//����� �� NodeBucket* Ÿ������ ĳ�����ؾ� �Ѵ�.
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
			//��� ���鿡�� ���ƿ;��� ��Ŷ�� �����͸� �����صд�.
			for (int i = 0; i < BUCKET_SIZE; i++)
			{
				nodeArr[i].pBucket = this;
			}
		}

		template<typename... Args>
		NodeBucket(Args... args)
			:allocCount(0), freeCount(0), next(nullptr)
		{
			//��� ���鿡�� ���ƿ;��� ��Ŷ�� �����͸� �����صд�.
			for (int i = 0; i < BUCKET_SIZE; i++)
			{
				nodeArr[i].pBucket = this;
			}
		}


		//Node�� �����ڴ� ȣ����� �ʵ��� Init �Լ��� ���� �����.
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
	// ������, �ı���.
	//
	// Parameters:	(int) �ʱ� �� ����.
	//				(bool) Alloc �� ������ / Free �� �ı��� ȣ�� ����
	//				(Args...) ������ ȣ�� �� �������� ���ڷ� �Ѱ��� ���� ����.
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

		//�޸� Ǯ �ν��Ͻ��� ����� TLS �ε����� �Ҵ�޴´�.
		_TLSIndex = TlsAlloc();
		if (_TLSIndex == TLS_OUT_OF_INDEXES)
		{
			__debugbreak();
		}

		NodeBucket* prevNodeBucket = nullptr;

		for (int i = 0; i < _capacity; i++)
		{
			//������ ȣ���� ���� ���� malloc�� Init �Լ��� ����ߴ�.
			NodeBucket* curNodeBucket = (NodeBucket*)malloc(sizeof(NodeBucket));
			if (curNodeBucket == nullptr)
			{
				__debugbreak();
			}

			curNodeBucket->Init();

			if (prevNodeBucket == nullptr)
			{
				//_pTopNodeBucket�� ������ ���� ������ �ּҸ� �־�� �Ѵ�.
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

			//Placement New�� ���� �ʱ�� �ߴٸ� ���� �Ҵ��� �� �����ڸ� ȣ�����ش�.
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
		//���� ���� ��� �ִ� �޸𸮸� �������ش�.
		while (pRealTopNode != nullptr)
		{
			NodeBucket* delNode = pRealTopNode;
			pRealTopNode = reinterpret_cast<NodeBucket*>(AddressConverter::DeleteMetaIDToAddress((UINT_PTR)(pRealTopNode->next)));
			delete delNode;
			InterlockedDecrement(&_capacity);
		}
	}


	//////////////////////////////////////////////////////////////////////////
	// ���� Ǯ���� ��� ��Ŷ�� �ϳ� �Ҵ� �޴´�.
	//
	// Parameters: ����.
	// Return: (NodeBucket *) ��� ��Ŷ ������.
	//////////////////////////////////////////////////////////////////////////
	template<typename... Args>
	NodeBucket* AllocFromPool(Args... args)
	{
		UINT_PTR pRealTop = NULL;

		while (true)
		{
			UINT_PTR pLocalTopMetaAddr = _pTopNodeBucket;
			//������ �ִ� ����� �� Alloc ����µ� �� �Ҵ��� ��û�� ���(_pTopNodeBucket == nullptr) ���� ������ش�.
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
	// ��� ���̾��� ��� ��Ŷ�� ���� Ǯ�� ��ȯ�Ѵ�.
	//
	// Parameters: (NodeBucket *) ��Ŷ ������.
	// Return: (BOOL) TRUE, FALSE.
	//////////////////////////////////////////////////////////////////////////
	bool FreeToPool(NodeBucket* pFreeBucket)
	{
		//���� Alloc �� ���� ���ٸ� �ٷ� false ��ȯ
		if (_useCount == 0)
		{
			return false;
		}

		while (true)
		{
			UINT_PTR pLocalTopMetaAddr = _pTopNodeBucket;
			pFreeBucket->next = reinterpret_cast<NodeBucket*>(pLocalTopMetaAddr);

			//pFreeBucket�� �ּҸ� �����Ѵ�.
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
	// TLS�� �ִ� ��� ��Ŷ���� ��� �ϳ��� �Ҵ�޴´�.
	//
	// Parameters: ����� �����ڸ� ȣ���� ���� ����.
	// Return: (DATA*) ������
	//////////////////////////////////////////////////////////////////////////
	template<typename... Args>
	DATA* Alloc(Args... args)
	{
		//TLS�� ����Ǿ� �ִ� ��� ��Ŷ�� ������
		NodeBucket* pNodeBucket = (NodeBucket*)TlsGetValue(_TLSIndex);
		if (pNodeBucket == nullptr)
		{
			//��ȿ�� ���� ������ ���� Ǯ���� �Ҵ� �޾ƿ��� TLS�� ����
			pNodeBucket = AllocFromPool(args...);

			TlsSetValue(_TLSIndex, (LPVOID)pNodeBucket);
		}

		DATA* returnDataPtr = &(pNodeBucket->nodeArr[pNodeBucket->allocCount].data);

		pNodeBucket->allocCount++;

		//��Ŷ�� ��� ��带 �� ����� ��� TLS���� ������. ���� �� ��Ŷ�� Free���� ��ȯ���ش�.
		if (pNodeBucket->allocCount == BUCKET_SIZE)
		{
			TlsSetValue(_TLSIndex, nullptr);
		}

		//������ ȣ�� ����
		if (_usePlacementNew)
		{
			new (returnDataPtr) DATA(args...);
		}

		return returnDataPtr;
	}

	//////////////////////////////////////////////////////////////////////////
	// �Ҵ� ���� ��带 ��ȯ�Ѵ�.
	//
	// Parameters: ����� �����ڸ� ȣ���� ���� ����.
	// Return: (DATA*) ������
	//////////////////////////////////////////////////////////////////////////
	bool Free(DATA* pData)
	{
		NodeBucket* pNodeBucket = (NodeBucket*)(((Node*)pData)->pBucket);
		if (pNodeBucket == nullptr)
		{
			return false;
		}

		// PlacementNew�� �Ѵٸ� �ٽ� Alloc �� �� �����ڸ� ȣ���� ���̹Ƿ� �Ҹ��ڸ� ȣ����Ѿ� �Ѵ�.
		if (_usePlacementNew)
		{
			pData->~DATA();
		}

		LONG ret = InterlockedIncrement(&(pNodeBucket->freeCount));
		if (ret == BUCKET_SIZE)
		{
			//�ߺ� ���� üũ
			if (pNodeBucket->allocCount < BUCKET_SIZE)
			{
				__debugbreak();
			}
			//�� ������ ��ȯ���ش�.
			FreeToPool(pNodeBucket);
		}
		return true;
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

		NodeBucket* pRealTopNode = reinterpret_cast<NodeBucket*>(AddressConverter::DeleteMetaIDToAddress(_pTopNodeBucket));
		//���� ���� ��� �ִ� �޸𸮸� �������ش�.
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
	// ���� Ȯ�� �� ��� ������ ��´�. (�޸�Ǯ ������ ��ü ����)
	//
	// Parameters: ����.
	// Return: (int) �޸� Ǯ ���� ��ü ����
	//////////////////////////////////////////////////////////////////////////
	int	GetCapacityCount() const
	{
		return _capacity;
	}

	//////////////////////////////////////////////////////////////////////////
	// ���� ������� ��� ������ ��´�.
	//
	// Parameters: ����.
	// Return: (int) ������� �� ����.
	//////////////////////////////////////////////////////////////////////////
	int	GetUseCount() const
	{
		return _useCount;
	}

private:
	// ������ �ֻ�� ���
	UINT_PTR _pTopNodeBucket;

	// �޸� Ǯ���� ������ ûũ�� ����
	LONG _capacity;

	// ��� ���� ����� ��
	LONG _useCount;

	// Alloc �Լ��� ȣ���� �� PlacementNew ��� ���� �÷���
	bool _usePlacementNew;

	UINT64 _addrIDCount;

	DWORD _TLSIndex;
};
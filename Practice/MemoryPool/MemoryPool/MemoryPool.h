/*---------------------------------------------------------------

	MemoryPool.

	�޸� Ǯ Ŭ���� (������Ʈ Ǯ / ��������Ʈ)
	Ư�� ������(����ü,Ŭ����,����)�� ������ �Ҵ� �� ��������.

	- ����.

	CMemoryPool<DATA> MemPool(300, FALSE);
	DATA *pData = MemPool.Alloc();

	pData ���

	MemPool.Free(pData);

	- ���� ��ġ
	������ġ�� �ְ� �ʹٸ� #define MEMORY_POOL_SAFEMODE ��ũ�θ� ��������
	�׷��� Free �� �� �� Ǯ�� �޸𸮿������� �޸� ��� �� �ڷ� ������ �κ��� ������ �˻��ϴ� �۾��� �߰��ȴ�.

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
	// Return:
	//////////////////////////////////////////////////////////////////////////
	MemoryPool(int blockNum, bool placementNew = false)
		:_capacity(blockNum), _useCount(0), _usePlacementNew(placementNew)
	{
		Node* prevNode = NULL;

		//blockNum�� ��ŭ �޸𸮸� �Ҵ����ش�.
		for (int i = 0; i < _capacity; i++)
		{
			Node* curNode;
			curNode = (Node*)malloc(sizeof(Node));
#ifdef MEMORY_POOL_SAFEMODE
			//���� ����� ��� �յڷ� ���� �޸� Ǯ�� �ν��Ͻ� �ּҰ��� �־��ش�.
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

			//Placement New�� ���� �ʱ�� �ߴٸ� ���� �Ҵ��� �� �����ڸ� ȣ�����ش�.
			if (!_usePlacementNew)
			{
				new (curNode) Node;
			}
		}

	}
	virtual	~MemoryPool()
	{
		//���� ���� ��� �ִ� �޸𸮸� �������ش�.
		while (_freeNode != NULL)
		{
			Node* delNode = _freeNode;
			_freeNode = _freeNode->next;
			delete delNode;
		}
	}


	//////////////////////////////////////////////////////////////////////////
	// �� �ϳ��� �Ҵ�޴´�.  
	//
	// Parameters: ����.
	// Return: (DATA *) ����Ÿ �� ������.
	//////////////////////////////////////////////////////////////////////////
	DATA* Alloc(void)
	{
		//���� ������ �ִ� ����� �� Alloc����µ� �� �Ҵ��� ��û�� ��� ���� ������ش�.
		if (_freeNode == NULL)
		{
			_freeNode = (Node*)malloc(sizeof(Node));
#ifdef MEMORY_POOL_SAFEMODE
			//���� ����� ��� �յڷ� ���� �޸� Ǯ�� �ν��Ͻ� �ּҰ��� �־��ش�.
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
	// ������̴� ���� �����Ѵ�.
	//
	// Parameters: (DATA *) �� ������.
	// Return: (BOOL) TRUE, FALSE.
	//////////////////////////////////////////////////////////////////////////
	bool	Free(DATA* pData)
	{
		//���� Alloc �� ���� ���ٸ� �ٷ� false ��ȯ
		if (_useCount == 0) 
			return false;

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
			return false;
#endif // MEMORY_POOL_SAFEMODE

		Node* tempNode;
		//���� �����͸� �ٽ� _freeNode�� �������ش�.
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
		//����ϴ� �޸𸮸� �����ϱ� ���� �Ҹ��ڸ� ȣ�����ش�.
		pData->~DATA();
	
		return true;
	}

	//////////////////////////////////////////////////////////////////////////
	// ���� Ȯ�� �� �� ������ ��´�. (�޸�Ǯ ������ ��ü ����)
	//
	// Parameters: ����.
	// Return: (int) �޸� Ǯ ���� ��ü ����
	//////////////////////////////////////////////////////////////////////////
	int		GetCapacityCount(void) { return _capacity; }

	//////////////////////////////////////////////////////////////////////////
	// ���� ������� �� ������ ��´�.
	//
	// Parameters: ����.
	// Return: (int) ������� �� ����.
	//////////////////////////////////////////////////////////////////////////
	int		GetUseCount(void) { return _useCount; }


	// ���� ������� ��ȯ�� (�̻��) ������Ʈ ���� ����.

private:

	Node* _freeNode;
	int _capacity;
	int _useCount;
	bool _usePlacementNew;
};

#endif
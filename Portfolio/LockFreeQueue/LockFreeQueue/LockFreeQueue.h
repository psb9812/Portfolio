#pragma once
#include <Windows.h>
#include <optional>
#include "TLSMemoryPool.h"
#include "AddressConverter.h"

template<typename T>
class LockFreeQueue
{
private:
    struct alignas(16) Address128
    {
        //QID : QID(8byte) / ���� 64��Ʈ
        //metaAddress : Meta ID(���� 17��Ʈ) + ���� �ּ�(���� 47��Ʈ) / ���� 64��Ʈ
        INT64 QID;
        INT64 metaAddress;

        Address128()
        {
            QID = 0;
            metaAddress = 0;
        }
        Address128(int qid, INT_PTR addr)
        {
            QID = (INT64)qid;
            metaAddress = addr;
        }

        INT64 GetQID() const
        {
            return QID;
        }

        UINT_PTR GetMetaAddress()
        {
            return (UINT_PTR)metaAddress;
        }

    };

    struct Node
    {
        T data;
        Address128 next128;

        Node()
            :data(NULL)
        {}

        Node(int qid, UINT64 addrID)
            :data(NULL), next128(qid, AddressConverter::InsertAddressIDToAddress((UINT_PTR)nullptr, addrID))
        {}

        Node(T value, int qid, UINT64 addrID)
            :data(value),
            next128(qid, AddressConverter::InsertAddressIDToAddress((UINT_PTR)nullptr, addrID))
        {}
    };

    //���� �� ���� ť �ν��Ͻ����� �������� ����ϴ� ���� ��� Ǯ
    static inline TLSMemoryPool<Node> _nodePool = TLSMemoryPool<Node>(0, true);

    //���� ���ø� Ÿ���� �� ���� ť �ν��Ͻ��� �ĺ��ϱ� ���� �ĺ��� ����� Count
    static inline LONG _QIDCount = 0;

    //�ش� �ν��Ͻ��� QID
    const LONG _QID;

    alignas(64) UINT_PTR _metaHead;
    alignas(64) UINT_PTR _metaTail;
    alignas(64) LONG _size;
    alignas(64) UINT64 _addrIDCount;

    const INT32 MAX_SIZE;

public:
    LockFreeQueue(int maxSize = 100000)
        :_size(0), _addrIDCount(0), _QID(_QIDCount++), MAX_SIZE(maxSize)
    {
        //���� ��� ���̵� �߱�
        UINT64 addrID = AddressConverter::GetAddressID(&_addrIDCount);
        //���� ��� ����
        Node* pDummy = _nodePool.Alloc(_QID, addrID);
        //��� / ���� �Ҵ�
        _metaHead = _metaTail = AddressConverter::InsertAddressIDToAddress((UINT_PTR)pDummy, addrID);
    }

    std::optional<LONG> Enqueue(T inputData)
    {
        UINT64 addrID = AddressConverter::GetAddressID(&_addrIDCount);
        //�Ҵ� �� ��, next�� �޸� �ּҿ� newNode�� ������ addrID�� �����Ѵ�. (for ��尡 �ߴ� ��Ȳ ����)
        Node* pNewNode = _nodePool.Alloc(inputData, _QID, addrID);
        //ABA������ ���� ���� �߱� ���� addrID�� �޸� �ּҿ� ����
        UINT_PTR newNodeMeta = AddressConverter::InsertAddressIDToAddress((UINT_PTR)pNewNode, addrID);

        while (true)
        {
            if (MAX_SIZE <= _size)
            {
                _nodePool.Free(pNewNode);
                return std::nullopt;
            }

            //ť�� metaTail�� ������ ����
            UINT_PTR localTailMeta = _metaTail;
            //ť�� realAddress ����(for next ����)
            Node* pLocalTailReal = (Node*)AddressConverter::DeleteAddressIDFromAddress(localTailMeta);

            //next�� ���� ��� Ǯ�� ���� DoubleCAS�� ��� �ϹǷ� QID + metaAddress�̴�.
            Address128 localTailNext128 = pLocalTailReal->next128;
            //next�� metaAddress ����
            UINT_PTR localTailNextMeta = localTailNext128.GetMetaAddress();
            //next�� realAddress ����
            Node* pLocalTailNextReal = (Node*)AddressConverter::DeleteAddressIDFromAddress(localTailNextMeta);

            //localTail�� next�� nullptr�� �ƴ϶�� �ɵ������� tail�� �� ĭ �о��ְ� �ٽ� tail ���� �޾ƿ���
            if (pLocalTailNextReal != nullptr)
            {
                InterlockedCompareExchange(&_metaTail, localTailNextMeta, localTailMeta);
                continue;
            }

            //������ ������ metaTail�� tail�� next�� �������� ť ��Ȳ�̶�� meta �ּҰ� ���ƾ� �����̴�.
            //�׷��� �ʴٸ� 
            if (AddressConverter::ExtractMetaID(localTailMeta) != AddressConverter::ExtractMetaID(localTailNextMeta))
            {
                continue;
            }

            //CAS1 (���� Ǯ�� ����� �� �ٸ� ť�� ��尡 �Ǿ���� ���� tail�� ��츦 �����ϱ� ���� Double CAS ����Ѵ�.)
            if (InterlockedCompareExchange128((__int64*)&(pLocalTailReal->next128), (LONG64)newNodeMeta, (LONG64)_QID, (__int64*)&localTailNext128) == false)
            {
                continue;
            }

            //CAS2 (for tail�� newNode�� �ű��)
            InterlockedCompareExchange(&_metaTail, newNodeMeta, localTailMeta);

            LONG retData = InterlockedIncrement(&_size);
            return retData;
        }
    }

    std::optional<T> Dequeue()
    {
        //����ִ��� üũ
        LONG localSize = InterlockedDecrement(&_size);
        if (localSize < 0)
        {
            InterlockedIncrement(&_size);
            return std::nullopt;
        }

        while (true)
        {
            UINT_PTR localHeadMeta = _metaHead;
            UINT_PTR localTailMeta = _metaTail;
            Node* pLocalHeadReal = (Node*)AddressConverter::DeleteAddressIDFromAddress(localHeadMeta);
            UINT_PTR localHeadNextMeta = pLocalHeadReal->next128.GetMetaAddress();
            Node* pLocalHeadNextReal = (Node*)AddressConverter::DeleteAddressIDFromAddress(localHeadNextMeta);

            //head�� next�� nullptr�̶�� �ٽ� head���� ���Ž�Ų��.
            //���� Ǯ �������� �������� ������ �ִ� ��� ��尡 �̹� �ٸ� ť�� ���� �� nullptr�� �� �� �ִ�. �̰��� �����Ϸ��� �ǵ�
            if (pLocalHeadNextReal == nullptr)
            {
                continue;
            }

            // ���� ���� ��Ȳ�̶�� �� ĭ �о��ش�.
            if (localHeadMeta == localTailMeta)
            {
                InterlockedCompareExchange(&_metaTail, localHeadNextMeta, localTailMeta);
                continue;
            }

            
            T retData = pLocalHeadNextReal->data;
            //head�� ���� ���� CAS�Ѵ�.
            if (InterlockedCompareExchange(&_metaHead, localHeadNextMeta, localHeadMeta) == localHeadMeta)
            {
                _nodePool.Free(pLocalHeadReal);
                return retData;
            }
        }
    }

    LONG GetSize() const
    {
        return _size;
    }

    void Clear()
    {
        while (Dequeue().has_value()) {}
    }
};
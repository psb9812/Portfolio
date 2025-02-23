#pragma once
#include <Windows.h>
#include <optional>
#include "TLSMemoryPool.h"
#include "AddressConverter.h"

/*
* # LockFreeQueue
* @ ���� ���� ����
*   - ABA ������ �ذ��ϱ� ���� TLSMemoryPool���� ��带 �Ҵ� �޴´�.
* 
*   - TLSMemoryPool���� LockFreeQueue�� ����ϱ� ���ؼ� QID�� ���� Queue ���� ��带 �ĺ��� �� �ְ� �Ѵ�.
*     �׷��� �ؼ� CAS�� ������ �� QID�� �˻��ؼ� �ٸ� Queue�� ���� CAS�� ������ ������ ť�� ������ ������ ���´�.
* 
*   - (Meta �ּ� + QID) ������ CAS�� �� �� ���� ���ϱ� ���ؼ� Address128 ����ü�� DoubleCAS�� �����Ѵ�.
*/

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
            :data(NULL), next128(qid, AddressConverter::InsertMetaIDToAddress((UINT_PTR)nullptr, addrID))
        {}

        Node(T value, int qid, UINT64 addrID)
            :data(value),
            next128(qid, AddressConverter::InsertMetaIDToAddress((UINT_PTR)nullptr, addrID))
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
    UINT64 _addrIDCount;

    const INT32 MAX_SIZE;

public:
    LockFreeQueue(int maxSize = 100000)
        :_size(0), _addrIDCount(0), _QID(_QIDCount++), MAX_SIZE(maxSize)
    {
        //���� ��� ���̵� �߱�
        UINT64 addrID = AddressConverter::GetAddressID(&_addrIDCount);
        //���� ��� ����
        Node* pDummy = _nodePool.Alloc(_QID, addrID);
        //���, ���� �Ҵ�
        _metaHead = _metaTail = AddressConverter::InsertMetaIDToAddress((UINT_PTR)pDummy, addrID);
    }

    std::optional<LONG> Enqueue(T inputData)
    {
        //���ο� addressID�� �߱޹ޱ� -> ��� �Ҵ�(QID -> addressID�� ���� �޸� �ּ� ���� 17��Ʈ�� ���� 
        UINT64 addrID = AddressConverter::GetAddressID(&_addrIDCount);
        Node* pNewNode = _nodePool.Alloc(inputData, _QID, addrID);
        UINT_PTR newNodeMeta = AddressConverter::InsertMetaIDToAddress((UINT_PTR)pNewNode, addrID);

        while (true)
        {
            //�ִ� ������ �̻����δ� ���� �Ұ�
            if (MAX_SIZE <= _size)
            {
                _nodePool.Free(pNewNode);
                return std::nullopt;
            }

            UINT_PTR localTailMeta = _metaTail;
            Node* pLocalTailReal = (Node*)AddressConverter::DeleteMetaIDToAddress(localTailMeta);
            Address128 localTailNext128 = pLocalTailReal->next128;
            UINT_PTR localTailNextMeta = localTailNext128.GetMetaAddress();
            Node* pLocalTailNextReal = (Node*)AddressConverter::DeleteMetaIDToAddress(localTailNextMeta);

            //������ ��尡 localTail�� next�� nullptr�� �ƴ϶�� �ɵ������� �� ĭ �о��ְ� �ٽ� �ݺ��� ����
            if (pLocalTailNextReal != nullptr)
            {
                InterlockedCompareExchange(&_metaTail, localTailNextMeta, localTailMeta);
                continue;
            }

            if (AddressConverter::ExtractMetaID(localTailMeta) != AddressConverter::ExtractMetaID(localTailNextMeta))
            {
                continue;
            }

            //CAS1 (���� Ǯ�� ����� �� �ٸ� ť�� ��尡 �Ǿ���� ���� tail�� ��츦 �����ϱ� ���� Double CAS ����Ѵ�.)
            if (InterlockedCompareExchange128((__int64*)&(pLocalTailReal->next128), (LONG64)newNodeMeta, (LONG64)_QID, (__int64*)&localTailNext128) == false)
            {
                continue;
            }

            //CAS2
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
            Node* pLocalHeadReal = (Node*)AddressConverter::DeleteMetaIDToAddress(localHeadMeta);
            UINT_PTR localHeadNextMeta = pLocalHeadReal->next128.GetMetaAddress();
            Node* pLocalHeadNextReal = (Node*)AddressConverter::DeleteMetaIDToAddress(localHeadNextMeta);

            //head�� next�� nullptr�̶�� �ٽ� head���� ���Ž�Ų��.
            //���� Ǯ �������� �������� ������ �ִ� ��� ��尡 �̹� �ٸ� ť�� ���� �� nullptr�� �� �� �ִ�. �̰��� �����Ϸ��� �ǵ�
            if (pLocalHeadNextReal == nullptr)
            {
                continue;
            }

            //���� ���� ��Ȳ�̶�� �� ĭ �о��ش�.
            if (localHeadMeta == localTailMeta)
            {
                InterlockedCompareExchange(&_metaTail, localHeadNextMeta, localTailMeta);
                continue;
            }

            T retData = pLocalHeadNextReal->data;
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
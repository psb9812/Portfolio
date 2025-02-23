#pragma once
#include <Windows.h>
#include "LockFreeMemoryPool.h"
#include "MemoryLog.h"

template<typename T>
class LockFreeQueue
{
private:
    LONG _size;

    struct Node
    {
        T data;
        Node* pNext;

        Node()
            :data(NULL), pNext(nullptr)
        {}
    };

    Node* _pHead;
    Node* _pTail;

    LockFreeMemoryPool<Node> _memoryPool;

    MemoryLog _log;

    // ABA ������ �ذ��ϱ� ���� �ִ� ��Ÿ �ּ� ID ��
    UINT _addressId = 0;

    //���� ��Ʈ�� ���� �� �ִ� MetaID���� �ִ밪
    const int MAX_ADDRID = 131072;

    //�޸� �ּҿ��� ������ �ʴ� ���� 17��Ʈ
    const int SPARE_BIT = 17;

    //�޸� �ּҿ��� ���Ǵ� 47��Ʈ
    const int VALID_BIT = 47;

public:
    LockFreeQueue()
        :_memoryPool(0, false), _size(0)
    {
        //���� ��� ����
        _pHead = _memoryPool.Alloc();
        _pHead->pNext = nullptr;
        _pTail = _pHead;
    }

    void Enqueue(T inputData)
    {
        Node* pNewNodeReal = _memoryPool.Alloc();
        pNewNodeReal->data = inputData;
        pNewNodeReal->pNext = nullptr;
        UINT64 addrID = GetAddressID();
        Node* pNewNodeMeta = (Node*)InsertMetaDataToAddress(pNewNodeReal, addrID);

        while (true)
        {
            Node* pLocalTailMeta = _pTail;
            Node* pLocalTailReal = (Node*)DeleteMetaDataToAddress(pLocalTailMeta);
            Node* pLocalTailNextMeta = pLocalTailReal->pNext;
            Node* pLocalTailNextReal = (Node*)DeleteMetaDataToAddress(pLocalTailNextMeta);
            if (pLocalTailNextReal != nullptr)
            {
                if (InterlockedCompareExchangePointer((PVOID*)&_pTail, pLocalTailNextMeta, pLocalTailMeta) == pLocalTailMeta)
                {
                    LONG localSize = InterlockedIncrement(&_size);
                    {
                        UINT64 ret = _log.WriteMemoryLog(Operation::Enqueue_Tail_manipulrate, pNewNodeMeta, nullptr,
                            pLocalTailMeta, pLocalTailNextMeta, nullptr, _size);
                    }
                }
            }
            else
            {
                if (InterlockedCompareExchangePointer((PVOID*)&pLocalTailReal->pNext, pNewNodeMeta, nullptr) == nullptr)
                {
                    {
                        UINT64 ret = _log.WriteMemoryLog(Operation::Enqueue_CAS1, pNewNodeMeta, nullptr,
                        pLocalTailMeta, pLocalTailNextMeta, nullptr, _size);
                    }
                    Node* realTail = nullptr;
                    if ((realTail = (Node*)InterlockedCompareExchangePointer((PVOID*)&_pTail, pNewNodeMeta, pLocalTailMeta)) != pLocalTailMeta)
                    {
                        {
                            UINT64 ret = _log.WriteMemoryLog(Operation::Enqueue_CAS2_fail, pNewNodeMeta, nullptr,
                                pLocalTailMeta, pLocalTailNextMeta, realTail, _size);
                        }
                        //2�� CAS�� �����ϴ� ���� : pLocalTail�� �̹� ������ ��带 �����װ� ��� ���� �� �ٸ� �ʿ��� �۾��� �ʹٴ� �Ͼ��
                        //pLocalTail�� ���� �޸� �ּҰ� ���� �κ��� �����Ǿ� _pTail�� ������ �Ǹ�(���� _pTail�� ���� �ʰ� �׳� ���� �Ǿ��� ��Ȳ�� ���� ����)
                        //next�� nullptr�� �����Ƿ� 1�� CAS�� ��������� 2�� CAS�� Meta�ּҸ� ���ϱ� ������ �����Ѵ�.
                    }
                    else
                    {
                        LONG localSize = InterlockedIncrement(&_size);
                        {
                            UINT64 ret = _log.WriteMemoryLog(Operation::Enqueue_CAS2_Success, pNewNodeMeta, nullptr,
                                pLocalTailMeta, pLocalTailNextMeta, realTail, _size);
                        }
                    }
                    break;
                }
            }
        }
    }

    int Dequeue(T& outParam)
    {
        //����� �����ؼ� �ϴ� ������.
        LONG localSize = InterlockedDecrement(&_size);
        if (localSize < 0)
        {
            InterlockedIncrement(&_size);
            //__debugbreak();
            return -1;
        }

        while (true)
        {
            Node* pLocalHeadMeta = _pHead;
            Node* pLocalHeadReal = (Node*)DeleteMetaDataToAddress(pLocalHeadMeta);

            //pLocalHead->next�� nullptr�� �Ǵ� ������ ã��.
            //���� : pLocalHeadNextMeta�� ���� ���� pLocalHeadReal�� ����Ű�� ��尡 localHead�� ����Ű�� ���� �ٸ� ����� �� �ִ�.
            Node* pLocalHeadNextMeta = pLocalHeadReal->pNext;
            Node* pLocalHeadNextReal = (Node*)DeleteMetaDataToAddress(pLocalHeadNextMeta);

            if (pLocalHeadNextReal == nullptr)
            {
                UINT64 ret = _log.WriteMemoryLog(Operation::Dequeue, pLocalHeadNextMeta,
                    (pLocalHeadNextReal == nullptr) ? (void*)0x123123 : pLocalHeadNextReal->pNext,
                    pLocalHeadMeta, pLocalHeadNextMeta, nullptr, _size);
                //__debugbreak();
            }
            else
            {
                if (InterlockedCompareExchangePointer((PVOID*)&_pHead, pLocalHeadNextMeta, pLocalHeadMeta) == pLocalHeadMeta)
                {
                    UINT64 ret = _log.WriteMemoryLog(Operation::Dequeue, pLocalHeadNextMeta, pLocalHeadNextReal->pNext,
                        pLocalHeadMeta, pLocalHeadNextMeta, nullptr, _size);
                    outParam = pLocalHeadNextReal->data;
                    _memoryPool.Free(pLocalHeadReal);
                    break;
                }
            }
        }
        return 0;
    }

    LONG GetSize() const
    {
        return _size;
    }


private:
    //////////////////////////////////////////////////////////////////////////
    // �־��� �޸� �ּ��� ���� 17��Ʈ�� MetaData ID���� �����Ѵ�.
    //
    // Parameters: (void*) �޸� �ּ�, (UINT64) MetaData ID
    // Return: (void*) MetaData ID�� ���Ե� �޸� �ּ�
    //////////////////////////////////////////////////////////////////////////
    inline void* InsertMetaDataToAddress(void* const address, UINT64 addressID)
    {
        UINT64 addr = reinterpret_cast<UINT64>(address);
        addressID <<= VALID_BIT;
        addr |= addressID;

        return reinterpret_cast<void*>(addr);
    }

    //////////////////////////////////////////////////////////////////////////
    // �־��� �޸� �ּҿ��� ���� 17��Ʈ�� ���� �����.
    //
    // Parameters: (void*) �޸� �ּ� (MetaAddress�� ����)
    // Return: (void*) ���� 17��Ʈ�� 0�� �޸� �ּ�
    //////////////////////////////////////////////////////////////////////////
    inline void* DeleteMetaDataToAddress(void* address)
    {
        UINT64 addrTypeInt = reinterpret_cast<UINT64>(address);

        addrTypeInt <<= SPARE_BIT;
        addrTypeInt >>= SPARE_BIT;

        return reinterpret_cast<void*>(addrTypeInt);
    }

    //////////////////////////////////////////////////////////////////////////
    // �ߺ����� �ʵ��� AddressID ���� �߱޹޴´�.
    //
    // Parameters: ����.
    // Return: (UINT64) Address ID
    //////////////////////////////////////////////////////////////////////////
    inline UINT64 GetAddressID()
    {
        return static_cast<UINT64>(InterlockedIncrement(&_addressId) % MAX_ADDRID);
    }
};
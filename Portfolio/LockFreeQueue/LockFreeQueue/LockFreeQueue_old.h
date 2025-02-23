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

    // ABA 문제를 해결하기 위해 넣는 메타 주소 ID 값
    UINT _addressId = 0;

    //여유 비트에 넣을 수 있는 MetaID값의 최대값
    const int MAX_ADDRID = 131072;

    //메모리 주소에서 사용되지 않는 상위 17비트
    const int SPARE_BIT = 17;

    //메모리 주소에서 사용되는 47비트
    const int VALID_BIT = 47;

public:
    LockFreeQueue()
        :_memoryPool(0, false), _size(0)
    {
        //더미 노드 생성
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

            if (pLocalTailReal == pLocalTailNextReal)
            {
                __debugbreak();
            }

            //_pTail의 next가 nullptr이 아니라면 누군가 2번 CAS에서 실패한 것이다. 그렇다면 다음 Enque에서 tail을 한 번 밀어서
            //정상적인 Enqueue가 가능하도록 _pTail을 _pTail의 pNext값으로 밀어준다.
            //한 번만 밀어줘도 되는 이유는 _pTail의 pNext가 nullptr이 아닌 시점에서 _pTail의 pNext의 뒤에 노드가 붙게 될 일은 1번 CAS로 인해 발생하지 않기 때문이다.
            if (pLocalTailNextReal != nullptr)
            {
                if (InterlockedCompareExchangePointer((PVOID*)&_pTail, pLocalTailNextMeta, pLocalTailMeta) == pLocalTailMeta)
                {
                    LONG localSize = InterlockedIncrement(&_size);
                    UINT64 ret = _log.WriteMemoryLog(Operation::Enqueue_Tail_manipulrate, pNewNodeMeta, nullptr,
                        pLocalTailMeta, pLocalTailNextMeta, nullptr, _size);

                }
            }
            else
            {
                if (InterlockedCompareExchangePointer((PVOID*)&pLocalTailReal->pNext, pNewNodeMeta, nullptr) == nullptr)
                {
                    UINT64 ret = _log.WriteMemoryLog(Operation::Enqueue_CAS1, pNewNodeMeta, nullptr,
                        pLocalTailMeta, pLocalTailNextMeta, nullptr, _size);

                    Node* realTail = nullptr;
                    if ((realTail = (Node*)InterlockedCompareExchangePointer((PVOID*)&_pTail, pNewNodeMeta, pLocalTailMeta)) != pLocalTailMeta)    //<< 실패의 경우 그 이유 추적
                    {
                        UINT64 ret = _log.WriteMemoryLog(Operation::Enqueue_CAS2_fail, pNewNodeMeta, nullptr,
                            pLocalTailMeta, pLocalTailNextMeta, realTail, _size);

                        //__debugbreak();
                        //2번 CAS가 실패하는 이유 : pLocalTail이 이미 삭제된 노드를 가리켰고 잠시 멈춘 후 다른 쪽에서 작업이 와다다 일어나서
                        //pLocalTail과 실제 메모리 주소가 같은 부분이 생성되어 _pTail로 설정이 되면(아직 _pTail이 되지 않고 그냥 생성 되어진 상황일 수도 있음)
                        //next가 nullptr이 맞으므로 1번 CAS는 통과하지만 2번 CAS는 Meta주소를 비교하기 때문에 실패한다.
                    }
                    else
                    {
                        LONG localSize = InterlockedIncrement(&_size);
                        UINT64 ret = _log.WriteMemoryLog(Operation::Enqueue_CAS2_Success, pNewNodeMeta, nullptr,
                            pLocalTailMeta, pLocalTailNextMeta, realTail, _size);
                    }
                    break;
                }
            }
        }

    }

    int Dequeue(T& outParam)
    {
        //사이즈에 의존해서 일단 비교하자.
        LONG localSize = InterlockedDecrement(&_size);
        if (localSize < 0)
        {
            InterlockedIncrement(&_size);
            //__debugbreak();
            return -1;
        }

        //_pHead->next == nullptr 이면 비어있다고 판단하고 리턴



        while (true)
        {
            Node* pLocalHeadMeta = _pHead;
            Node* pLocalHeadReal = (Node*)DeleteMetaDataToAddress(pLocalHeadMeta);

            //pLocalHead->next가 nullptr이 되는 이유를 찾자.
            //이유 : pLocalHeadNextMeta를 구할 시점 pLocalHeadReal이 가리키는 노드가 localHead가 가리키는 노드와 다른 노드일 수 있다.
            Node* pLocalHeadNextMeta = pLocalHeadReal->pNext;
            Node* pLocalHeadNextReal = (Node*)DeleteMetaDataToAddress(pLocalHeadNextMeta);

            if (pLocalHeadNextReal == nullptr)
            {
                UINT64 ret = _log.WriteMemoryLog(Operation::Dequeue, pLocalHeadNextMeta,
                    (pLocalHeadNextReal == nullptr) ? (void*)0x123123 : pLocalHeadNextReal->pNext,
                    pLocalHeadMeta, pLocalHeadNextMeta, nullptr, _size);
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
    // 주어진 메모리 주소의 상위 17비트에 MetaData ID값을 삽입한다.
    //
    // Parameters: (void*) 메모리 주소, (UINT64) MetaData ID
    // Return: (void*) MetaData ID가 삽입된 메모리 주소
    //////////////////////////////////////////////////////////////////////////
    inline void* InsertMetaDataToAddress(void* const address, UINT64 addressID)
    {
        UINT64 addr = reinterpret_cast<UINT64>(address);
        addressID <<= VALID_BIT;
        addr |= addressID;

        return reinterpret_cast<void*>(addr);
    }

    //////////////////////////////////////////////////////////////////////////
    // 주어진 메모리 주소에서 상위 17비트의 값을 지운다.
    //
    // Parameters: (void*) 메모리 주소 (MetaAddress로 가정)
    // Return: (void*) 상위 17비트가 0인 메모리 주소
    //////////////////////////////////////////////////////////////////////////
    inline void* DeleteMetaDataToAddress(void* address)
    {
        UINT64 addrTypeInt = reinterpret_cast<UINT64>(address);

        addrTypeInt <<= SPARE_BIT;
        addrTypeInt >>= SPARE_BIT;

        return reinterpret_cast<void*>(addrTypeInt);
    }

    //////////////////////////////////////////////////////////////////////////
    // 중복되지 않도록 AddressID 값을 발급받는다.
    //
    // Parameters: 없다.
    // Return: (UINT64) Address ID
    //////////////////////////////////////////////////////////////////////////
    inline UINT64 GetAddressID()
    {
        return static_cast<UINT64>(InterlockedIncrement(&_addressId) % MAX_ADDRID);
    }
};
#pragma once
#include <Windows.h>
#include <optional>
#include "TLSMemoryPool.h"
#include "AddressConverter.h"

/*
* # LockFreeQueue
* @ 구현 세부 사항
*   - ABA 문제를 해결하기 위해 TLSMemoryPool에서 노드를 할당 받는다.
* 
*   - TLSMemoryPool에서 LockFreeQueue를 사용하기 위해서 QID로 여러 Queue 간의 노드를 식별할 수 있게 한다.
*     그렇게 해서 CAS를 진행할 때 QID도 검사해서 다른 Queue의 노드로 CAS를 성공해 버려서 큐가 깨지는 현상을 막는다.
* 
*   - (Meta 주소 + QID) 정보를 CAS할 때 한 번에 비교하기 위해서 Address128 구조체로 DoubleCAS를 수행한다.
*/

template<typename T>
class LockFreeQueue
{
private:
    struct alignas(16) Address128
    {
        //QID : QID(8byte) / 하위 64비트
        //metaAddress : Meta ID(상위 17비트) + 실제 주소(하위 47비트) / 상위 64비트
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

    //같은 락 프리 큐 인스턴스들이 공용으로 사용하는 공용 노드 풀
    static inline TLSMemoryPool<Node> _nodePool = TLSMemoryPool<Node>(0, true);

    //같은 템플릿 타입의 락 프리 큐 인스턴스를 식별하기 위해 식별자 만드는 Count
    static inline LONG _QIDCount = 0;

    //해당 인스턴스의 QID
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
        //더미 노드 아이디 발급
        UINT64 addrID = AddressConverter::GetAddressID(&_addrIDCount);
        //더미 노드 생성
        Node* pDummy = _nodePool.Alloc(_QID, addrID);
        //헤드, 테일 할당
        _metaHead = _metaTail = AddressConverter::InsertMetaIDToAddress((UINT_PTR)pDummy, addrID);
    }

    std::optional<LONG> Enqueue(T inputData)
    {
        //새로운 addressID를 발급받기 -> 노드 할당(QID -> addressID를 실제 메모리 주소 상위 17비트에 삽입 
        UINT64 addrID = AddressConverter::GetAddressID(&_addrIDCount);
        Node* pNewNode = _nodePool.Alloc(inputData, _QID, addrID);
        UINT_PTR newNodeMeta = AddressConverter::InsertMetaIDToAddress((UINT_PTR)pNewNode, addrID);

        while (true)
        {
            //최대 사이즈 이상으로는 삽입 불가
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

            //다음의 노드가 localTail의 next가 nullptr이 아니라면 능동적으로 한 칸 밀어주고 다시 반복문 돌기
            if (pLocalTailNextReal != nullptr)
            {
                InterlockedCompareExchange(&_metaTail, localTailNextMeta, localTailMeta);
                continue;
            }

            if (AddressConverter::ExtractMetaID(localTailMeta) != AddressConverter::ExtractMetaID(localTailNextMeta))
            {
                continue;
            }

            //CAS1 (공용 풀을 사용할 때 다른 큐의 노드가 되어버린 지역 tail의 경우를 감지하기 위해 Double CAS 사용한다.)
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
        //비어있는지 체크
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

            //head의 next가 nullptr이라면 다시 head값을 갱신시킨다.
            //공용 풀 문제에서 지역에서 가지고 있는 헤드 노드가 이미 다른 큐에 들어갔을 때 nullptr이 될 수 있다. 이것을 방지하려는 의도
            if (pLocalHeadNextReal == nullptr)
            {
                continue;
            }

            //둘이 같은 상황이라면 한 칸 밀어준다.
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
#include <Windows.h>
#include <iostream>

struct alignas(16) Address128
{
    alignas(16) INT64 address[2];

    Address128()
    {
        address[0] = 0;
        address[1] = 0;
    }
    Address128(int qid, int meta, INT_PTR addr) 
    {
        address[0] = ((INT64)qid << 32) | (INT64)meta;
        address[1] = addr;
    }

    int GetQID() const
    {
        return address[0] >> 32 & 0x00000000FFFFFFFF;
    }

    int GetMetaID()
    {
        return address[0] & 0x00000000FFFFFFFF;
    }

    UINT_PTR GetAddress()
    {
        return address[1];
    }

};

bool AtomicCopy128(Address128* dest, Address128* src)
{
    Address128 localDest = *dest;

    // dest의 address 포인터에 대해 InterlockedCompareExchange128 호출
    return InterlockedCompareExchange128(
        reinterpret_cast<LONG64*>(dest),
        src->address[1],
        src->address[0],
        reinterpret_cast<LONG64*>(&localDest)
    );
}

int main()
{
    Address128 a(1, 2, 100);
    Address128 b(0, 0, 0);

    // AtomicCopy128 호출
    bool success = AtomicCopy128(&b, &a);

    // 결과 출력
    std::cout << (success ? "Copy succeeded: " : "Copy failed: ")
        << b.GetQID() << " " << b.GetMetaID() << " " << b.GetAddress() << std::endl;

    return 0;
}

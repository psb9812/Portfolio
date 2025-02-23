#pragma once
#include <Windows.h>

class AddressConverter
{
public:
	AddressConverter() = default;
	~AddressConverter() = default;


	//////////////////////////////////////////////////////////////////////////
	// 먼 미래에 여유 비트의 수가 변경될 때 감지하기 위한 코드
	// 현재 코드 작성 시점의 여유 비트 수는 17비트이다.
	//
	// Parameters: 없음
	// Return: 안 쓰는 비트 수가 여전히 17비트인지 여부
	//////////////////////////////////////////////////////////////////////////
	static bool CheckSpareBitNum()
	{
		SYSTEM_INFO si;
		GetSystemInfo(&si);

		UINT64 maxAddr = ((UINT64)si.lpMaximumApplicationAddress + (UINT64)si.lpMinimumApplicationAddress) & ~0;
		maxAddr >>= VALID_BIT;
		if (maxAddr != 0)
		{
			return false;
		}
		else
		{
			return true;
		}
	}

	//////////////////////////////////////////////////////////////////////////
	// 주어진 메모리 주소의 상위 17비트에 MetaData ID값을 삽입한다.
	//
	// Parameters: (UINT_PTR) 메모리 주소, (UINT64) MetaData ID
	// Return: (UINT_PTR) MetaData ID가 삽입된 메모리 주소
	//////////////////////////////////////////////////////////////////////////
	static inline UINT_PTR InsertAddressIDToAddress(UINT_PTR realAddr, UINT64 addressID)
	{
		return (addressID << VALID_BIT) | realAddr;
	}

	//////////////////////////////////////////////////////////////////////////
	// 주어진 메모리 주소에서 상위 17비트의 값을 지운다.
	//
	// Parameters: (UINT_PTR) 메모리 주소 (MetaAddress로 가정)
	// Return: (UINT_PTR) 상위 17비트가 0인 메모리 주소
	//////////////////////////////////////////////////////////////////////////
	static inline UINT_PTR DeleteAddressIDFromAddress(UINT_PTR metaAddr)
	{
		return metaAddr & REAL_ADDR_MASK;
	}

	static inline UINT_PTR ExtractMetaID(UINT_PTR metaAddr)
	{
		return metaAddr & ~REAL_ADDR_MASK;
	}

	//////////////////////////////////////////////////////////////////////////
	// 중복되지 않도록 AddressID 값을 발급받는다.
	//
	// Parameters: (UINT64*) 각 락프리 자료구조가 관리하는 addressID 카운트 값
	// Return: (UINT64) AddressID
	//////////////////////////////////////////////////////////////////////////
	static inline UINT64 GetAddressID(UINT64* addressID)
	{
		return (InterlockedIncrement(addressID) - 1) % MAX_META_ID;
	}

private:
	//여유 비트에 넣을 수 있는 MetaAddress값의 최대값
	static constexpr int MAX_META_ID = 131072;

	//메모리 주소에서 사용되지 않는 상위 비트 수
	static constexpr int SPARE_BIT = 17;

	//메모리 주소에서 사용되는 비트 수
	static constexpr int VALID_BIT = 64 - SPARE_BIT;

	//실제 메모리 주소 구할 때 사용되는 마스크 값
	static constexpr UINT_PTR REAL_ADDR_MASK = 0x00007FFFFFFFFFFF;
};
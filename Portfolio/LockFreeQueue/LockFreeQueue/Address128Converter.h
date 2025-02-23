#pragma once
#include <Windows.h>

class Address128Converter
{
public:
	Address128Converter() = default;
	~Address128Converter() = default;

	////////////////////////////////////////////////////////////////////////////
	//// 주어진 메모리 주소의 상위 17비트에 MetaData ID값을 삽입한다.
	////
	//// Parameters: (UINT_PTR) 메모리 주소, (UINT64) MetaData ID
	//// Return: (UINT_PTR) MetaData ID가 삽입된 메모리 주소
	////////////////////////////////////////////////////////////////////////////
	//static inline UINT_PTR InsertMetaIDToAddress(UINT_PTR realAddr, UINT64 addressID)
	//{
	//	return (addressID << VALID_BIT) | realAddr;
	//}

	////////////////////////////////////////////////////////////////////////////
	//// 주어진 메모리 주소에서 상위 17비트의 값을 지운다.
	////
	//// Parameters: (UINT_PTR) 메모리 주소 (MetaAddress로 가정)
	//// Return: (UINT_PTR) 상위 17비트가 0인 메모리 주소
	////////////////////////////////////////////////////////////////////////////
	//static inline UINT_PTR DeleteMetaIDToAddress(UINT_PTR metaAddr)
	//{
	//	return metaAddr & REAL_ADDR_MASK;
	//}

	//static inline UINT_PTR ExtractMetaID(UINT_PTR metaAddr)
	//{
	//	return metaAddr & ~REAL_ADDR_MASK;
	//}

	//////////////////////////////////////////////////////////////////////////
	// 중복되지 않도록 AddressID 값을 발급받는다.
	//
	// Parameters: (UINT64*) 각 락프리 자료구조가 관리하는 addressID 카운트 값
	// Return: (UINT64) AddressID
	//////////////////////////////////////////////////////////////////////////
	//static inline int GetMetaID(UINT64* addressID)
	//{
	//	return InterlockedIncrement(addressID) - 1;
	//}

private:
};
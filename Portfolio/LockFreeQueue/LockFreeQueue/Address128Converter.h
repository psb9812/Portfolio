#pragma once
#include <Windows.h>

class Address128Converter
{
public:
	Address128Converter() = default;
	~Address128Converter() = default;

	////////////////////////////////////////////////////////////////////////////
	//// �־��� �޸� �ּ��� ���� 17��Ʈ�� MetaData ID���� �����Ѵ�.
	////
	//// Parameters: (UINT_PTR) �޸� �ּ�, (UINT64) MetaData ID
	//// Return: (UINT_PTR) MetaData ID�� ���Ե� �޸� �ּ�
	////////////////////////////////////////////////////////////////////////////
	//static inline UINT_PTR InsertMetaIDToAddress(UINT_PTR realAddr, UINT64 addressID)
	//{
	//	return (addressID << VALID_BIT) | realAddr;
	//}

	////////////////////////////////////////////////////////////////////////////
	//// �־��� �޸� �ּҿ��� ���� 17��Ʈ�� ���� �����.
	////
	//// Parameters: (UINT_PTR) �޸� �ּ� (MetaAddress�� ����)
	//// Return: (UINT_PTR) ���� 17��Ʈ�� 0�� �޸� �ּ�
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
	// �ߺ����� �ʵ��� AddressID ���� �߱޹޴´�.
	//
	// Parameters: (UINT64*) �� ������ �ڷᱸ���� �����ϴ� addressID ī��Ʈ ��
	// Return: (UINT64) AddressID
	//////////////////////////////////////////////////////////////////////////
	//static inline int GetMetaID(UINT64* addressID)
	//{
	//	return InterlockedIncrement(addressID) - 1;
	//}

private:
};
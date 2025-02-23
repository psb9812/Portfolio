#pragma once
#include <Windows.h>

class AddressConverter
{
public:
	AddressConverter() = default;
	~AddressConverter() = default;


	//////////////////////////////////////////////////////////////////////////
	// �� �̷��� ���� ��Ʈ�� ���� ����� �� �����ϱ� ���� �ڵ�
	// ���� �ڵ� �ۼ� ������ ���� ��Ʈ ���� 17��Ʈ�̴�.
	//
	// Parameters: ����
	// Return: �� ���� ��Ʈ ���� ������ 17��Ʈ���� ����
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
	// �־��� �޸� �ּ��� ���� 17��Ʈ�� MetaData ID���� �����Ѵ�.
	//
	// Parameters: (UINT_PTR) �޸� �ּ�, (UINT64) MetaData ID
	// Return: (UINT_PTR) MetaData ID�� ���Ե� �޸� �ּ�
	//////////////////////////////////////////////////////////////////////////
	static inline UINT_PTR InsertAddressIDToAddress(UINT_PTR realAddr, UINT64 addressID)
	{
		return (addressID << VALID_BIT) | realAddr;
	}

	//////////////////////////////////////////////////////////////////////////
	// �־��� �޸� �ּҿ��� ���� 17��Ʈ�� ���� �����.
	//
	// Parameters: (UINT_PTR) �޸� �ּ� (MetaAddress�� ����)
	// Return: (UINT_PTR) ���� 17��Ʈ�� 0�� �޸� �ּ�
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
	// �ߺ����� �ʵ��� AddressID ���� �߱޹޴´�.
	//
	// Parameters: (UINT64*) �� ������ �ڷᱸ���� �����ϴ� addressID ī��Ʈ ��
	// Return: (UINT64) AddressID
	//////////////////////////////////////////////////////////////////////////
	static inline UINT64 GetAddressID(UINT64* addressID)
	{
		return (InterlockedIncrement(addressID) - 1) % MAX_META_ID;
	}

private:
	//���� ��Ʈ�� ���� �� �ִ� MetaAddress���� �ִ밪
	static constexpr int MAX_META_ID = 131072;

	//�޸� �ּҿ��� ������ �ʴ� ���� ��Ʈ ��
	static constexpr int SPARE_BIT = 17;

	//�޸� �ּҿ��� ���Ǵ� ��Ʈ ��
	static constexpr int VALID_BIT = 64 - SPARE_BIT;

	//���� �޸� �ּ� ���� �� ���Ǵ� ����ũ ��
	static constexpr UINT_PTR REAL_ADDR_MASK = 0x00007FFFFFFFFFFF;
};
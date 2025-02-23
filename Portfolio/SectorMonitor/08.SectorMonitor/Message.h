#pragma once
#include "TLSMemoryPool.h"
#include "NetworkProtocol.h"
#include <Windows.h>
#include <type_traits>


class Message
{
	//�����ڸ� private�� �ּ� �����̳� ��������� �������� ���ϰ� �����.
	//������ �ݵ�� static ��� ������ _messagePool�� ���� �����Ѵ�.
private:
	Message();
	Message(int bufferSize);
public:
	enum en_Message
	{
		BUFFER_DEFAULT_SIZE = 1400,		// ��Ŷ�� �⺻ ���� ������.
		BUFFER_MAX_SIZE = 10000,		// ����ȭ ������ �ִ� ������
		HEADER_SIZE = 5,				// ��� ������
	};


	~Message();

	void	Clear(void);

	//////////////////////////////////////////////////////////////////////////
	// ���� ũ�� ��������
	//
	// Parameters: (int) �����Ϸ��� ������.
	// Return: bool. �ִ� ����� �ѱ�� �����Ѵ�.
	//////////////////////////////////////////////////////////////////////////
	bool Resize(int size);


	__forceinline int	GetBufferSize() { return _bufferSize; }

	__forceinline int	GetHeaderSize() { return HEADER_SIZE; }

	__forceinline int	GetDataSize() { return _dataSize; }

	__forceinline int GetBufferFreeSize() { return static_cast<int>(_pEnd - _pWritePos); }

	__forceinline bool GetIsEncode() const { return _isEncode; }


	//////////////////////////////////////////////////////////////////////////
	// ��� ������ ���.
	// 
	// Parameters: ����.
	// Return: (char *)��� ������.
	//////////////////////////////////////////////////////////////////////////
public:
	char* GetHeaderPtr(void) { return _pBuffer; }

public:

	//////////////////////////////////////////////////////////////////////////
	// ���� ������ ���.
	//
	// Parameters: ����.
	// Return: (char *)���� ������.
	//////////////////////////////////////////////////////////////////////////
	__forceinline char* GetBufferPtr(void) { return _pBegin; }

	//////////////////////////////////////////////////////////////////////////
	// ���� Pos �̵�. (�����̵��� �ȵ�)
	// GetBufferPtr �Լ��� �̿��Ͽ� �ܺο��� ������ ���� ������ ������ ��� ���. 
	//
	// Parameters: (int) �̵� ������.
	// Return: (int) �̵��� ������.
	//////////////////////////////////////////////////////////////////////////
	int		MoveWritePos(int iSize);
	int		MoveReadPos(int iSize);

	/* ============================================================================= */
	// ������ �����ε�
	/* ============================================================================= */
	Message& operator = (Message& clSrMessage);

	//////////////////////////////////////////////////////////////////////////
	// �ֱ�.
	//////////////////////////////////////////////////////////////////////////

	template<typename T>
	__forceinline Message& operator << (T value)
	{
		static_assert(std::is_integral<T>::value || std::is_floating_point<T>::value);

		int valueSize = sizeof(T);
		if (GetBufferFreeSize() < valueSize)
		{
			//��������
			if (!Resize(_bufferSize * 2))
			{
				//SystemLog::GetInstance()->WriteLog(L"SYSTEM", SystemLog::LOG_LEVEL_SYSTEM, L"SerializeBuffer ReSize Fail size : %d", _bufferSize * 2);
			}
		}

		auto castPointer = (T*)_pWritePos;
		*castPointer = value;

		_pWritePos += valueSize;
		_dataSize += valueSize;
		return *this;
	}


	//////////////////////////////////////////////////////////////////////////
	// ����.
	//////////////////////////////////////////////////////////////////////////
	template<typename T>
	__forceinline Message& operator >> (T& value)
	{
		static_assert(std::is_integral<T>::value || std::is_floating_point<T>::value);

		int valueSize = sizeof(T);
		if (_dataSize < valueSize)
		{
			//���� �� ���µ� �������� �õ��ϸ� �ٷ� ���ܸ� ������ ������ �ľ��Ѵ�.
			//�������� ��� �������� �� �ٵ� �����Ͱ� ���ٸ� ���α׷����� �Ǽ��̹Ƿ� ������ �ľ��ϱ� �����̴�.
			//SystemLog::GetInstance()->WriteLog(L"ERROR", SystemLog::LOG_LEVEL_ERROR,
				//L"Impossible extraction attempt of serialization buffer"
			//);
			throw 0;
		}

		auto castPointer = (T*)_pReadPos;
		value = *castPointer;

		_pReadPos += valueSize;
		_dataSize -= valueSize;
		return *this;
	}


	//////////////////////////////////////////////////////////////////////////
	// ����Ÿ ���.
	//
	// Parameters: (char *)Dest ������. (int)Size.
	// Return: (int)������ ������.
	//////////////////////////////////////////////////////////////////////////
	__forceinline int GetData(char* pDest, int requestSize)
	{
		if (_dataSize < requestSize)
			return 0;

		memcpy_s(pDest, requestSize, _pReadPos, requestSize);
		_pReadPos += requestSize;
		_dataSize -= requestSize;
		return requestSize;
	}


	__forceinline int PutData(char* pSrc, int srcSize)
	{
		int spaceOfWrite = GetBufferFreeSize();

		if (srcSize > spaceOfWrite)
		{
			//��������
			if (!Resize(_bufferSize * 2))
			{
				//SystemLog::GetInstance()->WriteLog(L"SYSTEM", SystemLog::LOG_LEVEL_SYSTEM, L"SerializeBuffer ReSize Fail / Size attempted to change : %d", _bufferSize * 2);
			}
		}
		memcpy_s(_pWritePos, spaceOfWrite, pSrc, srcSize);

		_pWritePos += srcSize;
		_dataSize += srcSize;
		return srcSize;
	}

	LONG AddRefCount();

	//���۷��� ī��Ʈ ���� �Լ� + refCount�� 0�� �Ǹ� ����ȭ ���� Ǯ�� ��ȯ�Ѵ�.
	LONG SubRefCount();

	//��Ŷ�� ���̷ε� ������ ���� üũ�� ���ϴ� �Լ�
	unsigned char GetCheckSum();

	//��Ŷ�� üũ���� ���̷ε带 ���ڵ��ϴ� �Լ�
	void Encode(int staticKey);

	//��Ŷ�� üũ���� ���̷ε带 ���ڵ��ϴ� �Լ�
	//��ȯ���� ��ȣȭ ���� üũ�� �˻� ����̴�.
	bool Decode(unsigned char randKey, int staticKey);

	void SetIsEncode(bool value) { _isEncode = value; }

	//����� ������ �� �ִ� �Լ�
	template<HeaderType headerType>
	void WriteHeader(unsigned char packetCode);

protected:
	char* _pBuffer;									//��� ���� ���� ���� ������
	char* _pBegin;									//��� ������ ���� ���� ������
	char* _pEnd;
	char* _pWritePos;
	char* _pReadPos;

	int	_bufferSize;
	int	_dataSize;
	static TLSMemoryPool<Message> s_messagePool;	//����ȭ ���� Ǯ

	LONG _refCount;									//���۷��� ī��Ʈ
	bool _isEncode;									//���ڵ� ����

public:

	static Message* Alloc();
	static void Free(Message* pMessage);
	static int GetMemoryPoolCapacity();
	static int GetMemoryPoolUseSize();

	friend class TLSMemoryPool<Message>;
};

template<HeaderType headerType>
inline void Message::WriteHeader(unsigned char packetCode)
{
	if constexpr (headerType == Net)
	{
		NetHeader* pHeader = (NetHeader*)_pBuffer;
		pHeader->code = packetCode;
		pHeader->len = GetDataSize();
		pHeader->randKey = rand();
		pHeader->checkSum = GetCheckSum();
	}
	else
	{
		//��� ũ��� 5byte �����̹Ƿ� LanHeader�� �� ���� 3byte �տ��� ���ش�.
		LanHeader* pHeader = (LanHeader*)(_pBuffer + 3);
		pHeader->len = GetDataSize();
	}
}

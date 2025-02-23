#pragma once
#include "../NetServer.h" //������ �Լ� ���� ���� ����
#include "../PacketProtocol.h"
#include "SystemLog.h"
#include "TLSMemoryPool.h"
#include <Windows.h>


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
		BUFFER_DEFAULT_SIZE = 85,		// ��Ŷ�� �⺻ ���� ������.
		BUFFER_MAX_SIZE = 10000,		// ����ȭ ������ �ִ� ������
		NET_HEADER_LEN = 5,			
		LAN_HEADER_LEN = 2,
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


	inline int	GetBufferSize() const{ return _bufferSize; }

	template<HeaderType headerType>
	inline int	GetHeaderSize() const;

	inline int	GetDataSize() const { return _payloadSize; }

	inline int GetBufferFreeSize() const { return _bufferSize - _writePos; }

	inline bool GetIsEncode() const { return _isEncode; }


private:
	//////////////////////////////////////////////////////////////////////////
	// ��� ������ ���.
	// ��Ʈ��ũ ���̺귯�������� ���� �����ϵ��� friend�� �������ش�.
	// Parameters: ����.
	// Return: (char *)��� ������.
	//////////////////////////////////////////////////////////////////////////
	char* GetHeaderPtr(void) { return _pBuffer; }
public:
	friend char* NetServer::GetHeaderPtr(Message* pMessage);
	friend char* Session::GetHeaderPtr(Message* pMessage);

public:

	//////////////////////////////////////////////////////////////////////////
	// ���� ������ ���.
	//
	// Parameters: ����.
	// Return: (char *)���� ������.
	//////////////////////////////////////////////////////////////////////////
	inline char* GetPayloadPtr(void) { return _pBuffer + NET_HEADER_LEN; }

	//////////////////////////////////////////////////////////////////////////
	// ���� Pos �̵�. (�����̵��� �ȵ�)
	// GetPayloadPtr �Լ��� �̿��Ͽ� �ܺο��� ������ ���� ������ ������ ��� ���. 
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
	inline Message& operator << (T value)
	{
		static_assert(std::is_integral<T>::value || std::is_floating_point<T>::value);

		int valueSize = sizeof(T);
		if (GetBufferFreeSize() < valueSize)
		{
			//��������
			if (!Resize(_bufferSize * 2))
			{
				SystemLog::GetInstance()->WriteLog(L"SYSTEM", SystemLog::LOG_LEVEL_SYSTEM, L"SerializeBuffer ReSize Fail size : %d", _bufferSize * 2);
			}
		}

		auto castPointer = (T*)(_pBuffer + _writePos);
		*castPointer = value;

		_writePos += valueSize;
		_payloadSize += valueSize;
		return *this;
	}


	//////////////////////////////////////////////////////////////////////////
	// ����.
	//////////////////////////////////////////////////////////////////////////
	template<typename T>
	inline Message& operator >> (T& value)
	{
		static_assert(std::is_integral<T>::value || std::is_floating_point<T>::value);

		int valueSize = sizeof(T);
		if (_payloadSize < valueSize)
		{
			//���� �� ���µ� �������� �õ��ϸ� �ٷ� ���ܸ� ������ ������ �ľ��Ѵ�.
			//�������� ��� �������� �� �ٵ� �����Ͱ� ���ٸ� ���α׷����� �Ǽ��̹Ƿ� ������ �ľ��ϱ� �����̴�.
			SystemLog::GetInstance()->WriteLog(L"ERROR", SystemLog::LOG_LEVEL_ERROR,
				L"Impossible extraction attempt of serialization buffer"
			);
			throw 0;
		}

		auto castPointer = (T*)(_pBuffer + _readPos);
		value = *castPointer;

		_readPos += valueSize;
		_payloadSize -= valueSize;
		return *this;
	}


	//////////////////////////////////////////////////////////////////////////
	// ����Ÿ ���.
	//
	// Parameters: (char *)Dest ������. (int)Size.
	// Return: (int)������ ������.
	//////////////////////////////////////////////////////////////////////////
	inline int GetData(char* pDest, int requestSize)
	{
		if (_payloadSize < requestSize)
			return 0;

		memcpy_s(pDest, requestSize, _pBuffer + _readPos, requestSize);
		_readPos += requestSize;
		_payloadSize -= requestSize;
		return requestSize;
	}


	inline int PutData(char* pSrc, int srcSize)
	{
		int spaceOfWrite = GetBufferFreeSize();

		if (srcSize > spaceOfWrite)
		{
			//��������
			if (!Resize(_bufferSize * 2))
			{
				SystemLog::GetInstance()->WriteLog(L"SYSTEM", SystemLog::LOG_LEVEL_SYSTEM, L"SerializeBuffer ReSize Fail / Size attempted to change : %d", _bufferSize * 2);
			}
		}
		memcpy_s(_pBuffer + _writePos, spaceOfWrite, pSrc, srcSize);

		_writePos += srcSize;
		_payloadSize += srcSize;
		return srcSize;
	}

	inline LONG AddRefCount() { return InterlockedIncrement(&_refCount); }
	//���۷��� ī��Ʈ ���� �Լ� + refCount�� 0�� �Ǹ� ����ȭ ���� Ǯ�� ��ȯ�Ѵ�.
	LONG SubRefCount();

	//��Ŷ�� üũ���� ���̷ε带 ���ڵ��ϴ� �Լ�
	void Encode(int staticKey);
	//��Ŷ�� üũ���� ���̷ε带 ���ڵ��ϴ� �Լ�
	bool Decode(unsigned char randKey, int staticKey);

	unsigned char GetCheckSum();

	inline void SetIsEncode(bool value) { _isEncode = value; }

	//����� ������ �� �ִ� �Լ�
	template<HeaderType headerType>
	void WriteHeader(unsigned char packetCode);

public:

	static Message* Alloc();
	static void Free(Message* pMessage);
	static int GetMemoryPoolCapacity();
	static int GetMemoryPoolUseSize();

	friend class TLSMemoryPool<Message>;
private:
	char* _pBuffer;									//��� ���� ���� ���� ������
	int _writePos;
	int _readPos;

	int	_bufferSize;
	int	_payloadSize;

	LONG _refCount;									//���۷��� ī��Ʈ
	bool _isEncode;									//���ڵ� ����

	static TLSMemoryPool<Message> s_messagePool;	//����ȭ ���� Ǯ
};

template<HeaderType headerType>
inline int Message::GetHeaderSize() const
{
	if constexpr (headerType == Net)
		return NET_HEADER_LEN;
	else
		return LAN_HEADER_LEN;
}

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

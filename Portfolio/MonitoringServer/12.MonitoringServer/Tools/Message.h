#pragma once
#include "../NetServer.h" //프렌즈 함수 쓰기 위해 존재
#include "../PacketProtocol.h"
#include "SystemLog.h"
#include "TLSMemoryPool.h"
#include <Windows.h>


class Message
{
	//생성자를 private로 둬서 지역이나 명시적으로 생성하지 못하게 만든다.
	//생성은 반드시 static 멤버 변수인 _messagePool을 통해 수행한다.
private:
	Message();
	Message(int bufferSize);
public:
	enum en_Message
	{
		BUFFER_DEFAULT_SIZE = 85,		// 패킷의 기본 버퍼 사이즈.
		BUFFER_MAX_SIZE = 10000,		// 직렬화 버퍼의 최대 사이즈
		NET_HEADER_LEN = 5,			
		LAN_HEADER_LEN = 2,
	};

	~Message();

	void	Clear(void);

	//////////////////////////////////////////////////////////////////////////
	// 버퍼 크기 리사이즈
	//
	// Parameters: (int) 변경하려는 사이즈.
	// Return: bool. 최대 사이즈를 넘기면 실패한다.
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
	// 헤더 포인터 얻기.
	// 네트워크 라이브러리에서만 접근 가능하도록 friend로 선언해준다.
	// Parameters: 없음.
	// Return: (char *)헤더 포인터.
	//////////////////////////////////////////////////////////////////////////
	char* GetHeaderPtr(void) { return _pBuffer; }
public:
	friend char* NetServer::GetHeaderPtr(Message* pMessage);
	friend char* Session::GetHeaderPtr(Message* pMessage);

public:

	//////////////////////////////////////////////////////////////////////////
	// 버퍼 포인터 얻기.
	//
	// Parameters: 없음.
	// Return: (char *)버퍼 포인터.
	//////////////////////////////////////////////////////////////////////////
	inline char* GetPayloadPtr(void) { return _pBuffer + NET_HEADER_LEN; }

	//////////////////////////////////////////////////////////////////////////
	// 버퍼 Pos 이동. (음수이동은 안됨)
	// GetPayloadPtr 함수를 이용하여 외부에서 강제로 버퍼 내용을 수정할 경우 사용. 
	//
	// Parameters: (int) 이동 사이즈.
	// Return: (int) 이동된 사이즈.
	//////////////////////////////////////////////////////////////////////////
	int		MoveWritePos(int iSize);
	int		MoveReadPos(int iSize);

	/* ============================================================================= */
	// 연산자 오버로딩
	/* ============================================================================= */
	Message& operator = (Message& clSrMessage);

	//////////////////////////////////////////////////////////////////////////
	// 넣기.
	//////////////////////////////////////////////////////////////////////////

	template<typename T>
	inline Message& operator << (T value)
	{
		static_assert(std::is_integral<T>::value || std::is_floating_point<T>::value);

		int valueSize = sizeof(T);
		if (GetBufferFreeSize() < valueSize)
		{
			//리사이즈
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
	// 빼기.
	//////////////////////////////////////////////////////////////////////////
	template<typename T>
	inline Message& operator >> (T& value)
	{
		static_assert(std::is_integral<T>::value || std::is_floating_point<T>::value);

		int valueSize = sizeof(T);
		if (_payloadSize < valueSize)
		{
			//꺼낼 수 없는데 꺼내려고 시도하면 바로 예외를 던져서 문제를 파악한다.
			//프로토콜 대로 꺼내려고 할 텐데 데이터가 없다면 프로그래머의 실수이므로 빠르게 파악하기 위함이다.
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
	// 데이타 얻기.
	//
	// Parameters: (char *)Dest 포인터. (int)Size.
	// Return: (int)복사한 사이즈.
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
			//리사이즈
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
	//레퍼런스 카운트 감소 함수 + refCount가 0이 되면 직렬화 버퍼 풀에 반환한다.
	LONG SubRefCount();

	//패킷의 체크섬과 페이로드를 인코드하는 함수
	void Encode(int staticKey);
	//패킷의 체크섬과 페이로드를 디코드하는 함수
	bool Decode(unsigned char randKey, int staticKey);

	unsigned char GetCheckSum();

	inline void SetIsEncode(bool value) { _isEncode = value; }

	//헤더에 적절한 값 넣는 함수
	template<HeaderType headerType>
	void WriteHeader(unsigned char packetCode);

public:

	static Message* Alloc();
	static void Free(Message* pMessage);
	static int GetMemoryPoolCapacity();
	static int GetMemoryPoolUseSize();

	friend class TLSMemoryPool<Message>;
private:
	char* _pBuffer;									//헤더 포함 버퍼 시작 포인터
	int _writePos;
	int _readPos;

	int	_bufferSize;
	int	_payloadSize;

	LONG _refCount;									//레퍼런스 카운트
	bool _isEncode;									//인코딩 여부

	static TLSMemoryPool<Message> s_messagePool;	//직렬화 버퍼 풀
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
		//헤더 크기는 5byte 고정이므로 LanHeader를 쓸 때는 3byte 앞에다 써준다.
		LanHeader* pHeader = (LanHeader*)(_pBuffer + 3);
		pHeader->len = GetDataSize();
	}
}

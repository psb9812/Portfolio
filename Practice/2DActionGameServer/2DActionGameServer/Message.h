/*---------------------------------------------------------------
	Packet.

	��Ʈ��ũ ��Ŷ�� Ŭ����.
	�����ϰ� ��Ŷ�� ������� ����Ÿ�� In, Out �Ѵ�.

	- ����.

	Message Message;  or CMessage Message;

	�ֱ�.
	clPacket << 40030;		or	clPacket << iValue;	(int �ֱ�)
	clPacket << 1.4;		or	clPacket << fValue;	(float �ֱ�)


	����.
	clPacket >> iValue;		(int ����)
	clPacket >> byValue;		(BYTE ����)
	clPacket >> fValue;		(float ����)

	Message Packet2;

	!.	���ԵǴ� ����Ÿ FIFO ������ �����ȴ�.
		ȯ�� ť�� �ƴϹǷ�, �ֱ�(<<).����(>>) �� ȥ���ؼ� ������� �ʵ��� �Ѵ�



	* ���� ��Ŷ ���ν��������� ó��

	BOOL	netPacketProc_CreateMyCharacter(Message *clpPacket)
	{
		DWORD dwSessionID;
		short shX, shY;
		char chHP;
		BYTE byDirection;

		*clpPacket >> dwSessionID >> byDirection >> shX >> shY >> chHP;


		*clpPacket >> dwSessionID;
		*clpPacket >> byDirection;
		*clpPacket >> shX;
		*clpPacket >> shY;
		*clpPacket >> chHP;

		...
		...
	}


	* ���� �޽���(��Ŷ) �����ο����� ó��

	Message MoveStart;
	mpMoveStart(&MoveStart, dir, x, y);
	SendPacket(&MoveStart);


	void	mpMoveStart(Message *clpPacket, BYTE byDirection, short shX, short shY)
	{
		st_NETWORK_PACKET_HEADER	stPacketHeader;
		stPacketHeader.byCode = dfNETWORK_PACKET_CODE;
		stPacketHeader.bySize = 5;
		stPacketHeader.byType = dfPACKET_CS_MOVE_START;

		clpPacket->PutData((char *)&stPacketHeader, dfNETWORK_PACKET_HEADER_SIZE);

		*clpPacket << byDirection;
		*clpPacket << shX;
		*clpPacket << shY;

	}

----------------------------------------------------------------*/
#ifndef  __Message__
#define  __Message__
#include <Windows.h>
#include <iostream>

class Message
{
public:

	/*---------------------------------------------------------------
	Packet Enum.

	----------------------------------------------------------------*/
	enum en_Message
	{
		BUFFER_DEFAULT_SIZE = 1400,		// ��Ŷ�� �⺻ ���� ������.
		BUFFER_MAX_SIZE = 10000
	};

	//////////////////////////////////////////////////////////////////////////
	// ������, �ı���.
	//
	// Return:
	//////////////////////////////////////////////////////////////////////////
	Message();
	Message(int iBufferSize);

	virtual	~Message();


	//////////////////////////////////////////////////////////////////////////
	// ��Ŷ û��.
	//
	// Parameters: ����.
	// Return: ����.
	//////////////////////////////////////////////////////////////////////////
	void	Clear(void);

	//////////////////////////////////////////////////////////////////////////
	// ���� ũ�� ��������
	//
	// Parameters: (int) �����Ϸ��� ������.
	// Return: bool. �ִ� ����� �ѱ�� �����Ѵ�.
	//////////////////////////////////////////////////////////////////////////
	bool Resize(int size);

	//////////////////////////////////////////////////////////////////////////
	// ���� ������ ���.
	//
	// Parameters: ����.
	// Return: (int)��Ŷ ���� ������ ���.
	//////////////////////////////////////////////////////////////////////////
	int	GetBufferSize(void) { return _bufferSize; }
	//////////////////////////////////////////////////////////////////////////
	// ���� ������� ������ ���.
	//
	// Parameters: ����.
	// Return: (int)������� ����Ÿ ������.
	//////////////////////////////////////////////////////////////////////////
	int	GetDataSize(void) { return _dataSize; }

	int GetBufferFreeSize(void) { return _pEnd - _pWritePos; }


	//////////////////////////////////////////////////////////////////////////
	// ���� ������ ���.
	//
	// Parameters: ����.
	// Return: (char *)���� ������.
	//////////////////////////////////////////////////////////////////////////
	char* GetBufferPtr(void) { return _pBuffer; }

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
	// �ֱ�.	�� ���� Ÿ�Ը��� ��� ����.
	//////////////////////////////////////////////////////////////////////////
	Message& operator << (unsigned char byValue);
	Message& operator << (char chValue);

	Message& operator << (short shValue);
	Message& operator << (unsigned short wValue);

	Message& operator << (int iValue);
	Message& operator << (long lValue);
	Message& operator << (float fValue);

	Message& operator << (__int64 iValue);
	Message& operator << (double dValue);


	//////////////////////////////////////////////////////////////////////////
	// ����.	�� ���� Ÿ�Ը��� ��� ����.
	//////////////////////////////////////////////////////////////////////////
	Message& operator >> (BYTE& byValue);
	Message& operator >> (char& chValue);

	Message& operator >> (short& shValue);
	Message& operator >> (WORD& wValue);

	Message& operator >> (int& iValue);
	Message& operator >> (DWORD& dwValue);
	Message& operator >> (float& fValue);

	Message& operator >> (__int64& iValue);
	Message& operator >> (double& dValue);




	//////////////////////////////////////////////////////////////////////////
	// ����Ÿ ���.
	//
	// Parameters: (char *)Dest ������. (int)Size.
	// Return: (int)������ ������.
	//////////////////////////////////////////////////////////////////////////
	int		GetData(char* chpDest, int iSize);

	//////////////////////////////////////////////////////////////////////////
	// ����Ÿ ����.
	//
	// Parameters: (char *)Src ������. (int)SrcSize.
	// Return: (int)������ ������.
	//////////////////////////////////////////////////////////////////////////
	int		PutData(char* chpSrc, int iSrcSize);


protected:

	char* _pBuffer;
	char* _pEnd;
	char* _pWritePos;
	char* _pReadPos;

	int	_bufferSize;

	//------------------------------------------------------------
	// ���� ���ۿ� ������� ������.
	//------------------------------------------------------------
	int	_dataSize;
};



#endif
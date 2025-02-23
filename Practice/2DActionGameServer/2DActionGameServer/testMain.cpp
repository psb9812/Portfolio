#include <iostream>
#include "RingBuffer.h"

RingBuffer rBuffer(10);


int main()
{
	//RingBuffer recvRingBuffer(500);

	//char testStr[82] = "1234567890 abcdefghijklmnopqrstuvwxyz 1234567890 abcdefghijklmnopqrstuvwxyz 12345";
	//char peekBuf[82] = { 0, };
	//char dequeueBuf[82] = { 0, };

	//int testLen = 81;
	//FILE* pFile;
	//fopen_s(&pFile, "RingBufferTestLog.txt", "w");

	//time_t seedTime = time((NULL));
	//srand(1717087590);
	//int sum = 0;

	//while (1)
	//{
	//	int randomNum = (rand() % testLen) + 1;

	//	if (sum + randomNum > testLen)
	//	{
	//		int over = sum + randomNum - testLen;
	//		//recvRingBuffer.Enqueue(testStr + sum, randomNum - over);
	//		//recvRingBuffer.Enqueue(testStr, over);



	//		memcpy_s(recvRingBuffer.GetRearBufferPtr(), recvRingBuffer.DirectEnqueueSize(),
	//			testStr + sum, randomNum - over);
	//		recvRingBuffer.MoveRear(randomNum - over);
	//		memcpy_s(recvRingBuffer.GetRearBufferPtr(), recvRingBuffer.GetFreeSize(),
	//			testStr, over);
	//		recvRingBuffer.MoveRear(over);
	//		sum = over;
	//	}
	//	else
	//	{
	//		//recvRingBuffer.Enqueue(testStr + sum, randomNum);
	//		sum += randomNum;
	//	}
	//	memset(peekBuf, 0, sizeof(peekBuf));
	//	memset(dequeueBuf, 0, sizeof(dequeueBuf));

	//	recvRingBuffer.Peek(peekBuf, randomNum);
	//	recvRingBuffer.MoveFront(randomNum);
	//	//recvRingBuffer.Dequeue(dequeueBuf, randomNum);

	//	//if (strcmp(peekBuf, dequeueBuf) != 0)
	//	//{
	//	//	fwrite(dequeueBuf, 1, sizeof(dequeueBuf), pFile);
	//	//	break;
	//	//}

	//	peekBuf[randomNum] = '\0';

	//	printf("%s", peekBuf);
	//}

	//fclose(pFile); 

	RingBuffer recvBuffer;

	char data[8];
	memset(data, 8, sizeof(data));
	char buffer[8];
	while (1)
	{
		recvBuffer.Enqueue(data, sizeof(data));
		recvBuffer.Enqueue(data, sizeof(data));

		recvBuffer.Dequeue(buffer, sizeof(buffer));
		recvBuffer.Dequeue(buffer, sizeof(buffer));

		std::cout << "drtdqSize: " << recvBuffer.DirectEnqueueSize() << '\n' << recvBuffer.DirectDequeueSize() << '\n';
	}

	return 0;
}
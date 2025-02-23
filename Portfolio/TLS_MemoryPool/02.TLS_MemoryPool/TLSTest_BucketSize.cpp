#include <iostream>
#include <Windows.h>
#include <process.h>
#include "TLSMemoryPool.h"
#include <thread>
#include <vector>
#include <chrono>
#include <fstream>
#pragma comment (lib, "Winmm.lib")
#include "Datas.h"
#include <utility>

struct DATA
{
	char data[16];
};

constexpr int THREAD_NUM = 4;
constexpr int TEST_ITER = 100000;	//������� �鸸�� ����, ���� ���� ����
constexpr int TOTAL_TEST_NUM = THREAD_NUM * TEST_ITER;
constexpr int STEP = 32;
constexpr int MAX_BUCKET_SIZE = 1024 * 20;

//�� �����尡 �����ϴ� �ð� - �׽�Ʈ���� ����
std::vector<INT64> allocTotalPerThread(THREAD_NUM + 1);
std::vector<INT64> freeTotalPerThread(THREAD_NUM + 1);

//������ �׽�Ʈ ������ ��� ���� - ���� ���Ͽ� �� ������
std::vector<INT64> allocAvg(MAX_BUCKET_SIZE + 1);
std::vector<INT64> freeAvg(MAX_BUCKET_SIZE + 1);

//�׽�Ʈ�� ���� �����尡 �� �غ� �Ǿ��� �� ������ �� �ְ� ���ִ� �̺�Ʈ
HANDLE startEvent;

//��ġ ��ũ ���� �Լ�
template<int BucketSize>
void Benchmark();

template<std::size_t... Is>
void runBenchmark(std::index_sequence<Is...>)
{
	//Fold Expression�� ����� �� ���� ���� Benchmark�� ȣ��
	(Benchmark<(Is + 1) * STEP> (), ...);
}
//���Ϸ� �����ϴ� �Լ�
void SaveFile();

template<int BucketSize>
void TLSThread(int idx, int threadNum, TLSMemoryPool<DATA, BucketSize>& pool);

//���� Ƚ���� ī��Ʈ
int progressNum = 1;

int main()
{
	timeBeginPeriod(1);
	startEvent = CreateEvent(NULL, TRUE, FALSE, nullptr);

	//������ ũ�⺰ ��ġ��ũ ����
	runBenchmark(std::make_index_sequence<MAX_BUCKET_SIZE / STEP>{});
	
	SaveFile();
	timeEndPeriod(1);
	return 0;
}

template<int BucketSize>
void Benchmark()
{
	std::vector<std::thread> threads;

	//�̺�Ʈ�� �����Ѵ�.
	ResetEvent(startEvent);
	TLSMemoryPool<DATA, BucketSize> tlsMP(TOTAL_TEST_NUM / BucketSize, false);

	//TLS ����
	for (int k = 1; k <= THREAD_NUM; k++)
	{
		threads.emplace_back(TLSThread<BucketSize>, k, THREAD_NUM, std::ref(tlsMP));
	}
	//������ ���� ���� �̺�Ʈ �ñ׳� �ֱ�
	SetEvent(startEvent);

	//��� �׽�Ʈ ������ ���� ���� ���
	for (auto& t : threads)
	{
		if (t.joinable())
		{
			t.join();
		}
	}

	allocAvg[progressNum] = 0;
	freeAvg[progressNum] = 0;

	// ���ġ ���
	for (int t = 1; t <= THREAD_NUM; t++)
	{
		allocAvg[progressNum] += allocTotalPerThread[t];
		freeAvg[progressNum] += freeTotalPerThread[t];
	}

	// ������ ������ŭ max ���� �� Ƚ���� ������
	allocAvg[progressNum] /= (TEST_ITER * THREAD_NUM - THREAD_NUM);
	freeAvg[progressNum] /= (TEST_ITER * THREAD_NUM - THREAD_NUM);

	wprintf(L"BucketSize %d Test Fin.\n", BucketSize);
	progressNum++;
}

void SaveFile()
{
	std::fstream file;
	file.open("TLS_Test_Per_BucketSize.txt", std::ios::out);
	if (!file)
	{
		std::cerr << "������ �� �� ����.\n";
	}
	//���� ���� ����
	file << "bucketSize, alloc, free\n";

	for (int i = 1; i <= MAX_BUCKET_SIZE / STEP; i++)
	{
		file << i * STEP << ", " << allocAvg[i] << ", " << freeAvg[i] << "\n";
	}

	file.close();
}

template<int BucketSize>
void TLSThread(int idx, int threadNum, TLSMemoryPool<DATA, BucketSize>& pool)
{
	WaitForSingleObject(startEvent, INFINITE);
	//��迡�� �ִ�ġ�� �����ؼ� �����ϱ� ����.
	INT64 allocMax = 0;
	INT64 freeMax = 0;

	//�ڽ��� �� ����ġ ���� ���� ����
	allocTotalPerThread[idx] = 0;
	freeTotalPerThread[idx] = 0;

	std::vector<DATA*> datas(TEST_ITER - (TEST_ITER % BucketSize));
	for (int i = 0; i < TEST_ITER - (TEST_ITER % BucketSize); i++)	// ��Ŷ �Ҵ� ���� �� �� ������ ������ ����
	{
		auto allocStart = std::chrono::high_resolution_clock::now();
		datas[i] = pool.Alloc();
		auto allocEnd = std::chrono::high_resolution_clock::now();

		std::chrono::duration<INT64, std::nano> allocDuration = std::chrono::duration_cast<std::chrono::nanoseconds>(allocEnd - allocStart);
		//����ġ �ݿ�
		allocTotalPerThread[idx] += allocDuration.count();
		//�ִ�ġ ����
		if (allocMax < allocDuration.count())
			allocMax = allocDuration.count();
	}

	for (int i = 0; i < TEST_ITER - (TEST_ITER % BucketSize); i++)
	{
		auto freeStart = std::chrono::high_resolution_clock::now();
		pool.Free(datas[i]);
		auto freeEnd = std::chrono::high_resolution_clock::now();

		std::chrono::duration<INT64, std::nano> freeDuration = std::chrono::duration_cast<std::chrono::nanoseconds>(freeEnd - freeStart);
		//����ġ �ݿ�
		freeTotalPerThread[idx] += freeDuration.count();
		//�ִ�ġ ����
		if (freeMax < freeDuration.count())
			freeMax = freeDuration.count();
	}

	//���� �ִ� ��Ŷ ����
	

	//�ִ�ġ ���ֱ�
	allocTotalPerThread[idx] -= allocMax;
	freeTotalPerThread[idx] -= freeMax;
}
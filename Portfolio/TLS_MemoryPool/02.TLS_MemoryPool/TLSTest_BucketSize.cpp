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
constexpr int TEST_ITER = 100000;	//스레드당 백만번 수행, 심한 경쟁 유도
constexpr int TOTAL_TEST_NUM = THREAD_NUM * TEST_ITER;
constexpr int STEP = 32;
constexpr int MAX_BUCKET_SIZE = 1024 * 20;

//각 스레드가 저장하는 시간 - 테스트마다 갱신
std::vector<INT64> allocTotalPerThread(THREAD_NUM + 1);
std::vector<INT64> freeTotalPerThread(THREAD_NUM + 1);

//스레드 테스트 각각의 평균 값들 - 최종 파일에 쓸 데이터
std::vector<INT64> allocAvg(MAX_BUCKET_SIZE + 1);
std::vector<INT64> freeAvg(MAX_BUCKET_SIZE + 1);

//테스트를 위한 스레드가 다 준비 되었을 때 시작할 수 있게 해주는 이벤트
HANDLE startEvent;

//벤치 마크 수행 함수
template<int BucketSize>
void Benchmark();

template<std::size_t... Is>
void runBenchmark(std::index_sequence<Is...>)
{
	//Fold Expression을 사용해 각 값에 대해 Benchmark를 호출
	(Benchmark<(Is + 1) * STEP> (), ...);
}
//파일로 저장하는 함수
void SaveFile();

template<int BucketSize>
void TLSThread(int idx, int threadNum, TLSMemoryPool<DATA, BucketSize>& pool);

//진행 횟수를 카운트
int progressNum = 1;

int main()
{
	timeBeginPeriod(1);
	startEvent = CreateEvent(NULL, TRUE, FALSE, nullptr);

	//데이터 크기별 벤치마크 수행
	runBenchmark(std::make_index_sequence<MAX_BUCKET_SIZE / STEP>{});
	
	SaveFile();
	timeEndPeriod(1);
	return 0;
}

template<int BucketSize>
void Benchmark()
{
	std::vector<std::thread> threads;

	//이벤트를 리셋한다.
	ResetEvent(startEvent);
	TLSMemoryPool<DATA, BucketSize> tlsMP(TOTAL_TEST_NUM / BucketSize, false);

	//TLS 측정
	for (int k = 1; k <= THREAD_NUM; k++)
	{
		threads.emplace_back(TLSThread<BucketSize>, k, THREAD_NUM, std::ref(tlsMP));
	}
	//스레드 경쟁 시작 이벤트 시그널 주기
	SetEvent(startEvent);

	//모든 테스트 스레드 종료 까지 대기
	for (auto& t : threads)
	{
		if (t.joinable())
		{
			t.join();
		}
	}

	allocAvg[progressNum] = 0;
	freeAvg[progressNum] = 0;

	// 평균치 계산
	for (int t = 1; t <= THREAD_NUM; t++)
	{
		allocAvg[progressNum] += allocTotalPerThread[t];
		freeAvg[progressNum] += freeTotalPerThread[t];
	}

	// 스레드 개수만큼 max 값을 뺀 횟수로 나누기
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
		std::cerr << "파일을 열 수 없음.\n";
	}
	//파일 제목 쓰기
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
	//통계에서 최대치는 제외해서 보정하기 위함.
	INT64 allocMax = 0;
	INT64 freeMax = 0;

	//자신이 쓸 누적치 저장 공간 비우기
	allocTotalPerThread[idx] = 0;
	freeTotalPerThread[idx] = 0;

	std::vector<DATA*> datas(TEST_ITER - (TEST_ITER % BucketSize));
	for (int i = 0; i < TEST_ITER - (TEST_ITER % BucketSize); i++)	// 버킷 할당 받은 거 다 쓰도록 조건을 조절
	{
		auto allocStart = std::chrono::high_resolution_clock::now();
		datas[i] = pool.Alloc();
		auto allocEnd = std::chrono::high_resolution_clock::now();

		std::chrono::duration<INT64, std::nano> allocDuration = std::chrono::duration_cast<std::chrono::nanoseconds>(allocEnd - allocStart);
		//누적치 반영
		allocTotalPerThread[idx] += allocDuration.count();
		//최대치 갱신
		if (allocMax < allocDuration.count())
			allocMax = allocDuration.count();
	}

	for (int i = 0; i < TEST_ITER - (TEST_ITER % BucketSize); i++)
	{
		auto freeStart = std::chrono::high_resolution_clock::now();
		pool.Free(datas[i]);
		auto freeEnd = std::chrono::high_resolution_clock::now();

		std::chrono::duration<INT64, std::nano> freeDuration = std::chrono::duration_cast<std::chrono::nanoseconds>(freeEnd - freeStart);
		//누적치 반영
		freeTotalPerThread[idx] += freeDuration.count();
		//최대치 갱신
		if (freeMax < freeDuration.count())
			freeMax = freeDuration.count();
	}

	//남아 있는 버킷 비우기
	

	//최대치 빼주기
	allocTotalPerThread[idx] -= allocMax;
	freeTotalPerThread[idx] -= freeMax;
}
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

constexpr int THREAD_NUM = 4;
constexpr int TEST_ITER = 500000;
constexpr int BUCKET_CAPACITY = 500;
constexpr int DATA_SIZE_NUM = 64;	//16 * 64 = 1024

//각 스레드가 저장하는 시간 - 테스트마다 갱신
std::vector<INT64> allocTotalPerThread(THREAD_NUM + 1);
std::vector<INT64> freeTotalPerThread(THREAD_NUM + 1);
std::vector<INT64> newTotalPerThread(THREAD_NUM + 1);
std::vector<INT64> deleteTotalPerThread(THREAD_NUM + 1);

//스레드 테스트 각각의 평균 값들 - 최종 파일에 쓸 데이터
std::vector<INT64> allocAvg(DATA_SIZE_NUM + 1);
std::vector<INT64> freeAvg(DATA_SIZE_NUM + 1);
std::vector<INT64> newAvg(DATA_SIZE_NUM + 1);
std::vector<INT64> deleteAvg(DATA_SIZE_NUM + 1);

//테스트를 위한 스레드가 다 준비 되었을 때 시작할 수 있게 해주는 이벤트
HANDLE startEvent;

//벤치 마크 수행 함수
template<typename DataType>
void Benchmark();
void SaveFile();
template<typename T>
void NewDeleteThread(int idx, int threadNum);
template<typename T>
void TLSThread(int idx, int threadNum, TLSMemoryPool<T, BUCKET_CAPACITY>& pool);

//16단위로 진행 횟수를 카운트
int progressNum = 1;

int main()
{
	timeBeginPeriod(1);
	startEvent = CreateEvent(NULL, TRUE, FALSE, nullptr);

	//데이터 크기별 벤치마크 수행, 16단위로 증가
	Benchmark<DATA16>();
	Benchmark<DATA32>();
	Benchmark<DATA48>();
	Benchmark<DATA64>();
	Benchmark<DATA80>();
	Benchmark<DATA96>();
	Benchmark<DATA112>();
	Benchmark<DATA128>();
	Benchmark<DATA144>();
	Benchmark<DATA160>();
	Benchmark<DATA176>();
	Benchmark<DATA192>();
	Benchmark<DATA208>();
	Benchmark<DATA224>();
	Benchmark<DATA240>();
	Benchmark<DATA256>();
	Benchmark<DATA272>();
	Benchmark<DATA288>();
	Benchmark<DATA304>();
	Benchmark<DATA320>();
	Benchmark<DATA336>();
	Benchmark<DATA352>();
	Benchmark<DATA368>();
	Benchmark<DATA384>();
	Benchmark<DATA400>();
	Benchmark<DATA416>();
	Benchmark<DATA432>();
	Benchmark<DATA448>();
	Benchmark<DATA464>();
	Benchmark<DATA480>();
	Benchmark<DATA496>();
	Benchmark<DATA512>();
	Benchmark<DATA528>();
	Benchmark<DATA544>();
	Benchmark<DATA560>();
	Benchmark<DATA576>();
	Benchmark<DATA592>();
	Benchmark<DATA608>();
	Benchmark<DATA624>();
	Benchmark<DATA640>();
	Benchmark<DATA656>();
	Benchmark<DATA672>();
	Benchmark<DATA688>();
	Benchmark<DATA704>();
	Benchmark<DATA720>();
	Benchmark<DATA736>();
	Benchmark<DATA752>();
	Benchmark<DATA768>();
	Benchmark<DATA784>();
	Benchmark<DATA800>();
	Benchmark<DATA816>();
	Benchmark<DATA832>();
	Benchmark<DATA848>();
	Benchmark<DATA864>();
	Benchmark<DATA880>();
	Benchmark<DATA896>();
	Benchmark<DATA912>();
	Benchmark<DATA928>();
	Benchmark<DATA944>();
	Benchmark<DATA960>();
	Benchmark<DATA976>();
	Benchmark<DATA992>();
	Benchmark<DATA1008>();
	Benchmark<DATA1024>();

	SaveFile();
	timeEndPeriod(1);
	return 0;
}

template<typename DataType>
void Benchmark()
{
	std::vector<std::thread> threads;

	ResetEvent(startEvent);
	//New/Delete 측정
	for (int k = 1; k <= THREAD_NUM; k++)
	{
		threads.emplace_back(NewDeleteThread<DataType>, k, THREAD_NUM);
	}
	SetEvent(startEvent);
	for (auto& t : threads)
	{
		if (t.joinable())
		{
			t.join();
		}
	}
	threads.clear();
	ResetEvent(startEvent);

	TLSMemoryPool<DataType, BUCKET_CAPACITY> tlsMP(THREAD_NUM * TEST_ITER / BUCKET_CAPACITY, false);
	//TLS 측정
	for (int k = 1; k <= THREAD_NUM; k++)
	{
		threads.emplace_back(TLSThread<DataType>, k, THREAD_NUM, std::ref(tlsMP));
	}
	SetEvent(startEvent);
	for (auto& t : threads)
	{
		if (t.joinable())
		{
			t.join();
		}
	}

	newAvg[progressNum] = 0;
	deleteAvg[progressNum] = 0;
	allocAvg[progressNum] = 0;
	freeAvg[progressNum] = 0;

	// 평균치 계산
	for (int t = 1; t <= THREAD_NUM; t++)
	{
		newAvg[progressNum] += newTotalPerThread[t];
		deleteAvg[progressNum] += deleteTotalPerThread[t];
		allocAvg[progressNum] += allocTotalPerThread[t];
		freeAvg[progressNum] += freeTotalPerThread[t];
	}

	newAvg[progressNum] /= (TEST_ITER * THREAD_NUM);
	deleteAvg[progressNum] /= (TEST_ITER * THREAD_NUM);
	allocAvg[progressNum] /= (TEST_ITER * THREAD_NUM);
	freeAvg[progressNum] /= (TEST_ITER * THREAD_NUM);

	wprintf(L"DataSize %d Test Fin.\n", progressNum);
	progressNum++;	
}

void SaveFile()
{
	std::fstream file;
	file.open("TLS_Test_Per_DataSize.txt", std::ios::out);
	if (!file)
	{
		std::cerr << "파일을 열 수 없음.\n";
	}
	//파일 제목 쓰기
	file << "dataSize, new, delete, alloc, free\n";

	for (int i = 1; i <= DATA_SIZE_NUM; i++)
	{
		file << i * 16 << ", " << newAvg[i] << ", " << deleteAvg[i] << ", " << allocAvg[i] << ", " << freeAvg[i] << "\n";
	}

	file.close();
}

template<typename T>
void NewDeleteThread(int idx, int threadNum)
{
	WaitForSingleObject(startEvent, INFINITE);

	//자신이 쓸 누적치 저장 공간 비우기
	newTotalPerThread[idx] = 0;
	deleteTotalPerThread[idx] = 0;

	std::vector<T*> datas(TEST_ITER);

	auto newStart = std::chrono::high_resolution_clock::now();
	for (int i = 0; i < TEST_ITER; i++)
	{
		datas[i] = new T;
	}
	auto newEnd = std::chrono::high_resolution_clock::now();
	std::chrono::duration<INT64, std::nano> newDuration = std::chrono::duration_cast<std::chrono::nanoseconds>(newEnd - newStart);
	newTotalPerThread[idx] += newDuration.count();


	auto deleteStart = std::chrono::high_resolution_clock::now();
	for (int i = 0; i < TEST_ITER; i++)
	{
		delete datas[i];
	}
	auto deleteEnd = std::chrono::high_resolution_clock::now();
	std::chrono::duration<INT64, std::nano> deleteDuration = std::chrono::duration_cast<std::chrono::nanoseconds>(deleteEnd - deleteStart);
	deleteTotalPerThread[idx] += deleteDuration.count();
}


template<typename T>
void TLSThread(int idx, int threadNum, TLSMemoryPool<T, BUCKET_CAPACITY>& pool)
{
	WaitForSingleObject(startEvent, INFINITE);

	//자신이 쓸 누적치 저장 공간 비우기
	allocTotalPerThread[idx] = 0;
	freeTotalPerThread[idx] = 0;

	auto allocStart = std::chrono::high_resolution_clock::now();
	std::vector<T*> datas(TEST_ITER);
	for (int i = 0; i < TEST_ITER; i++)
	{
		datas[i] = pool.Alloc();
	}
	auto allocEnd = std::chrono::high_resolution_clock::now();
	std::chrono::duration<INT64, std::nano> allocDuration = std::chrono::duration_cast<std::chrono::nanoseconds>(allocEnd - allocStart);
	allocTotalPerThread[idx] += allocDuration.count();


	auto freeStart = std::chrono::high_resolution_clock::now();
	for (int i = 0; i < TEST_ITER; i++)
	{
		pool.Free(datas[i]);
	}
	auto freeEnd = std::chrono::high_resolution_clock::now();
	std::chrono::duration<INT64, std::nano> freeDuration = std::chrono::duration_cast<std::chrono::nanoseconds>(freeEnd - freeStart);
	freeTotalPerThread[idx] += freeDuration.count();
}
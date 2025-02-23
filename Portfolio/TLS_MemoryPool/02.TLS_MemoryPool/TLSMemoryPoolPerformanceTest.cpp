#include <iostream>
#include <Windows.h>
#include <process.h>
#include "TLSMemoryPool.h"
#include <thread>
#include <vector>
#include <chrono>
#include <fstream>
#pragma comment (lib, "Winmm.lib")


struct DATA
{
	char data[512];
};

constexpr int THREAD_NUM = 4;
constexpr int TEST_ITER = 500000;
constexpr int BUCKET_SIZE = 500;

//각 스레드가 저장하는 시간 - 테스트마다 갱신
std::vector<INT64> allocTotalPerThread(THREAD_NUM + 1);
std::vector<INT64> freeTotalPerThread(THREAD_NUM + 1);
std::vector<INT64> newTotalPerThread(THREAD_NUM + 1);
std::vector<INT64> deleteTotalPerThread(THREAD_NUM + 1);

//스레드 테스트 각각의 평균 값들 - 최종 파일에 쓸 데이터
std::vector<INT64> allocAvg(THREAD_NUM + 1);
std::vector<INT64> freeAvg(THREAD_NUM + 1);
std::vector<INT64> newAvg(THREAD_NUM + 1);
std::vector<INT64> deleteAvg(THREAD_NUM + 1);

//테스트를 위한 스레드가 다 준비 되었을 때 시작할 수 있게 해주는 manual 이벤트
HANDLE startEvent;

//미리 생성하는 버킷의 수는 전체 테스트 수(단일 테스트 수 * 스레드 수) / 버킷 사이즈 이다.
TLSMemoryPool<DATA, BUCKET_SIZE> tlsMP(TEST_ITER * THREAD_NUM / BUCKET_SIZE, false);

//벤치 마크 수행 함수
void Benchmark();
void SaveFile();
void NewDeleteThread(int idx, int threadNum);
void TLSThread(int idx, int threadNum);

int main()
{
	//힙 예열
	for (int i = 0; i < 100; i++)
	{
		DATA* pData = new DATA;
		delete pData;
	}

	timeBeginPeriod(1);
	startEvent = CreateEvent(NULL, TRUE, FALSE, nullptr);

	Benchmark();
	SaveFile();
	timeEndPeriod(1);
	return 0;
}

void Benchmark()
{
	for (int threadNumOfTest = 1; threadNumOfTest <= THREAD_NUM; threadNumOfTest++)
	{
		//i개의 스레드에서 new/delete와 alloc/free를 측정
		std::vector<std::thread> threads;
		
		ResetEvent(startEvent);
		//New/Delete 측정
		for (int threadIdx = 1; threadIdx <= threadNumOfTest; threadIdx++)
		{
			threads.emplace_back(NewDeleteThread, threadIdx, threadNumOfTest);
		}
		//생성한 모든 스레드가 대기할 수 있도록 잠깐 시간 두기
		Sleep(100);
		SetEvent(startEvent);
		// 스레드 종료 대기
		for (auto& t : threads) 
		{
			if (t.joinable()) 
			{
				t.join();
			}
		}
		threads.clear();
		ResetEvent(startEvent);

		//TLS 측정
		for (int threadIdx = 1; threadIdx <= threadNumOfTest; threadIdx++)
		{
			threads.emplace_back(TLSThread, threadIdx, threadNumOfTest);
		}
		//생성한 모든 스레드가 대기할 수 있도록 잠깐 시간 두기
		Sleep(100);
		SetEvent(startEvent);
		//스레드 종료 대기
		for (auto& t : threads)
		{
			if (t.joinable())
			{
				t.join();
			}
		}

		newAvg[threadNumOfTest] = 0;
		deleteAvg[threadNumOfTest] = 0;
		allocAvg[threadNumOfTest] = 0;
		freeAvg[threadNumOfTest] = 0;

		// 이 테스트에서 진행한 스레드의 모든 시간 합을 더한다.
		for (int threadIdx = 1; threadIdx <= threadNumOfTest; threadIdx++)
		{
			newAvg[threadNumOfTest] += newTotalPerThread[threadIdx];
			deleteAvg[threadNumOfTest] += deleteTotalPerThread[threadIdx];
			allocAvg[threadNumOfTest] += allocTotalPerThread[threadIdx];
			freeAvg[threadNumOfTest] += freeTotalPerThread[threadIdx];
		}

		// 테스트 횟수로 나눠서 평균 단일 연산 시간을 구한다.
		newAvg[threadNumOfTest] /= TEST_ITER * threadNumOfTest;
		deleteAvg[threadNumOfTest] /= TEST_ITER * threadNumOfTest;
		allocAvg[threadNumOfTest] /= TEST_ITER * threadNumOfTest;
		freeAvg[threadNumOfTest] /= TEST_ITER * threadNumOfTest;

		//종료 했음을 알린다.
		wprintf(L"thread % d Test Fin.\n", threadNumOfTest);
	}
}

void SaveFile()
{
	std::fstream file;
	file.open("TLS_Test_Per_Thread.txt", std::ios::out);
	if (!file)
	{
		std::cerr << "파일을 열 수 없음.\n";
		return;
	}
	//파일 제목 쓰기
	file << "스레드 수, new, delete, alloc, free\n";

	for (int i = 1; i <= THREAD_NUM; i++)
	{
		file << i << ", " << newAvg[i] << ", " << deleteAvg[i] << ", " << allocAvg[i] << ", " << freeAvg[i] << "\n";
	}

	file.close();
}

void NewDeleteThread(int idx, int threadNum)
{
	//이벤트 대기
	WaitForSingleObject(startEvent, INFINITE);

	//자신이 쓸 누적치 저장 공간 비우기
	newTotalPerThread[idx] = 0;
	deleteTotalPerThread[idx] = 0;

	std::vector<DATA*> datas(TEST_ITER);

	auto newStart = std::chrono::high_resolution_clock::now();
	//시간 측정
	for(int i = 0; i < TEST_ITER; i++)
	{
		datas[i] = new DATA;
	}
	auto newEnd = std::chrono::high_resolution_clock::now();
	std::chrono::duration<INT64, std::nano> newDuration = std::chrono::duration_cast<std::chrono::nanoseconds>(newEnd - newStart);
	//누적치 반영
	newTotalPerThread[idx] += newDuration.count();

	auto deleteStart = std::chrono::high_resolution_clock::now();
	for (int i = 0; i < TEST_ITER; i++)
	{
		delete datas[i];
	}
	auto deleteEnd = std::chrono::high_resolution_clock::now();
	std::chrono::duration<INT64, std::nano> deleteDuration = std::chrono::duration_cast<std::chrono::nanoseconds>(deleteEnd - deleteStart);
	//누적치 반영
	deleteTotalPerThread[idx] += deleteDuration.count();

	wprintf(L"New Delete %d Fin\n", threadNum);
}

void TLSThread(int idx, int threadNum)
{
	WaitForSingleObject(startEvent, INFINITE);

	//자신이 쓸 누적치 저장 공간 비우기
	allocTotalPerThread[idx] = 0;
	freeTotalPerThread[idx] = 0;

	std::vector<DATA*> datas(TEST_ITER);
	auto allocStart = std::chrono::high_resolution_clock::now();
	//시간 측정
	for (int i = 0; i < TEST_ITER; i++)
	{
		datas[i] = tlsMP.Alloc();
	}
	auto allocEnd = std::chrono::high_resolution_clock::now();

	std::chrono::duration<INT64, std::nano> allocDuration = std::chrono::duration_cast<std::chrono::nanoseconds>(allocEnd - allocStart);
	//누적치 반영
	allocTotalPerThread[idx] += allocDuration.count();

	auto freeStart = std::chrono::high_resolution_clock::now();
	for (int i = 0; i < TEST_ITER; i++)
	{
		tlsMP.Free(datas[i]);
	}
	auto freeEnd = std::chrono::high_resolution_clock::now();
	std::chrono::duration<INT64, std::nano> freeDuration = std::chrono::duration_cast<std::chrono::nanoseconds>(freeEnd - freeStart);
	//누적치 반영
	freeTotalPerThread[idx] += freeDuration.count();

	wprintf(L"TLS %d Fin\n", threadNum);
}
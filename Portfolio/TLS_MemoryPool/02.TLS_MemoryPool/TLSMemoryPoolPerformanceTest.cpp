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

//�� �����尡 �����ϴ� �ð� - �׽�Ʈ���� ����
std::vector<INT64> allocTotalPerThread(THREAD_NUM + 1);
std::vector<INT64> freeTotalPerThread(THREAD_NUM + 1);
std::vector<INT64> newTotalPerThread(THREAD_NUM + 1);
std::vector<INT64> deleteTotalPerThread(THREAD_NUM + 1);

//������ �׽�Ʈ ������ ��� ���� - ���� ���Ͽ� �� ������
std::vector<INT64> allocAvg(THREAD_NUM + 1);
std::vector<INT64> freeAvg(THREAD_NUM + 1);
std::vector<INT64> newAvg(THREAD_NUM + 1);
std::vector<INT64> deleteAvg(THREAD_NUM + 1);

//�׽�Ʈ�� ���� �����尡 �� �غ� �Ǿ��� �� ������ �� �ְ� ���ִ� manual �̺�Ʈ
HANDLE startEvent;

//�̸� �����ϴ� ��Ŷ�� ���� ��ü �׽�Ʈ ��(���� �׽�Ʈ �� * ������ ��) / ��Ŷ ������ �̴�.
TLSMemoryPool<DATA, BUCKET_SIZE> tlsMP(TEST_ITER * THREAD_NUM / BUCKET_SIZE, false);

//��ġ ��ũ ���� �Լ�
void Benchmark();
void SaveFile();
void NewDeleteThread(int idx, int threadNum);
void TLSThread(int idx, int threadNum);

int main()
{
	//�� ����
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
		//i���� �����忡�� new/delete�� alloc/free�� ����
		std::vector<std::thread> threads;
		
		ResetEvent(startEvent);
		//New/Delete ����
		for (int threadIdx = 1; threadIdx <= threadNumOfTest; threadIdx++)
		{
			threads.emplace_back(NewDeleteThread, threadIdx, threadNumOfTest);
		}
		//������ ��� �����尡 ����� �� �ֵ��� ��� �ð� �α�
		Sleep(100);
		SetEvent(startEvent);
		// ������ ���� ���
		for (auto& t : threads) 
		{
			if (t.joinable()) 
			{
				t.join();
			}
		}
		threads.clear();
		ResetEvent(startEvent);

		//TLS ����
		for (int threadIdx = 1; threadIdx <= threadNumOfTest; threadIdx++)
		{
			threads.emplace_back(TLSThread, threadIdx, threadNumOfTest);
		}
		//������ ��� �����尡 ����� �� �ֵ��� ��� �ð� �α�
		Sleep(100);
		SetEvent(startEvent);
		//������ ���� ���
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

		// �� �׽�Ʈ���� ������ �������� ��� �ð� ���� ���Ѵ�.
		for (int threadIdx = 1; threadIdx <= threadNumOfTest; threadIdx++)
		{
			newAvg[threadNumOfTest] += newTotalPerThread[threadIdx];
			deleteAvg[threadNumOfTest] += deleteTotalPerThread[threadIdx];
			allocAvg[threadNumOfTest] += allocTotalPerThread[threadIdx];
			freeAvg[threadNumOfTest] += freeTotalPerThread[threadIdx];
		}

		// �׽�Ʈ Ƚ���� ������ ��� ���� ���� �ð��� ���Ѵ�.
		newAvg[threadNumOfTest] /= TEST_ITER * threadNumOfTest;
		deleteAvg[threadNumOfTest] /= TEST_ITER * threadNumOfTest;
		allocAvg[threadNumOfTest] /= TEST_ITER * threadNumOfTest;
		freeAvg[threadNumOfTest] /= TEST_ITER * threadNumOfTest;

		//���� ������ �˸���.
		wprintf(L"thread % d Test Fin.\n", threadNumOfTest);
	}
}

void SaveFile()
{
	std::fstream file;
	file.open("TLS_Test_Per_Thread.txt", std::ios::out);
	if (!file)
	{
		std::cerr << "������ �� �� ����.\n";
		return;
	}
	//���� ���� ����
	file << "������ ��, new, delete, alloc, free\n";

	for (int i = 1; i <= THREAD_NUM; i++)
	{
		file << i << ", " << newAvg[i] << ", " << deleteAvg[i] << ", " << allocAvg[i] << ", " << freeAvg[i] << "\n";
	}

	file.close();
}

void NewDeleteThread(int idx, int threadNum)
{
	//�̺�Ʈ ���
	WaitForSingleObject(startEvent, INFINITE);

	//�ڽ��� �� ����ġ ���� ���� ����
	newTotalPerThread[idx] = 0;
	deleteTotalPerThread[idx] = 0;

	std::vector<DATA*> datas(TEST_ITER);

	auto newStart = std::chrono::high_resolution_clock::now();
	//�ð� ����
	for(int i = 0; i < TEST_ITER; i++)
	{
		datas[i] = new DATA;
	}
	auto newEnd = std::chrono::high_resolution_clock::now();
	std::chrono::duration<INT64, std::nano> newDuration = std::chrono::duration_cast<std::chrono::nanoseconds>(newEnd - newStart);
	//����ġ �ݿ�
	newTotalPerThread[idx] += newDuration.count();

	auto deleteStart = std::chrono::high_resolution_clock::now();
	for (int i = 0; i < TEST_ITER; i++)
	{
		delete datas[i];
	}
	auto deleteEnd = std::chrono::high_resolution_clock::now();
	std::chrono::duration<INT64, std::nano> deleteDuration = std::chrono::duration_cast<std::chrono::nanoseconds>(deleteEnd - deleteStart);
	//����ġ �ݿ�
	deleteTotalPerThread[idx] += deleteDuration.count();

	wprintf(L"New Delete %d Fin\n", threadNum);
}

void TLSThread(int idx, int threadNum)
{
	WaitForSingleObject(startEvent, INFINITE);

	//�ڽ��� �� ����ġ ���� ���� ����
	allocTotalPerThread[idx] = 0;
	freeTotalPerThread[idx] = 0;

	std::vector<DATA*> datas(TEST_ITER);
	auto allocStart = std::chrono::high_resolution_clock::now();
	//�ð� ����
	for (int i = 0; i < TEST_ITER; i++)
	{
		datas[i] = tlsMP.Alloc();
	}
	auto allocEnd = std::chrono::high_resolution_clock::now();

	std::chrono::duration<INT64, std::nano> allocDuration = std::chrono::duration_cast<std::chrono::nanoseconds>(allocEnd - allocStart);
	//����ġ �ݿ�
	allocTotalPerThread[idx] += allocDuration.count();

	auto freeStart = std::chrono::high_resolution_clock::now();
	for (int i = 0; i < TEST_ITER; i++)
	{
		tlsMP.Free(datas[i]);
	}
	auto freeEnd = std::chrono::high_resolution_clock::now();
	std::chrono::duration<INT64, std::nano> freeDuration = std::chrono::duration_cast<std::chrono::nanoseconds>(freeEnd - freeStart);
	//����ġ �ݿ�
	freeTotalPerThread[idx] += freeDuration.count();

	wprintf(L"TLS %d Fin\n", threadNum);
}
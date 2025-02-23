#include <iostream>
#include "MemoryPool.h"
#define PROFILE
#include "Profiler.h"
using namespace std;

constexpr int TEST_NUM = 100000;

class Test
{
public:
	Test()
	{
		// cout << "Constructor Test" << endl;
	}

	~Test()
	{
		// cout << "Destructor Test" << endl;
	}

	char byte[500] = { 0, };
};

void Test1()
{
	Test* test[TEST_NUM];

	START_PROFILEING(f, "new/delete Ver");

	for (int i = 0; i < TEST_NUM; i++)
	{
		test[i] = new Test;
	}

	for (int i = 0; i < TEST_NUM; i++)
	{
		delete test[i];
	}
}

MemoryPool<Test> objPool(TEST_NUM, true);

void Test2()
{
	Test* test[TEST_NUM];
	START_PROFILEING(d, "MemoryPool Ver");

	for (int i = 0; i < TEST_NUM; i++)
	{
		test[i] = objPool.Alloc();
	}

	for (int i = 0; i < TEST_NUM; i++)
	{
		objPool.Free(test[i]);
	}
}

int main()
{
	for (int i = 0; i < 1 ; i++)
	{
		Test1();
		Test2();
	}

	ProfileManager::ProfileDataOutText(L"MemoryPoolProfile3.txt");

	return 0;
}
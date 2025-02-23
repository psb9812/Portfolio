#include <iostream>
#include "LockFreeQueue.h"


int main()
{
	LockFreeQueue<int> queue;

	for (int i = 0; i < 10; i++)
	{
		queue.Enqueue(i);
	}

	std::cout << "size : " << queue.GetSize() << '\n';

	while(true)
	{
		auto ret = queue.Dequeue();
		if (!ret.has_value())
		{
			break;
		}
		std::cout << ret.value() << '\n';
	}
	std::cout << "size : " << queue.GetSize() << '\n';

	std::cout << "ClearTest \n";
	for (int i = 0; i < 10; i++)
	{
		queue.Enqueue(i);
	}

	queue.Clear();
	while (true)
	{
		auto ret = queue.Dequeue();
		if (!ret.has_value())
		{
			break;
		}
		std::cout << ret.value() << '\n';
	}
	std::cout << "size : " << queue.GetSize() << '\n';



	return 0;
}
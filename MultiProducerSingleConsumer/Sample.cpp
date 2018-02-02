// MultiProducerMultiConsumer.cpp : Defines the entry point for the console application.
//

#include <iostream>
#include <chrono>
#include "MultiProducerSingleConsumer.h"
#include <conio.h>
#include <windows.h>
#include <ctime>
#define TIME_DIVIDER 10000

int fn(int i, UINT delay)
{
	while (true)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(delay));
		return i;
	}
}

void fn_consumer(int i)
{
	std::cout << std::endl << i << " received from producers at time = " << std::chrono::high_resolution_clock::now().time_since_epoch().count() / TIME_DIVIDER;
}

int main(int argc, char* argv[])
{
	std::shared_ptr<std::queue<int>> queue(new std::queue<int>);
	std::shared_ptr<std::mutex> mutex(new std::mutex);
	std::shared_ptr<std::condition_variable> cv(new std::condition_variable);
	
	MultiProducerSingleConsumer<int> pc(queue, mutex, cv, fn_consumer);
	for (UINT i = 1; i <= 10; i++)
		pc.addProducer(std::bind(fn, i, i*100));

	_getch();//SingleProducerSingleConsumer::start doesn't block the parent thread so we need to block ourselves

	return 0;
}


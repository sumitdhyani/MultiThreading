// TimerVersion2.cpp : Defines the entry point for the console application.
//
#include "TimerCaller.h"
#include <iostream>
#include <iostream>
#include <iomanip>
#include <chrono>
#include <conio.h>
#include <ctime>
#include <thread>
#include <string>
#include <stdlib.h>
#include <windows.h>


#define TIME_DIVIDER 10000
void f(int n)
{
	std::cout << std::endl << "Inside f" << n << " at time =" << std::chrono::high_resolution_clock::now().time_since_epoch().count() / TIME_DIVIDER;
}

int main()
{
	std::shared_ptr<std::queue<std::function<void()>>> fn_queue(new std::queue<std::function<void()>>);
	std::shared_ptr<std::mutex> mutex(new std::mutex);
	std::shared_ptr<std::condition_variable> cv(new std::condition_variable);
	std::vector<std::pair<std::function<void()>, UINT>> fn_producers;


	TimerCaller tc(fn_queue, mutex, cv);
	std::vector<long> timerIds;
	for (UINT i = 1; i <= 10; i++)
		timerIds.push_back(tc.addTimer(std::bind(f, i), i * 100));
	
	_getch();
	for (auto id : timerIds)
		tc.deleteTimer(id);

	return 0;
}


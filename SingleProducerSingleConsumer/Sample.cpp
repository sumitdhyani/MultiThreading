// SingleProducerSingleConsumer.cpp : Defines the entry point for the console application.
//
#include <iostream>
#include <string>
#include <mutex>
#include <condition_variable>
#include "Common.h"
#include <conio.h>
#include <windows.h>
#include "SingleProducerSingleConsumer.h"

int push()
{
	static int i = 0;
	Sleep(100);
	return ++i;
}

void process(int item)
{
	std::cout << std::endl << item << " Received from producer";
}


int main(int argc, char* argv[])
{
	std::shared_ptr<std::queue<int>> queue(new std::queue<int>);
	std::shared_ptr<std::mutex> mutex(new std::mutex);
	std::shared_ptr<std::condition_variable> cv(new std::condition_variable);

	SingleProducerSingleConsumer<int>* pc = new SingleProducerSingleConsumer<int>(queue, mutex, cv, push, process);
	pc->start();
	_getch();//SingleProducerSingleConsumer::start doesn't block the parent thread so we need to block ourselves

	return 0;
}


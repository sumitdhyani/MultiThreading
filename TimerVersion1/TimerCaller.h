#pragma once
#include "Common.h"
#include "MultiProducerSingleConsumer/MultiProducerSingleConsumer.h"
#include <memory>
#include <map>


//Using a decorator to reuse later requirements of logging and other pre-processing
template <class T>
class returnSameThingWithDelay : public returnSameThing<T>
{
	std::shared_ptr<returnSameThing> m_baseObject;
	UINT m_delay;
public:
	T operator()()
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(m_delay));
		return (*m_baseObject)();
	}

	returnSameThingWithDelay(T object, std::shared_ptr<returnSameThing<T>> baseObject, UINT delay) :
							 returnSameThing<T>(object),
							 m_baseObject(baseObject),
							 m_delay(delay)
	{}
};



class TimerCaller
{

	typedef std::function<void()> VoidFunc;
	typedef MultiProducerSingleConsumer<VoidFunc> Mpsc;//Stands for Multi-producer, Single-consumer

	std::unique_ptr<Mpsc> m_mpsc;//Accessed only in one thread so unique_ptr
public:
	TimerCaller(std::shared_ptr<std::queue<VoidFunc>> queue,
				std::shared_ptr<std::mutex> mutex,
				std::shared_ptr<std::condition_variable> cv
				)
				:m_mpsc(std::unique_ptr<Mpsc>(new Mpsc(queue, mutex, cv, callItAsAFunction<VoidFunc>)))

	{
	}

	long addTimer(VoidFunc func, UINT delay)
	{
		return m_mpsc->addProducer(returnSameThingWithDelay<VoidFunc>(func, std::shared_ptr<returnSameThing<VoidFunc>>(new returnSameThing<VoidFunc>(func)), delay));
	}

	bool deleteTimer(long timerId)
	{
		return m_mpsc->removeProducer(timerId);
	}

	~TimerCaller(){};
};

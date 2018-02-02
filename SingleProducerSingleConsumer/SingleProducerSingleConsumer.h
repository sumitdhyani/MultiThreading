#pragma once
#include "Common.h"
#include <boost/smart_ptr/shared_ptr.hpp>

//The container which synchronizes the events between producer and consumer
//Inherit this class and override "push" and "process" methods
//Call the start method
template <class T>
class SingleProducerSingleConsumer
{
	std::shared_ptr<bool> m_consumerWaitingFlag;
	std::shared_ptr<FifoQueueProducerPredicate<T>> m_producer;
	std::shared_ptr<FifoQueueConsumerPredicate<T>> m_consumer;
	//A handle to the maintain the reference count of the mutex shared_ptr to 1, so as to prevent the freeing of the mutex
	std::shared_ptr<std::mutex> m_mutex;
	std::shared_ptr<std::condition_variable> m_cv;
public:
	SingleProducerSingleConsumer(	std::shared_ptr<std::queue<T>> queue,
									std::shared_ptr<std::mutex> mutex,
									std::shared_ptr<std::condition_variable> cv,
									std::function<T()> fn_producer,
									std::function<void(T)> fn_consumer
									)
		 :m_consumerWaitingFlag(std::shared_ptr<bool>(new bool(true))),
		 m_producer(std::shared_ptr<FifoQueueProducerPredicate<T>>(new FifoQueueProducerPredicate<T>(queue, std::shared_ptr<std::unique_lock<std::mutex>>(new std::unique_lock<std::mutex>(*mutex, std::defer_lock)), cv, fn_producer, m_consumerWaitingFlag) ) ),
		 m_consumer(std::shared_ptr<FifoQueueConsumerPredicate<T>>(new FifoQueueConsumerPredicate<T>(queue, std::shared_ptr<std::unique_lock<std::mutex>>(new std::unique_lock<std::mutex>(*mutex, std::defer_lock)), cv, fn_consumer, m_consumerWaitingFlag) ) ),
		 m_mutex(mutex),
		 m_cv(cv)
	{
	}


	void start()
	{
		m_producer->start();
		m_consumer->start();
	}

	~SingleProducerSingleConsumer()
	{
		m_producer->exit();
		m_cv->notify_one();
		m_consumer->exit();
	}
};

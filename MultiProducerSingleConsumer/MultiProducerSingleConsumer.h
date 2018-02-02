#pragma once
#include "Common.h"
#include <map>

template <class T>
class MultiProducerSingleConsumer
{
	typedef FifoQueueProducerPredicate<T> Producer;
	typedef FifoQueueConsumerPredicate<T> Consumer;
	typedef std::unique_lock<std::mutex> Lock;

	std::shared_ptr<std::queue<T>> m_queue;
	std::map<long, std::shared_ptr<Producer>> m_producers;
	std::shared_ptr<bool> m_consumerWaitingFlag;
	std::shared_ptr<std::condition_variable> m_cv;
	std::shared_ptr<std::mutex> m_mutex_queue;
	std::mutex m_mutex_producerStore;

	std::shared_ptr<FifoQueueConsumerPredicate<T>> m_consumer;
public:
	MultiProducerSingleConsumer(std::shared_ptr<std::queue<T>> queue,
								 std::shared_ptr<std::mutex> mutex,
								 std::shared_ptr<std::condition_variable> cv,
								 std::function<void(T)> fn_consumer
								 )
								 :m_queue(queue),
								  m_mutex_queue(mutex),
								  m_cv(cv),
								  m_consumerWaitingFlag(std::shared_ptr<bool>(new bool(true)))

	{
		 m_consumer = std::shared_ptr<Consumer>(new Consumer(queue,
															 std::shared_ptr<Lock>(new Lock(*mutex, std::defer_lock)),
															 cv,
															 fn_consumer,
															 m_consumerWaitingFlag
															 )
												);
		 m_consumer->start();
	}


	long addProducer(std::function<T()> func_producer)
	{
		std::shared_ptr<Lock> lk(new Lock(*m_mutex_queue, std::defer_lock));
		std::shared_ptr<Producer> producer(new Producer(m_queue,
														lk,
														m_cv,
														func_producer,
														m_consumerWaitingFlag
														)
											);
		producer->start();
		long id = std::chrono::high_resolution_clock::now().time_since_epoch().count() + rand();//Add rand() to make sure the key is unique, in case this method is called very quickly at the same time
		Lock lock(m_mutex_producerStore);
		m_producers[id] = producer;
		return id;
	}

	bool removeProducer(long id)
	{
		Lock lock(m_mutex_producerStore);
		auto searchResult = m_producers.find(id);
		if (m_producers.end() != searchResult)
		{
			searchResult->second->exit();
			m_producers.erase(searchResult);
			return true;
		}
		else
			return false;
	}

	~MultiProducerSingleConsumer()
	{
		try
		{
			for (auto it : m_producers)
				it.second->exit();

			m_cv->notify_all();
			m_consumer->exit();
		}
		catch (...){}
	};
};


//template <class T>
//class MultiProducerSingleConsumerVersion2
//{
//	std::queue<boost::optional<int>>& m_eventQueue;
//	std::mutex* m_mutex;
//	std::condition_variable* m_cv;
//	std::vector<std::function<boost::optional<T>()>> m_fn_producers;
//	std::function<void(T)> m_fn_consumer;
//	std::vector<boost::shared_ptr<FifoQueueProducerPredicate<T>>> m_producers;
//	boost::shared_ptr<FifoQueueConsumerPredicate<T>> m_consumer;
//public:
//	MultiProducerSingleConsumerVersion2(std::queue<boost::optional<int>>& queue,
//										const std::vector<std::function<boost::optional<T>()>>& fn_producers,
//										std::function<void(T)> fn_consumer
//										) :
//										m_eventQueue(queue),
//										m_fn_producers(fn_producers),
//										m_fn_consumer(fn_consumer)
//	{
//	}
//
//
//	void start()
//	{
//		for (auto item : m_fn_producers)
//		{
//			FifoQueueProducerPredicate<T>* producerThread = new FifoQueueProducerPredicate<T>(m_eventQueue, m_mutex, m_cv, item);
//			producerThread->start();
//			m_producers.push_back(boost::shared_ptr<FifoQueueProducerPredicate<T>>(producerThread));
//		}
//
//		m_consumer = boost::shared_ptr<FifoQueueConsumerPredicate<T>>(new FifoQueueConsumerPredicate<T>(m_eventQueue, m_mutex, m_cv, m_fn_consumer));
//		m_consumer->start();
//
//		for (auto thread : m_producers)
//			(*thread)->join();
//
//		(*m_consumer)->join();
//	}
//};
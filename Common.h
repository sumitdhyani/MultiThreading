#pragma once

#include <algorithm>
#include <xfunctional>
#include <vector>
#include <iostream>	
#include <fstream>
#include <thread>
#include <queue>
#include <functional>
#include <chrono>
#include <mutex>
#include <memory>
#include <condition_variable>
#include <boost/optional/optional.hpp>
#include <atomic>  
#include <future>  

#if !defined(UINT)
typedef unsigned int UINT;
#endif

#if !defined(INT)
typedef int INT;
#endif


template <class T, class comp = std::less<T> >
class BinaryHeap
{
	std::vector<T> _arr;
	UINT _size;

	UINT getParentIndex(UINT index)
	{
		if (0 == index)
			return 0;
		else
			return ((index - 1) / 2);
	}

	UINT getLeftChildIndex(UINT index)
	{
		return (index * 2 + 1);
	}

	UINT getRightChildIndex(UINT index)
	{
		return (index * 2 + 2);
	}

public:
	BinaryHeap()
	{
		_size = 0;
	}

	~BinaryHeap(){};

	bool empty()
	{
		return (_size == 0);
	}

	void insert(T val)
	{
		if (_arr.size() >= ++_size)
			_arr[_size - 1] = val;
		else
			_arr.push_back(val);

		UINT currIndex = _size - 1;
		UINT  parentIndex = getParentIndex(currIndex);
		while (comp()(_arr[currIndex], _arr[parentIndex]))
		{
			std::swap(_arr[currIndex], _arr[parentIndex]);
			currIndex = parentIndex;
			parentIndex = getParentIndex(currIndex);
		}
	}

	void removeTop()
	{
		if (empty())
			return;

		_arr[0] = _arr[_size - 1];
		--_size;

		UINT currIndex = 0;

		bool carryOn = true;
		while (carryOn)
		{
			UINT left = (getLeftChildIndex(currIndex) < _size) ? getLeftChildIndex(currIndex) : currIndex;
			UINT right = (getRightChildIndex(currIndex) < _size) ? getRightChildIndex(currIndex) : currIndex;

			UINT indexToCompare = comp()(_arr[left], _arr[right]) ? left : right;
			if (comp()(_arr[indexToCompare], _arr[currIndex]))
			{
				std::swap(_arr[indexToCompare], _arr[currIndex]);
				currIndex = indexToCompare;
			}
			else
				carryOn = false;
		}
	}

	const T& getExtreme() const
	{
		if (_size > 0)
			return _arr[0];
		else
			throw std::runtime_error("Heap is empty and top is being asked");
	}

	UINT getSize() const
	{
		return _size;
	}
};


typedef BinaryHeap<UINT, std::less<UINT>> MinHeap;
typedef BinaryHeap<UINT, std::greater<UINT>> MaxHeap;


//OOP wrapper over std::thread
//Call the "start" method in the parent thread which will later call the "run" method later in the new thread
class Thread
{
	bool m_ThreadRunning;
	virtual void run() = 0;
	std::future<void> m_future;
public:

	void start()
	{
		m_future = std::async(&Thread::run,this);
	}

	virtual ~Thread(){}

	virtual void exit()
	{
		m_future.get();
	}
};

//Producer class
template <class T>
class SingleProducer : public Thread
{
	std::shared_ptr<std::unique_lock<std::mutex>> m_lock;
	std::shared_ptr<std::condition_variable> m_cv;
	const std::shared_ptr<bool> m_consumerWaitingFlag;
	std::atomic<bool> m_exitThread;
	virtual void preLockActions() {};
	virtual void preUnlockActions() {};
	virtual void run()
	{
		while (!m_exitThread.load())
		{
			T item = getNextElement();
			preLockActions();
			m_lock->lock();
			pushItem(item);
			preUnlockActions();
			//m_consumerWaitingFlag is accessed by both producer(s) and consumer(s), so should be accessed within a critical section,
			//hence this redundant and stupid looking "if"
			if (*m_consumerWaitingFlag)
			{
				m_lock->unlock();
				m_cv->notify_all();
			}
			else
				m_lock->unlock();
		}
	}

	virtual bool isQueueEmpty() = 0;
	virtual void pushItem(T queueItem) = 0;
	virtual T getNextElement() = 0;
public:
	SingleProducer(std::shared_ptr<std::unique_lock<std::mutex>> lock, std::shared_ptr<std::condition_variable> cv, const std::shared_ptr<bool> consumerWaitingFlag)
		:m_lock(lock),
		 m_cv(cv),
		 m_consumerWaitingFlag(consumerWaitingFlag)
	{
		m_exitThread.store(false);
	}

	virtual void exit()
	{
		//Do the action which will make the thread exit
		m_exitThread.store(true);
		Thread::exit();
	}
};



//Consumer class
template <class T>
class SingleConsumer : public Thread
{
	std::shared_ptr<std::unique_lock<std::mutex>> m_lock;
	std::shared_ptr<std::condition_variable> m_cv;
	std::shared_ptr<bool> m_consumerWaitingFlag;
	std::atomic<bool> m_exitThread;
	virtual void preLockActions() {};
	virtual void preUnlockActions() {};
	virtual void run()
	{
		while (!m_exitThread.load())
		{
			preLockActions();
			m_lock->lock();
			if (isQueueEmpty())
			{
				*m_consumerWaitingFlag = true;
				m_cv->wait(*m_lock);
				*m_consumerWaitingFlag = false;
			}

			while (!isQueueEmpty())
				storeLocally(pullItem());
			preUnlockActions();
			m_lock->unlock();

			processLocalQueueElements();
		}
		std::cout << std::endl << "Exiting" << std::endl;

	}

	virtual void storeLocally(T item) = 0;
	virtual void processLocalQueueElements() = 0;
	virtual bool isQueueEmpty() = 0;
	virtual T pullItem() = 0;
public:
	SingleConsumer(std::shared_ptr<std::unique_lock<std::mutex>> lock, std::shared_ptr<std::condition_variable> cv, std::shared_ptr<bool> consumerWaitingFlag)
		:m_lock(lock),
		 m_cv(cv),
		 m_consumerWaitingFlag(consumerWaitingFlag)
	{
		m_exitThread.store(false);
		*m_consumerWaitingFlag = false;
	}

	virtual void exit()
	{
		//Do the action which will make the thread exit
		m_exitThread.store(true);
		Thread::exit();
	}
};


//Producer which pushes elements in the shared queue in fifo fashion
template <class T>
class FifoQueueProducer : public SingleProducer < T >
{
	std::shared_ptr<std::queue<T>> m_eventQueque;
	virtual bool isQueueEmpty()
	{
		return m_eventQueque->empty();
	}

	void pushItem(T queueItem)
	{
		m_eventQueque->push(queueItem);
	}

public:
	FifoQueueProducer(std::shared_ptr<std::queue<T>> queue, std::shared_ptr<std::unique_lock<std::mutex>> lock, std::shared_ptr<std::condition_variable> cv, const std::shared_ptr<bool> consumerWaitingFlag)
		:SingleProducer < T >(lock,cv, consumerWaitingFlag),
		 m_eventQueque(queue)
	{
	}
};



//Consumer which pulls elements in the shared queue in fifo fashion
template <class T>
class FifoQueueConsumer : public SingleConsumer < T >
{
	std::shared_ptr<std::queue<T>> m_eventQueque;
	std::queue<T> m_localEventQueque;
	virtual void onNewItem(T item) = 0;
	virtual bool isQueueEmpty()
	{
		return m_eventQueque->empty();
	}

	virtual void storeLocally(T item)
	{
		m_localEventQueque.push(item);
	}

	virtual void processLocalQueueElements()
	{
		while (!m_localEventQueque.empty())
		{
			onNewItem(m_localEventQueque.front());
			m_localEventQueque.pop();
		}
	}

	virtual T pullItem()
	{
		T item = m_eventQueque->front();
		m_eventQueque->pop();
		return item;
	}

public:
	FifoQueueConsumer(std::shared_ptr<std::queue<T>> queue, std::shared_ptr<std::unique_lock<std::mutex>> lock, std::shared_ptr<std::condition_variable> cv, std::shared_ptr<bool> consumerWaitingFlag)
		:SingleConsumer < T >(lock, cv, consumerWaitingFlag),
		 m_eventQueque(queue)
	{
	}
};




//Execute the predicate in producer thread
template <class T>
class FifoQueueProducerPredicate : public FifoQueueProducer < T >
{
	std::function<T()> m_fn;
	virtual T getNextElement()
	{
		return m_fn();
	}

public:
	FifoQueueProducerPredicate(std::shared_ptr<std::queue<T>> queue, std::shared_ptr<std::unique_lock<std::mutex>> lock, std::shared_ptr<std::condition_variable> cv, std::function<T()> fn, const std::shared_ptr<bool> consumerWaitingFlag)
		:FifoQueueProducer < T >(queue, lock, cv, consumerWaitingFlag),
		 m_fn(fn)
	{
	}
};


//Execute the predicate in consumer thread
template <class T>
class FifoQueueConsumerPredicate : public FifoQueueConsumer < T >
{
	std::function<void(T)> m_fn;
	virtual void onNewItem(T item)
	{
		m_fn(item);
	}

public:
	FifoQueueConsumerPredicate(std::shared_ptr<std::queue<T>> queue, std::shared_ptr<std::unique_lock<std::mutex>> lock, std::shared_ptr<std::condition_variable> cv, std::function<void(T)> fn, std::shared_ptr<bool> consumerWaitingFlag)
		:FifoQueueConsumer < T >(queue, lock, cv, consumerWaitingFlag),
		 m_fn(fn)
	{
	}
};


template <class T>
class FifoDelayedCaller : public FifoQueueProducerPredicate<T>
{
	UINT m_delay;
	virtual void preLockActions()
	{
		if (0 < m_delay)
			std::this_thread::sleep_for(std::chrono::milliseconds(m_delay));
	}
public:
	FifoDelayedCaller(std::shared_ptr<std::queue<T>> queue, std::shared_ptr<std::unique_lock<std::mutex>> lock, std::shared_ptr<std::condition_variable> cv, std::function<T()> fn, UINT delay, std::shared_ptr<bool> consumerWaitingFlag)
		:FifoQueueProducerPredicate<T>(queue, lock, cv, fn, consumerWaitingFlag),
		 m_delay(delay)
	{}
};

// For timer utilities
template <class T>
class returnSameThing
{
	T m_object;

public:
	T operator()()
	{
		return m_object;
	}

	returnSameThing(T object) :
		m_object(object)
	{
	}

	const T& operator=(const returnSameThing& object)
	{
		m_object = object.m_object;
		return m_object;
	}

	const T& operator=(const T& object)
	{
		m_object = object.m_object;
		return m_object;
	}
};

template<typename T>
void callItAsAFunction(T obj)
{
	obj();
}
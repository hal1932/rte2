#pragma once
#include "common.h"
#include <functional>

namespace rte {

	// SWIGでC++11のスレッド系APIが使えないから、勉強も兼ねてそれっぽいのを自前で用意する

	class Thread RTE_FINAL
	{
	public:
		Thread();
		~Thread();

		Thread(Thread&) = delete;
		Thread(Thread&&) = delete;
		Thread& operator=(Thread&) = delete;

		void start(std::function<void(void*)> threadFunc, void* arg = nullptr);
		void join();
		void detach();

		int getId();
		bool isAlive();
	};

	class Mutex RTE_FINAL
	{
	public:
		Mutex();
		~Mutex() = default;

		Mutex(Mutex&) = delete;
		Mutex(Mutex&&) = delete;
		Mutex& operator=(Mutex&) = delete;

		void lock();
		bool tryLock();
		void unlock();
	};

	class UniqueLock RTE_FINAL
	{
	public:
		UniqueLock(Mutex& mutex) : mMutex(mutex) { mMutex.lock(); }
		~UniqueLock() { mMutex.unlock(); }

		UniqueLock() = delete;
		UniqueLock(UniqueLock&) = delete;
		UniqueLock(UniqueLock&&) = delete;
		UniqueLock& operator=(UniqueLock&) = delete;

	private:
		Mutex& mMutex;
	};

	class ConditionVariable RTE_FINAL
	{
	public:
		ConditionVariable() = default;
		~ConditionVariable() = default;

		ConditionVariable(ConditionVariable&) = delete;
		ConditionVariable(ConditionVariable&&) = delete;
		ConditionVariable& operator=(ConditionVariable&) = delete;

		void notifyOne();
		void notifyAll();

		void wait(UniqueLock& lock);
		void waitFor(UniqueLock& lock, int timeoutMilliSeconds, std::function<void()> pred);
	};

}// namespace rte

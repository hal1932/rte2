#pragma once
#include "common.h"
#include <functional>

namespace rte {

	// SWIG��C++11�̃X���b�h�nAPI���g���Ȃ�����A�׋������˂Ă�����ۂ��̂����O�ŗp�ӂ���

	class Thread RTE_FINAL : noncopyable, nonmovable
	{
	public:
		Thread();
		~Thread();

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

		Mutex(const Mutex&) = default;
		Mutex(Mutex&&) = default;
		Mutex& operator=(const Mutex&) = default;

		void lock();
		bool tryLock();
		void unlock();
	};

	class UniqueLock RTE_FINAL : nonmovable
	{
	public:
		UniqueLock(Mutex& mutex) : mMutex(mutex) { mMutex.lock(); }
		~UniqueLock() { mMutex.unlock(); }

		UniqueLock() = delete;
		UniqueLock(const UniqueLock&) = default;
		UniqueLock& operator=(const UniqueLock&) = default;

	private:
		Mutex& mMutex;
	};

	class ConditionVariable RTE_FINAL : noncopyable, nonmovable
	{
	public:
		ConditionVariable() = default;
		~ConditionVariable() = default;

		void notifyOne();
		void notifyAll();

		void wait(UniqueLock& lock);
		void waitFor(UniqueLock& lock, int timeoutMilliSeconds, std::function<void()> pred);
	};

}// namespace rte

#pragma once
#include "common.h"
#include <functional>

namespace rte {

	// SWIGでC++11のスレッド系APIが使えないから、勉強も兼ねてそれっぽいのを自前で用意する

	class Thread RTE_FINAL : private noncopyable, private nonmovable
	{
	public:
		Thread();
		~Thread();

		void start(std::function<unsigned int(void*)> threadFunc, void* arg = nullptr);
		unsigned int join();

		int getId();

	private:
		HANDLE mHandle;
	};

	class LockObject : private noncopyable, private nonmovable
	{
	public:
		virtual void lock() = 0;
		virtual bool tryLock() = 0;
		virtual void unlock() = 0;
		virtual bool isLocked() = 0;
	};

	class CriticalSection : public LockObject
	{
	public:
		CriticalSection();
		~CriticalSection();

		void lock();
		bool tryLock();
		void unlock();
		bool isLocked() { return mIsLocked; }

	private:
		CRITICAL_SECTION mCriticalSection;
		bool mIsLocked;
	};

	class UniqueLock RTE_FINAL : public LockObject
	{
	public:
		explicit UniqueLock(LockObject& lockObj, bool deferLock = false)
			: mLockObj(lockObj)
		{
			if (!deferLock)
			{
				lockObj.lock();
			}
		}
		~UniqueLock()
		{
			if (mLockObj.isLocked())
			{
				mLockObj.unlock();
			}
		}

		void lock() { mLockObj.lock(); }
		bool tryLock() { return mLockObj.tryLock(); }
		void unlock() { return mLockObj.unlock(); }
		bool isLocked() { return mLockObj.isLocked(); }

	private:
		LockObject& mLockObj;
	};

	class ConditionVariable RTE_FINAL : noncopyable, nonmovable
	{
	public:
		ConditionVariable();
		~ConditionVariable();

		void notifyOne();
		void wait(UniqueLock& lock);
		void wait(UniqueLock& lock, std::function<bool(void)> pred);

	private:
		HANDLE mHandle;
		CriticalSection mLock;
	};
}// namespace rte

#include "Thread.h"
#include <process.h>

namespace {

	struct ThreadInfo_
	{
		std::function<unsigned int(void*)> func;
		void* arg;
	};

	static unsigned int __stdcall threadFunc_(void* arg)
	{
		auto pInfo = reinterpret_cast<ThreadInfo_*>(arg);
		auto result = pInfo->func(pInfo->arg);
		rte::mem::safeDelete(&pInfo);
		_endthreadex(result);
		return result;
	}

}

namespace rte {

	Thread::Thread()
		: mHandle(nullptr)
	{ }

	Thread::~Thread()
	{
		assert(mHandle == nullptr);
	}

	void Thread::start(std::function<unsigned int(void*)> threadFunc, void* arg)
	{
		assert(mHandle == nullptr);

		auto pInfo = new ThreadInfo_();
		pInfo->func = threadFunc;
		pInfo->arg = arg;

		mHandle = reinterpret_cast<HANDLE>(_beginthreadex(
			nullptr, 0,
			threadFunc_, static_cast<void*>(pInfo),
			0, nullptr));
	}

	unsigned int Thread::join()
	{
		assert(mHandle != nullptr);

		auto result = WaitForSingleObject(mHandle, INFINITE);

		CloseHandle(mHandle);
		mHandle = nullptr;

		return static_cast<unsigned int>(result);
	}

	int Thread::getId()
	{
		return (mHandle == nullptr) ? 0 : reinterpret_cast<int>(mHandle);
	}

	/*------------------------------------------------------------------*/

	CriticalSection::CriticalSection()
		: mIsLocked(false)
	{
		InitializeCriticalSection(&mCriticalSection);
	}
	
	CriticalSection::~CriticalSection()
	{
		DeleteCriticalSection(&mCriticalSection);
	}

	void CriticalSection::lock()
	{
		EnterCriticalSection(&mCriticalSection);
		mIsLocked = true;
	}

	bool CriticalSection::tryLock()
	{
		auto result = TryEnterCriticalSection(&mCriticalSection);
		if (result == TRUE)
		{
			mIsLocked = true;
			return true;
		}
		return false;
	}

	void CriticalSection::unlock()
	{
		LeaveCriticalSection(&mCriticalSection);
		mIsLocked = false;
	}

	/*------------------------------------------------------------------*/

	ConditionVariable::ConditionVariable()
	{
		mHandle = CreateEvent(nullptr, TRUE, FALSE, nullptr);
		if (mHandle == nullptr)
		{
			logError(log::getLastErrorString(GetLastError()));
			throw new std::exception("failed to create ConditionVariable");
		}
	}

	ConditionVariable::~ConditionVariable()
	{
		if (mHandle != nullptr)
		{
			CloseHandle(mHandle);
		}
	}

	void ConditionVariable::notifyOne()
	{
		assert(mHandle != nullptr);

		mLock.lock();
		{
			SetEvent(mHandle);
		}
		mLock.unlock();
	}

	void ConditionVariable::wait(UniqueLock& lock)
	{
		assert(mHandle != nullptr);

		lock.unlock();
		{
			mLock.lock();
			{
				WaitForSingleObject(mHandle, INFINITE);
				ResetEvent(mHandle);
			}
			mLock.unlock();
		}
		lock.lock();
	}

	void ConditionVariable::wait(UniqueLock& lock, std::function<bool(void)> pred)
	{
		while (!pred())
		{
			wait(lock);
		}
	}

}// namespace rte

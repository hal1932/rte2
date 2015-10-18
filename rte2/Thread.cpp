#include "Thread.h"

namespace rte {

	Thread::Thread()
	{
	}

	Thread::~Thread()
	{
	}

	void Thread::start(std::function<void(void*)> threadFunc, void* arg)
	{
	}

	void Thread::join()
	{
	}

	void Thread::detach()
	{
	}

	int Thread::getId()
	{
		return -1;
	}

	bool Thread::isAlive()
	{
		return false;
	}

	/*------------------------------------------------------------------*/

	Mutex::Mutex()
	{
	}

	void Mutex::lock()
	{
	}

	bool Mutex::tryLock()
	{
		return false;
	}

	void Mutex::unlock()
	{
	}

	/*------------------------------------------------------------------*/

	void ConditionVariable::notifyOne()
	{
	}

	void ConditionVariable::notifyAll()
	{
	}

	void ConditionVariable::wait(UniqueLock& lock)
	{
	}

	void ConditionVariable::waitFor(UniqueLock& lock, int timeoutMilliSeconds, std::function<void()> pred)
	{
	}

}// namespace rte

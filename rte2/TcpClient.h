#pragma once
#include "tcpCommon.h"

#include "Thread.h"
#include <vector>
#include <functional>

namespace rte {

	class Socket;

	class TcpClient RTE_FINAL : private noncopyable, private nonmovable
	{
	public:
		TcpClient();
		~TcpClient();

		bool connect(const std::string& host, int port);
		void close();

		void sendAsync(const uint8_t* buffer, int bufferSize);

		std::vector<TcpReceivedData> popReceivedQueue();
		std::vector<TcpSentData> popSentQueue();

		bool isConnectionAlive() { return !mIsConnectionClosed && mConnectionThread.isRunning(); }

	private:
		Socket* mpSocket;
		CriticalSection mSocketLock;

		Thread mConnectionThread;
		bool mIsConnectionClosed;

		InterlockedVector<TcpSentData> mSendRequestList;

		InterlockedVector<TcpReceivedData> mReceivedList;
		InterlockedVector<TcpSentData> mSentList;

		int connectionThread_(void*);
		bool sendData_();
		bool receiveData_();
	};

}// namespace rte

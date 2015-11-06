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

		bool isConnectionAlive() { return !mIsConnectionClosed; }

	private:
		Socket* mpSocket;
		CriticalSection mSocketLock;

		Thread mReceiveThread;
		Thread mSendThread;

		InterlockedVector<TcpSentData> mSendRequestList;

		InterlockedVector<TcpReceivedData> mReceivedList;
		InterlockedVector<TcpSentData> mSentList;

		bool mIsConnectionClosed;

		int receiveThread_(void*);
		int sendThread_(void*);
	};

}// namespace rte

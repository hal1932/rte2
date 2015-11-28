#pragma once
#include "tcpCommon.h"
#include "Thread.h"
#include <vector>
#include <map>
#include <functional>
#include <chrono>

namespace rte {

	class Socket;

	class TcpServer RTE_FINAL : private noncopyable, private nonmovable
	{
	public:
		TcpServer();
		~TcpServer();

		bool open(int port);
		void close();

		bool cleanupInvalidConnection();

		void setKeepAliveInterval(int seconds) { mKeepAliveIntervalSeconds = seconds; }

		void sendAsync(int id, const uint8_t* buffer, int bufferSize);
		void broadcastAsync(const uint8_t* buffer, int bufferSize);

		int getClientCount() { return mConnectionDic.size(); }
		std::vector<int> getClientList();

		std::vector<int> popAcceptedQueue();
		std::vector<TcpReceivedData> popReceivedQueue();
		std::vector<TcpSentData> popSentQueue();
		std::vector<int> popClosedQueue();

		void closeConnection(int clientId);

	private:
		Socket* mpSocket;

		int mKeepAliveIntervalSeconds;

		Thread mAcceptThread;
		std::map<Socket*, Thread*> mConnectionDic;
		InterlockedVector<Socket*> mCleanupRequestList;

		InterlockedVector<TcpSentData> mSendRequestList;

		InterlockedVector<int> mAcceptedList;
		InterlockedVector<TcpReceivedData> mReceivedList;
		InterlockedVector<int> mClosedList;
		InterlockedVector<int> mCloseRequestList;
		InterlockedVector<TcpSentData> mSentList;

		volatile bool mIsConnectionClosed;

		int acceptThread_(void*);
		int connectionThread_(void*);
		bool sendData_(Socket* pClient);
		bool receiveData_(Socket* pClient);
		bool checkKeepAlive_(Socket* pClient, std::chrono::system_clock::time_point* pLastCheckKeepAlive);
	};

}// namespace rte

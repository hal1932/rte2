#pragma once
#include "tcpCommon.h"
#include "Thread.h"
#include <vector>
#include <map>
#include <functional>

namespace rte {

	class Socket;

	class TcpServer RTE_FINAL : private noncopyable, private nonmovable
	{
	public:
		TcpServer();
		~TcpServer();

		bool open(int port);
		void close();

		void sendAsync(int id, const uint8_t* buffer, int bufferSize);
		void broadcastAsync(const uint8_t* buffer, int bufferSize);

		int getClientCount() { return mClientDic.size(); }
		std::vector<int> getClientList();

		std::vector<int> popAcceptedQueue();
		std::vector<TcpReceivedData> popReceivedQueue();
		std::vector<TcpSentData> popSentQueue();
		std::vector<int> popClosedQueue();

		void closeConnection(int id);

	private:
		Socket* mpSocket;

		Thread mAcceptThread;
		Thread mSendThread;

		struct ClientInfo
		{
			Thread* pReceiveThread;
			CriticalSection* mpLock;
		};
		std::map<Socket*, ClientInfo> mClientDic;

		rte::InterlockedVector<int> mAcceptedList;
		rte::InterlockedVector<TcpReceivedData> mReceivedList;
		rte::InterlockedVector<int> mClosedList;
		rte::InterlockedVector<int> mCloseRequestList;
		rte::InterlockedVector<TcpSentData> mSentList;

		volatile bool mIsConnectionClosed;

		unsigned int acceptThread_(void*);
		unsigned int receiveThread_(void* arg);
		unsigned int sendThread_(void*);
	};

}// namespace rte

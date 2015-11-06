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
			CriticalSection* pLock;
		};
		std::map<Socket*, ClientInfo> mClientDic;

		InterlockedVector<TcpSentData> mSendRequestList;

		InterlockedVector<int> mAcceptedList;
		InterlockedVector<TcpReceivedData> mReceivedList;
		InterlockedVector<int> mClosedList;
		InterlockedVector<int> mCloseRequestList;
		InterlockedVector<TcpSentData> mSentList;

		volatile bool mIsConnectionClosed;

		int acceptThread_(void*);
		int receiveThread_(void* arg);
		int sendThread_(void*);
	};

}// namespace rte

#pragma once
#include "common.h"
#include "Thread.h"
#include <map>

namespace rte {

	class Socket;

	class TcpServer RTE_FINAL
	{
	public:
		typedef void (*OnAcceptClient)(int id);
		typedef void (*OnReceiveData)(int id, const uint8_t* buffer, int bufferSize);
		typedef void (*OnCloseConnection)(int id);

		struct Config
		{
			uint16_t portBegin;
			uint16_t portEnd;
			OnAcceptClient onAcceptClient;
			OnReceiveData onReceiveData;
			OnCloseConnection onCloseConnection;
		};

		struct SendResult
		{
			int id;
			int sendBytes;
		};

	public:
		TcpServer();
		~TcpServer();

		TcpServer(TcpServer&) = delete;
		TcpServer& operator=(TcpServer&) = delete;

		bool configure(const Config& config);

		bool open(int port);
		void close();

		SendResult send(int id, const uint8_t* buffer, int bufferSize);
		std::vector<SendResult> broadcast(const uint8_t* buffer, int bufferSize);

		void closeConnection(int id);

	private:
		Config mConfig;
		int mNextPort;

		Socket* mpSocket;

		Thread mAcceptThread;
		std::map<Socket*, Thread*> mClientThreadDic;
		std::vector<int> mCloseRequestList;
	};

}// namespace rte

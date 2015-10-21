#pragma once
#include "common.h"
#include "Thread.h"
#include <map>

namespace rte {

	class Socket;

	class TcpServer RTE_FINAL : noncopyable, nonmovable
	{
	public:
		typedef void (*OnAcceptClient)(int id);
		typedef void (*OnSendData)(int id, const uint8_t* buffer, int bufferSize, int sendBytes);
		typedef void (*OnReceiveData)(int id, const uint8_t* buffer, int bufferSize);
		typedef void (*OnCloseConnection)(int id);

		struct Config
		{
			OnAcceptClient onAcceptClient;
			OnSendData onSendData;
			OnReceiveData onReceiveData;

			Config()
				: onAcceptClient(nullptr),
				  onSendData(nullptr),
				  onReceiveData(nullptr)
			{ }
		};

	public:
		TcpServer();
		~TcpServer();

		bool configure(const Config& config);

		bool open(int port);
		void close();

		void sendAsync(int id, const uint8_t* buffer, int bufferSize);
		void broadcastAsync(const uint8_t* buffer, int bufferSize);

		void closeConnection(int id);

	private:
		Config mConfig;

		Socket* mpSocket;

		Thread mAcceptThread;

		struct ClientInfo
		{
			Thread* pReceiveThread;
			Mutex* mpLock;
		};
		std::map<Socket*, ClientInfo> mClientDic;

		Thread mSendThread;

		std::vector<int> mCloseRequestList;
		Mutex mCloseRequestLock;

		volatile bool mIsConnectionClosed;

		struct SendData
		{
			Socket* pClientSocket;
			const uint8_t* buffer;
			int bufferSize;
		};
		std::vector<SendData> mSendDataList;

		void acceptThread_(void*);
		void receiveThread_(void* arg);
		void sendThread_(void*);
	};

}// namespace rte

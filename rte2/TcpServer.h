#pragma once
#include "common.h"
#include "Thread.h"
#include <map>

namespace rte {

	class Socket;

	struct TcpServerConfig
	{
		typedef void(*OnAcceptClient)(int id);
		typedef void(*OnSendData)(int id, const uint8_t* buffer, int bufferSize, int sendBytes);
		typedef void(*OnReceiveData)(int id, const uint8_t* buffer, int bufferSize);
		typedef bool(*OnConnectionError)(int id, const uint8_t* buffer, int bufferSize);

		OnAcceptClient onAcceptClient;
		OnSendData onSendData;
		OnReceiveData onReceiveData;
		OnConnectionError onConnectionError;

		TcpServerConfig()
			: onAcceptClient(nullptr),
			  onSendData(nullptr),
			  onReceiveData(nullptr),
			  onConnectionError(nullptr)
		{ }
	};

	class TcpServer RTE_FINAL : private noncopyable, private nonmovable
	{
	public:
		TcpServer();
		~TcpServer();

		bool configure(const TcpServerConfig& config);

		bool open(int port);
		void close();

		void sendAsync(int id, const uint8_t* buffer, int bufferSize);
		void broadcastAsync(const uint8_t* buffer, int bufferSize);

		void closeConnection(int id);

	private:
		TcpServerConfig mConfig;

		Socket* mpSocket;

		Thread mAcceptThread;

		struct ClientInfo
		{
			Thread* pReceiveThread;
			CriticalSection* mpLock;
		};
		std::map<Socket*, ClientInfo> mClientDic;

		Thread mSendThread;

		std::vector<int> mCloseRequestList;
		CriticalSection mCloseRequestLock;

		volatile bool mIsConnectionClosed;

		struct SendData
		{
			Socket* pClientSocket;
			const uint8_t* buffer;
			int bufferSize;
		};
		std::vector<SendData> mSendDataList;
		CriticalSection mSendDataLock;

		unsigned int acceptThread_(void*);
		unsigned int receiveThread_(void* arg);
		unsigned int sendThread_(void*);
	};

}// namespace rte

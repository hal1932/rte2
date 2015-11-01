#pragma once
#include "common.h"
#include "Thread.h"
#include <vector>

namespace rte {

	class Socket;

	struct TcpClientConfig
	{
		typedef void(*OnSendData)(const uint8_t* buffer, int bufferSize);
		typedef void(*OnReceiveData)(const uint8_t* buffer, int bufferSize);
		typedef bool(*OnConnectionError)(const uint8_t* buffer, int bufferSize);

		OnSendData onSendData;
		OnReceiveData onReceiveData;
		OnConnectionError onConnectionError;

		TcpClientConfig()
			: onSendData(nullptr),
			  onReceiveData(nullptr),
			  onConnectionError(nullptr)
		{ }
	};

	class TcpClient RTE_FINAL : private noncopyable, private nonmovable
	{
	public:
		TcpClient();
		~TcpClient();

		bool configure(const TcpClientConfig& config);

		bool connect(const std::string& host, int port);
		void close();

		void sendAsync(const uint8_t* buffer, int bufferSize);

	private:
		TcpClientConfig mConfig;

		Socket* mpSocket;
		CriticalSection mSocketLock;

		Thread mReceiveThread;
		Thread mSendThread;

		bool mIsConnectionClosed;

		struct SendData
		{
			const uint8_t* buffer;
			int bufferSize;
		};
		std::vector<SendData> mSendDataList;
		CriticalSection mSendDataLock;

		unsigned int receiveThread_(void*);
		unsigned int sendThread_(void*);
	};

}// namespace rte

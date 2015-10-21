#pragma once
#include "common.h"
#include "Thread.h"
#include <vector>

namespace rte {

	class Socket;

	class TcpClient RTE_FINAL : noncopyable, nonmovable
	{
	public:
		typedef void (*OnSendData)(const uint8_t* buffer, int bufferSize);
		typedef void (*OnReceiveData)(const uint8_t* buffer, int bufferSize);
		typedef bool (*OnConnectionError)(const uint8_t* buffer, int bufferSize);

		struct Config
		{
			OnSendData onSendData;
			OnReceiveData onReceiveData;
			OnConnectionError onConnectionError;

			Config()
				: onSendData(nullptr),
				  onReceiveData(nullptr),
				  onConnectionError(nullptr)
			{ }
		};

	public:
		TcpClient();
		~TcpClient();

		bool configure(const Config& config);

		bool connect(const std::string& host, int port);
		void close();

		void sendAsync(const uint8_t* buffer, int bufferSize);

	private:
		Config mConfig;

		Socket* mpSocket;
		Mutex mSocketLock;

		Thread mReceiveThread;
		Thread mSendThread;

		bool mIsConnectionClosed;

		struct SendData
		{
			const uint8_t* buffer;
			int bufferSize;
		};
		std::vector<SendData> mSendDataList;
		Mutex mSendDataLock;

		void receiveThread_(void*);
		void sendThread_(void*);
	};

}// namespace rte

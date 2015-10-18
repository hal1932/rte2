#pragma once
#include "common.h"

namespace rte {

	class Socket RTE_FINAL
	{
	public:
		enum class ProtocolType : uint8_t
		{
			Invalid,
			Tcp,
		};

	public:
		Socket();
		~Socket() = default;

		Socket(Socket&) = delete;
		Socket(Socket&&) = delete;
		Socket& operator=(Socket&) = delete;

		static bool setup();
		static void shutdown();

		// 共通
		bool configure(ProtocolType type);

		int send(const uint8_t* buffer, int bufferSize);
		int recv(uint8_t* buffer, int bufferSize);
		void close();

		void setBlocking(bool enable);
		int getAvailabieSize();

		ProtocolType getProtocolType() { return mProtocolType; }
		const std::string& getHost() { return mHost; }
		int getPort() { return mPort; }

		// サーバ
		bool bind(int port);
		bool listen(int backlog = 1);
		TriBool accept(Socket* pAccepted);

		// クライアント
		bool connect(const std::string& host, int port);

	private:
		static WSADATA sWsaData;

		SOCKET mSocket;
		struct sockaddr_in mSockAddr;

		ProtocolType mProtocolType;
		std::string mHost;
		int mPort;
	};

}// namespace rte

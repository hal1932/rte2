#include "Socket.h"
#include <WS2tcpip.h>

namespace {
	void handleWsaError_(int err = 0xFFFFFFFF)
	{
		if (err == 0xFFFFFFFF)
		{
			err = WSAGetLastError();
		}
		if (err == NO_ERROR)
		{
			return;
		}
		logError(rte::log::getLastErrorString(err));
	}
}

namespace rte {

	WSADATA Socket::sWsaData;

	Socket::Socket()
		: mSocket(INVALID_SOCKET)
		, mProtocolType(Socket::ProtocolType::Invalid)
		, mHost("")
		, mPort(-1)
	{}

	bool Socket::setup()
	{
		sWsaData.wVersion = 0xFFFF;
		auto result = WSAStartup(MAKEWORD(2, 0), &sWsaData);
		if (result == SOCKET_ERROR)
		{
			handleWsaError_();
			return false;
		}
		return true;
	}

	void Socket::shutdown()
	{
		if (sWsaData.wVersion != 0xFFFF)
		{
			WSACleanup();
		}
	}

	bool Socket::configure(Socket::ProtocolType type)
	{
		assert(mSocket == INVALID_SOCKET);

		switch (type)
		{
		case Socket::ProtocolType::Tcp:
			mSocket = socket(AF_INET, SOCK_STREAM, 0);
			mSockAddr.sin_family = AF_INET;
			break;

		default:
			logError("invalid socket type");
			assert(false);
		}

		if (mSocket == INVALID_SOCKET)
		{
			handleWsaError_();
			return false;
		}
		return true;
	}

	int Socket::send(const uint8_t* buffer, int bufferSize)
	{
		assert(mSocket != INVALID_SOCKET);

		auto sendBytes = ::send(mSocket, reinterpret_cast<const char*>(buffer), bufferSize, 0);
		if (sendBytes == SOCKET_ERROR)
		{
			handleWsaError_();
		}
		return sendBytes;
	}

	int Socket::recv(uint8_t* buffer, int bufferSize)
	{
		assert(mSocket != INVALID_SOCKET);

		auto recvBytes = ::recv(mSocket, reinterpret_cast<char*>(buffer), bufferSize, 0);
		if (recvBytes == SOCKET_ERROR)
		{
			auto err = WSAGetLastError();
			if (err == WSAEWOULDBLOCK)
			{
				return 0;
			}
			handleWsaError_(err);
		}
		return recvBytes;
	}

	void Socket::close()
	{
		assert(mSocket != INVALID_SOCKET);

		closesocket(mSocket);
		mSocket = INVALID_SOCKET;
	}

	void Socket::setBlocking(bool enable)
	{
		assert(mSocket != INVALID_SOCKET);

		u_long value = (enable) ? 1 : 0;
		ioctlsocket(mSocket, FIONBIO, &value);
	}

	int Socket::getAvailabieSize()
	{
		assert(mSocket != INVALID_SOCKET);

		int size;
		auto result = ioctlsocket(mSocket, FIONREAD, reinterpret_cast<u_long*>(&size));
		if (result == SOCKET_ERROR)
		{
			handleWsaError_();
			return -1;
		}
		return size;
	}

	bool Socket::bind(int port)
	{
		assert(mSocket != INVALID_SOCKET);

		// sin_family ÇÕ configure ÇÃÇ∆Ç´Ç…ê›íËçœÇ›
		mSockAddr.sin_port = htons(static_cast<u_short>(port));
		mSockAddr.sin_addr.S_un.S_addr = INADDR_ANY;

		auto result = ::bind(mSocket, reinterpret_cast<const sockaddr*>(&mSockAddr), sizeof(mSockAddr));
		if (result == SOCKET_ERROR)
		{
			handleWsaError_();
			return false;
		}
		return true;
	}

	bool Socket::listen(int backlog)
	{
		assert(mSocket != INVALID_SOCKET);

		auto result = ::listen(mSocket, backlog);
		if (result == SOCKET_ERROR)
		{
			handleWsaError_();
			return false;
		}
		return true;
	}

	TriBool Socket::accept(Socket* pAccepted)
	{
		assert(mSocket != INVALID_SOCKET);

		struct sockaddr_in client;
		int len = sizeof(client);
		auto clientSocket = ::accept(mSocket, reinterpret_cast<sockaddr*>(&client), &len);
		if (clientSocket == INVALID_SOCKET)
		{
			auto err = WSAGetLastError();
			if (err == WSAEWOULDBLOCK)
			{
				return TriBool::Unknown;
			}
			handleWsaError_(err);
			return TriBool::False;
		}

		pAccepted->mSocket = clientSocket;
		pAccepted->mSockAddr = client;
		pAccepted->mProtocolType = mProtocolType;
		pAccepted->mPort = client.sin_port;

		char hostname[32];
		inet_ntop(AF_INET, &client.sin_addr.S_un.S_addr, hostname, sizeof(hostname));
		pAccepted->mHost = std::string(hostname);

		return TriBool::True;
	}

	bool Socket::connect(const std::string& host, int port)
	{
		assert(mSocket != INVALID_SOCKET);

		// sin_family ÇÕ configure ÇÃÇ∆Ç´Ç…ê›íËçœÇ›
		inet_pton(AF_INET, host.c_str(), &mSockAddr.sin_addr);
		mSockAddr.sin_port = htons(static_cast<u_short>(port));

		auto result = ::connect(mSocket, reinterpret_cast<const sockaddr*>(&mSockAddr), sizeof(mSockAddr));
		if (result == SOCKET_ERROR)
		{
			handleWsaError_();
			return false;
		}
		return true;
	}

}// namespace rte

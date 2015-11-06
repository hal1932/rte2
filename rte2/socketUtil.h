#pragma once
#include "common.h"
#include "Socket.h"

namespace rte {
	namespace socketUtil {

		/// 受信キューが空になるまで受信
		inline mem::Array<uint8_t> receive(Socket* pSocket)
		{
			const int cBufferSize = 1024;

			mem::Array<uint8_t> received(cBufferSize);
			auto recvBytes = pSocket->recv(received.get(), received.size());
			if (recvBytes < 0)
			{
				received.invalidate();
				return std::move(received);
			}

			received.resize(recvBytes);

			if (recvBytes > 0)
			{
				mem::Array<uint8_t> tmp(cBufferSize);
				while (pSocket->getAvailabieSize() > 0)
				{
					recvBytes = pSocket->recv(tmp.get(), tmp.size());
					if (recvBytes > 0)
					{
						tmp.resize(recvBytes);
						received.append(std::move(tmp));
					}
				}
			}

			return std::move(received);
		}

	}// namespace socketUtil
}// namespace rte

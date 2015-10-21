#pragma once
#include "common.h"
#include "Socket.h"

namespace rte {
	namespace socketUtil {

		/// 受信キューが空になるまで受信
		mem::SafeArray<uint8_t> receive(Socket* pSocket)
		{
			const int cBufferSize = 1024;

			mem::SafeArray<uint8_t> received(cBufferSize);
			auto recvBytes = pSocket->recv(received.get(), received.size());
			received.resize(recvBytes);

			if (recvBytes > 0)
			{
				mem::SafeArray<uint8_t> tmp(cBufferSize);
				while (pSocket->getAvailabieSize() > 0)
				{
					recvBytes = pSocket->recv(tmp.get(), tmp.size());
					if (recvBytes > 0)
					{
						tmp.resize(recvBytes);
						received.append(tmp.get(), tmp.size());
					}
				}
			}

			return std::move(received);
		}

	}// namespace socketUtil
}// namespace rte

#pragma once
#include "common.h"
#include "Socket.h"

namespace rte {
	namespace socketUtil {

		/// 受信キューが空になるまで受信
		inline int receive(mem::Array<uint8_t>* pReceivedData, Socket* pSocket)
		{
			if (pSocket->getAvailabieSize() == 0)
			{
				return 0;
			}

			const int cBufferSize = 1024;
			pReceivedData->resize(cBufferSize);

			auto recvBytes = pSocket->recv(pReceivedData->get(), pReceivedData->size());
			if (recvBytes <= 0)
			{
				pReceivedData->invalidate();
				return -1;
			}

			pReceivedData->resize(recvBytes);

			while (pSocket->getAvailabieSize() > 0)
			{
				mem::Array<uint8_t> tmp(cBufferSize);
				recvBytes = pSocket->recv(tmp.get(), tmp.size());
				if (recvBytes > 0)
				{
					tmp.resize(recvBytes);
					pReceivedData->append(std::move(tmp));
				}
			}

			return pReceivedData->size();
		}

		inline bool sendReceivedConfirmation(Socket* pSocket)
		{
			uint8_t confirm[1];
			auto confirmBytes = pSocket->send(confirm, 1);
			return (confirmBytes == 1);
		}

		inline bool receiveReceivedConfirmation(Socket* pSocket)
		{
			uint8_t confirm[1];
			int confirmBytes = 0;
			while (confirmBytes == 0)
			{
				confirmBytes = pSocket->recv(confirm, 1);
				Sleep(5);
			}
			return (confirmBytes == 1);
		}

		inline void handleWsaError(const char* msg = nullptr, int err = 0xFFFFFFFF)
		{
			if (err == 0xFFFFFFFF)
			{
				err = WSAGetLastError();
			}
			if (err == NO_ERROR)
			{
				return;
			}
			std::string s(msg);
			if (s.length() > 0) s += ": ";
			logError(s + rte::log::getLastErrorString(err));
		}

	}// namespace socketUtil
}// namespace rte

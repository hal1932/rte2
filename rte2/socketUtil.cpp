#include "socketUtil.h"

namespace rte
{
	namespace socketUtil
	{

		/// 受信キューが空になるまで受信
		int receive(mem::Array<uint8_t>* pReceivedData, Socket* pSocket)
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

		bool sendReceivedConfirmation(Socket* pSocket)
		{
			uint8_t confirm[] = { '@' };
			auto confirmBytes = pSocket->send(confirm, 1);
			return (confirmBytes == 1);
		}

		bool receiveReceivedConfirmation(Socket* pSocket)
		{
			uint8_t confirm[1];
			int confirmBytes = 0;
			while (true)
			{
				confirmBytes = pSocket->recv(confirm, 1);
				if (confirmBytes == 0)
				{
					Sleep(5);
				}
				else
				{
					break;
				}
			}
			return (confirmBytes == 1 && confirm[0] == '@');
		}

		bool sendKeepAlive(Socket* pSocket)
		{
			uint8_t keepAlive[] = { '$' };
			auto sentBytes = pSocket->send(keepAlive, 1);
			if (sentBytes != 1)
			{
				logInfo("failed to send keep-alive");
				return false;
			}

			int recvBytes = 0;
			while (true)
			{
				recvBytes = pSocket->recv(keepAlive, 1);
				if (recvBytes == 0)
				{
					Sleep(5);
				}
				else
				{
					break;
				}
			}
			return (recvBytes == 1 && keepAlive[0] == '$');
		}

		bool isKeepAlive(const mem::Array<uint8_t>& data)
		{
			return (data.get()[0] == '$' && data.size() == 1);
		}

		bool replyKeepAlive(Socket* pSocket)
		{
			uint8_t keepAlive[] = { '$' };
			return (pSocket->send(keepAlive, 1) == 1);
		}

	}// namespace socketUtil
}// namespace rte

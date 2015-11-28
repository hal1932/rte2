#include "tcpCommon.h"
#include "socketUtil.h"

namespace rte {
	
	bool sendDataToSocket(Socket* pSocket, TcpSentData* pData)
	{
		// 送信
		pData->sentSize = pSocket->send(pData->buffer, pData->bufferSize);
		if (pData->sentSize == pData->bufferSize)
		{
			// サーバ側からの受信通知のチェック
			if (socketUtil::receiveReceivedConfirmation(pSocket))
			{
				return true;
			}
			else
			{
				logInfo("failed to receive confirmation: " + pData->toString());
				return false;
			}
		}
		else
		{
			// 送信失敗
			logInfo("failed to send data: " + pData->toString());
			return true;
		}
	}

	bool receiveDataFromSocket(TcpReceivedData* pOut, Socket* pSocket)
	{
		mem::Array<uint8_t> tmpReceivedData;

		// 受信
		auto receivedSize = socketUtil::receive(&tmpReceivedData, pSocket);
		if (receivedSize == 0)
		{
			// 受信するものがなかった
			pOut->bufferSize = 0;
			return true;
		}
		else if (receivedSize < 0)
		{
			// 受信失敗
			logInfo("failed to receive data");
			return false;
		}

		// keep-alive かどうかチェック
		if (socketUtil::isKeepAlive(tmpReceivedData))
		{
			return socketUtil::replyKeepAlive(pSocket);
		}

		// 受信通知を送信
		if (socketUtil::sendReceivedConfirmation(pSocket))
		{
			pOut->buffer = tmpReceivedData.get();
			pOut->bufferSize = tmpReceivedData.size();
			return true;
		}

		assert(false);
	}

}
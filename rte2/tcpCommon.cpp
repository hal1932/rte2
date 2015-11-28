#include "tcpCommon.h"
#include "socketUtil.h"

namespace rte {
	
	bool sendDataToSocket(Socket* pSocket, TcpSentData* pData)
	{
		// ���M
		pData->sentSize = pSocket->send(pData->buffer, pData->bufferSize);
		if (pData->sentSize == pData->bufferSize)
		{
			// �T�[�o������̎�M�ʒm�̃`�F�b�N
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
			// ���M���s
			logInfo("failed to send data: " + pData->toString());
			return true;
		}
	}

	bool receiveDataFromSocket(TcpReceivedData* pOut, Socket* pSocket)
	{
		mem::Array<uint8_t> tmpReceivedData;

		// ��M
		auto receivedSize = socketUtil::receive(&tmpReceivedData, pSocket);
		if (receivedSize == 0)
		{
			// ��M������̂��Ȃ�����
			pOut->bufferSize = 0;
			return true;
		}
		else if (receivedSize < 0)
		{
			// ��M���s
			logInfo("failed to receive data");
			return false;
		}

		// keep-alive ���ǂ����`�F�b�N
		if (socketUtil::isKeepAlive(tmpReceivedData))
		{
			return socketUtil::replyKeepAlive(pSocket);
		}

		// ��M�ʒm�𑗐M
		if (socketUtil::sendReceivedConfirmation(pSocket))
		{
			pOut->buffer = tmpReceivedData.get();
			pOut->bufferSize = tmpReceivedData.size();
			return true;
		}

		assert(false);
	}

}
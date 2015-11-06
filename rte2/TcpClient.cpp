#include "TcpClient.h"
#include "Socket.h"
#include "socketUtil.h"

namespace {

	static const int cThreadPollingInterval = 10;

}

namespace rte {

	TcpClient::TcpClient()
		: mpSocket(nullptr)
	{ }

	TcpClient::~TcpClient()
	{
		assert(mIsConnectionClosed);
		assert(mpSocket == nullptr);
	}

	bool TcpClient::connect(const std::string& host, int port)
	{
		assert(mpSocket == nullptr);

		mpSocket = new Socket();
		if (!mpSocket->configure(Socket::ProtocolType::Tcp))
		{
			mem::safeDelete(&mpSocket);
			return false;
		}

		if (!mpSocket->connect(host, port))
		{
			return false;
		}
		mpSocket->setBlocking(true);

		mIsConnectionClosed = false;
		mReceiveThread.start(std::bind(&TcpClient::receiveThread_, this, std::placeholders::_1));
		mSendThread.start(std::bind(&TcpClient::sendThread_, this, std::placeholders::_1));

		return true;
	}

	void TcpClient::close()
	{
		assert(mpSocket != nullptr);

		mIsConnectionClosed = true;

		mReceiveThread.join();
		mSendThread.join();

		mpSocket->close();
		mem::safeDelete(&mpSocket);
	}

	void TcpClient::sendAsync(const uint8_t* buffer, int bufferSize)
	{
		assert(mpSocket != nullptr);

		TcpSentData data;
		data.buffer = const_cast<uint8_t*>(buffer);
		data.bufferSize = bufferSize;
		mSendRequestList.emplaceBack(std::move(data));
	}

	std::vector<TcpReceivedData> TcpClient::popReceivedQueue()
	{
		std::vector<TcpReceivedData> result;
		mReceivedList.swap(&result);
		return std::move(result);
	}

	std::vector<TcpSentData> TcpClient::popSentQueue()
	{
		std::vector<TcpSentData> result;
		mSentList.swap(&result);
		return std::move(result);
	}

	int TcpClient::receiveThread_(void*)
	{
		while (true)
		{
			// ��~�`�F�b�N
			if (mIsConnectionClosed)
			{
				break;
			}

			// �f�[�^��M
			mem::Array<uint8_t> receivedData;
			{
				UniqueLock lock(mSocketLock);
				receivedData = std::move(socketUtil::receive(mpSocket));
			}

			if (receivedData.size() > 0)
			{
				// ��M�f�[�^���L���[�ɋl�߂�
				TcpReceivedData result;
				result.buffer = receivedData.get();
				result.bufferSize = receivedData.size();

				mReceivedList.emplaceBack(std::move(result));
			}
			else if (receivedData.size() == 0)
			{
				// �������Ȃ�
			}
			else
			{
				// �G���[
				logError("receiving from server failed");

				// �s���f�[�^���L���[�ɋl�߂�
				TcpReceivedData result;
				result.buffer = nullptr;
				result.bufferSize = -1;

				mReceivedList.emplaceBack(std::move(result));
			}

			Sleep(cThreadPollingInterval);
		}

		return 0;
	}

	int TcpClient::sendThread_(void*)
	{
		while (true)
		{
			// ��~�`�F�b�N
			if (mIsConnectionClosed)
			{
				break;
			}

			if (mSendRequestList.size() > 0)
			{
				// �L���[����ɂȂ�܂ő��M
				std::vector<TcpSentData> dataList;
				mSendRequestList.swap(&dataList);

				for (auto data : dataList)
				{
					int sentSize;
					{
						UniqueLock lock(mSocketLock);
						sentSize = mpSocket->send(data.buffer, data.bufferSize);
					}

					if (sentSize == data.bufferSize)
					{
						// ���M�f�[�^���L���[�ɋl�߂�
						TcpSentData result;
						result.buffer = data.buffer;
						result.bufferSize = data.bufferSize;
						result.sentSize = sentSize;
						mSentList.emplaceBack(std::move(result));
					}
					else
					{
						// �G���[
						logError("sending to server failed");

						// �s���f�[�^���L���[�ɋl�߂�
						TcpSentData result;
						result.buffer = nullptr;
						result.bufferSize = -1;
						mSentList.emplaceBack(std::move(result));
					}
				}
			}

			Sleep(cThreadPollingInterval);
		}

		return 0;
	}

}// namespace rte

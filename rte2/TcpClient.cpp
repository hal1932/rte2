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
		mConnectionThread.start(std::bind(&TcpClient::connectionThread_, this, std::placeholders::_1));

		return true;
	}

	void TcpClient::close()
	{
		assert(mpSocket != nullptr);

		mIsConnectionClosed = true;
		mConnectionThread.join();

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

	int TcpClient::connectionThread_(void*)
	{
		while (true)
		{
			// 停止チェック
			if (mIsConnectionClosed)
			{
				return 0;
			}

			// 受信
			if (!sendData_())
			{
				break;
			}

			// 送信
			if (!receiveData_())
			{
				break;
			}

			Sleep(cThreadPollingInterval);
		}

		return -1;
	}

	bool TcpClient::sendData_()
	{
		if (mSendRequestList.size() == 0)
		{
			return true;
		}

		std::vector<TcpSentData> dataList;
		mSendRequestList.swap(&dataList);

		auto success = true;

		for (auto data : dataList)
		{
			if (sendDataToSocket(mpSocket, &data))
			{
				mSentList.emplaceBack(std::move(data));
				success &= true;
			}
			else
			{
				// 送信失敗
				logInfo("failed to send data: " + data.toString());
				success = false;
			}
		}

		return success;
	}

	bool TcpClient::receiveData_()
	{
		TcpReceivedData result;
		if (receiveDataFromSocket(&result, mpSocket))
		{
			if (result.bufferSize > 0)
			{
				//printf("received: %d\n", result.bufferSize);
				mReceivedList.emplaceBack(std::move(result));
			}
			return true;
		}
		return false;
	}

#if false
	int TcpClient::receiveThread_(void*)
	{
		mem::Array<uint8_t> tmpReceivedData;

		while (true)
		{
			// 停止チェック
			if (mIsConnectionClosed)
			{
				break;
			}

			// データ受信
			auto receivedSize = 0;
			{
				UniqueLock lock(mSocketLock);
				receivedSize = socketUtil::receive(&tmpReceivedData, mpSocket);

				if (receivedSize > 0)
				{
					if (socketUtil::isKeepAlive(tmpReceivedData))
					{
						socketUtil::replyKeepAlive(mpSocket);
						logInfo("reply keep-alive");
						receivedSize = 0;
					}
					else
					{
						// 受信通知を送信
						if (!socketUtil::sendReceivedConfirmation(mpSocket))
						{
							logError("sending receive-confirmation failed");
							receivedSize = -1;
						}
					}
				}
			}

			if (receivedSize > 0)
			{
				// 受信データをキューに詰める
				TcpReceivedData result;
				result.buffer = tmpReceivedData.get();
				result.bufferSize = tmpReceivedData.size();

				mReceivedList.emplaceBack(std::move(result));
			}
			else if (receivedSize == 0)
			{
				// 何もしない
			}
			else
			{
				// エラー
				logError("receiving from server failed");
				close();
				break;
			}

			Sleep(cThreadPollingInterval);
		}

		return 0;
	}

	int TcpClient::sendThread_(void*)
	{
		while (true)
		{
			// 停止チェック
			if (mIsConnectionClosed)
			{
				break;
			}

			if (mSendRequestList.size() > 0)
			{
				// キューが空になるまで送信
				std::vector<TcpSentData> dataList;
				mSendRequestList.swap(&dataList);

				for (auto data : dataList)
				{
					int sentSize;
					{
						UniqueLock lock(mSocketLock);
						sentSize = mpSocket->send(data.buffer, data.bufferSize);

						if (sentSize == data.bufferSize)
						{
							// 受信通知のチェック
							if (!socketUtil::receiveReceivedConfirmation(mpSocket))
							{
								logError("receiving receive-confirmation failed");
								sentSize = 0;
							}
						}
					}

					if (sentSize == data.bufferSize)
					{
						TcpSentData result;
						result.buffer = const_cast<uint8_t*>(data.buffer);
						result.bufferSize = data.bufferSize;
						result.sentSize = sentSize;
						mSentList.emplaceBack(std::move(result));
					}
					else
					{
						// エラー
						logError("sending to client failed");
						close();
						break;
					}
				}
			}

			Sleep(cThreadPollingInterval);
		}

		return 0;
	}
#endif

}// namespace rte

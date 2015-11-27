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
					// 受信通知を送信
					if (!socketUtil::sendReceivedConfirmation(mpSocket))
					{
						logError("sending receive-confirmation failed");
						receivedSize = -1;
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
RECEIVE_ERROR:
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

}// namespace rte

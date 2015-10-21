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
		assert(!mReceiveThread.isAlive());
		assert(mpSocket == nullptr);
	}

	bool TcpClient::configure(const TcpClient::Config& config)
	{
		assert(mpSocket == nullptr);

		mConfig = config;

		mpSocket = new Socket();
		if (!mpSocket->configure(Socket::ProtocolType::Tcp))
		{
			mem::safeDelete(&mpSocket);
			return false;
		}

		return true;
	}

	bool TcpClient::connect(const std::string& host, int port)
	{
		assert(mpSocket != nullptr);

		if (!mpSocket->connect(host, port))
		{
			return false;
		}

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

		UniqueLock lock(mSendDataLock);
		SendData data;
		data.buffer = buffer;
		data.bufferSize = bufferSize;
		mSendDataList.emplace_back(data);
	}

	void TcpClient::receiveThread_(void*)
	{
		while (true)
		{
			// 停止チェック
			if (mIsConnectionClosed)
			{
				break;
			}

			// データ受信
			mem::SafeArray<uint8_t> receivedData;
			{
				UniqueLock lock(mSocketLock);
				receivedData = socketUtil::receive(mpSocket);
			}

			if (receivedData.size() > 0)
			{
				// 受信コールバック
				if (mConfig.onReceiveData != nullptr)
				{
					mConfig.onReceiveData(receivedData.get(), receivedData.size());
				}
			}
			else if (receivedData.size() == 0)
			{
				// 何もしない
			}
			else
			{
				// エラー
				logError("receiving from server failed");
				if (mConfig.onConnectionError != nullptr)
				{
					if (!mConfig.onConnectionError(nullptr, 0))
					{
						break;
					}
				}
			}

			Sleep(cThreadPollingInterval);
		}
	}

	void TcpClient::sendThread_(void*)
	{
		while (true)
		{
			// 停止チェック
			if (mIsConnectionClosed)
			{
				break;
			}

			if (mSendDataList.size() > 0)
			{
				// キューが空になるまで送信
				std::vector<SendData> dataList;
				{
					UniqueLock lock(mSendDataLock);
					mSendDataList.swap(dataList);
				}

				for (auto data : dataList)
				{
					int sendBytes;
					{
						UniqueLock lock(mSocketLock);
						sendBytes = mpSocket->send(data.buffer, data.bufferSize);
					}

					if (sendBytes == data.bufferSize)
					{
						// 送信コールバック
						if (mConfig.onSendData != nullptr)
						{
							mConfig.onSendData(data.buffer, data.bufferSize);
						}
					}
					else
					{
						// エラー
						logError("sending to server failed");
						if (mConfig.onConnectionError != nullptr)
						{
							if (!mConfig.onConnectionError(data.buffer, data.bufferSize))
							{
								break;
							}
						}
					}
				}
			}

			Sleep(cThreadPollingInterval);
		}
	}

}// namespace rte

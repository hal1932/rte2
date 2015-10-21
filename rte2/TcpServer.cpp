#include "TcpServer.h"
#include "Socket.h"
#include "socketUtil.h"
#include <algorithm>

namespace {

	int socketToId(rte::Socket* pSocket)
	{
		return reinterpret_cast<int>(pSocket);
	}

	rte::Socket* idToSocket(int id)
	{
		return reinterpret_cast<rte::Socket*>(id);
	}

	static const int cThreadPollingInterval = 10;
}

namespace rte {

	TcpServer::TcpServer()
		: mpSocket(nullptr)
	{ }

	TcpServer::~TcpServer()
	{
		assert(!mAcceptThread.isAlive());
		assert(mClientDic.size() == 0);
		assert(mpSocket == nullptr);
	}

	bool TcpServer::configure(const Config& config)
	{
		assert(mpSocket == nullptr);

		mConfig = config;

		mpSocket = new Socket();
		if (!mpSocket->configure(Socket::ProtocolType::Tcp))
		{
			mem::safeDelete(&mpSocket);
			return false;
		}

		mpSocket->setBlocking(true);

		return true;
	}

	bool TcpServer::open(int port)
	{
		assert(mpSocket != nullptr);

		if (!mpSocket->bind(port))
		{
			return false;
		}
		if (!mpSocket->listen())
		{
			return false;
		}

		mIsConnectionClosed = false;
		mAcceptThread.start(std::bind(&TcpServer::acceptThread_, this, std::placeholders::_1));
		mSendThread.start(std::bind(&TcpServer::sendThread_, this, std::placeholders::_1));

		return true;
	}

	void TcpServer::close()
	{
		assert(mpSocket != nullptr);

		// 全部まとめて止める
		mIsConnectionClosed = true;

		mAcceptThread.join();

		for (std::pair<Socket*, ClientInfo> item : mClientDic)
		{
			item.second.pReceiveThread->join();
			mem::safeDelete(&item.second.pReceiveThread);
			mem::safeDelete(&item.second.mpLock);

			mem::safeDelete(&item.first);
		}
		mClientDic.clear();

		mpSocket->close();
		mem::safeDelete(&mpSocket);
	}

	void TcpServer::sendAsync(int id, const uint8_t* buffer, int bufferSize)
	{
		assert(mpSocket != nullptr);

		UniqueLock lock(mSendDataLock);

		SendData data;
		data.pClientSocket = idToSocket(id);
		data.buffer = buffer;
		data.bufferSize = bufferSize;
		mSendDataList.emplace_back(data);
	}

	void TcpServer::broadcastAsync(const uint8_t* buffer, int bufferSize)
	{
		assert(mpSocket != nullptr);

		for (auto client : mClientDic)
		{
			auto id = socketToId(client.first);
			sendAsync(id, buffer, bufferSize);
		}
	}

	void TcpServer::closeConnection(int id)
	{
		assert(mpSocket != nullptr);

		auto ptr = idToSocket(id);
		auto found = mClientDic.find(ptr);
		if (found == mClientDic.end())
		{
			return;
		}
		auto clientInfo = *found;
		mClientDic.erase(found);

		auto pClientSocket = clientInfo.first;
		auto pReceiveThread = clientInfo.second.pReceiveThread;

		// 指定クライアントのスレッドに停止リクエストを発行
		{
			UniqueLock lock(mCloseRequestLock);
			mCloseRequestList.push_back(socketToId(pClientSocket));
		}

		pReceiveThread->join();
		mem::safeDelete(&pReceiveThread);
		mem::safeDelete(&clientInfo.second.mpLock);

		pClientSocket->close();
		mem::safeDelete(&pClientSocket);
	}

	void TcpServer::acceptThread_(void*)
	{
		auto pClient = new Socket();

		while (true)
		{
			if (mIsConnectionClosed)
			{
				break;
			}

			auto result = mpSocket->accept(pClient);
			if (result == TriBool::True)
			{
				// クライアント受付コールバック
				if (mConfig.onAcceptClient != nullptr)
				{
					auto clientId = socketToId(pClient);
					mConfig.onAcceptClient(clientId);
				}

				// データ受信スレッド起動
				auto pReceiveThread = new Thread();
				pReceiveThread->start(std::bind(&TcpServer::receiveThread_, this, std::placeholders::_1), pClient);

				ClientInfo info;
				info.pReceiveThread = pReceiveThread;
				info.mpLock = new Mutex();
				mClientDic[pClient] = info;

				pClient = new Socket();
			}

			Sleep(cThreadPollingInterval);
		}

		mem::safeDelete(&pClient);
	}

	void TcpServer::receiveThread_(void* arg)
	{
		auto pClientSocket = static_cast<Socket*>(arg);
		auto clientId = socketToId(pClientSocket);
		Mutex& clientLock = *mClientDic[pClientSocket].mpLock;

		const int bufferSize = 1024;
		mem::SafeArray<uint8_t> buffer(bufferSize);

		while (true)
		{
			// 停止リクエストをチェック
			{
				UniqueLock lock(mCloseRequestLock);

				auto closeRequest = std::find(mCloseRequestList.begin(), mCloseRequestList.end(), clientId);
				if (mIsConnectionClosed || closeRequest != mCloseRequestList.end())
				{
					mCloseRequestList.erase(closeRequest);
					break;
				}
			}

			// データ受信
			mem::SafeArray<uint8_t> receivedData;
			{
				UniqueLock lock(clientLock);
				receivedData = socketUtil::receive(pClientSocket);
			}

			if (receivedData.size() > 0)
			{
				// 受信コールバック
				if (mConfig.onReceiveData != nullptr)
				{
					mConfig.onReceiveData(clientId, buffer.get(), buffer.size());
				}
			}
			else if (receivedData.size() == 0)
			{
				// 何もしない
			}
			else
			{
				// エラー
				logError("receiving from client failed");
				if (mConfig.onConnectionError != nullptr)
				{
					if (!mConfig.onConnectionError(clientId, nullptr, 0))
					{
						break;
					}
				}
			}

			Sleep(cThreadPollingInterval);
		}
	}

	void TcpServer::sendThread_(void*)
	{
		while (true)
		{
			// 停止リクエストをチェック
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
					auto pClientSocket = data.pClientSocket;
					Mutex& clientLock = *mClientDic[pClientSocket].mpLock;

					int sendBytes;
					{
						UniqueLock lock(clientLock);
						sendBytes = pClientSocket->send(data.buffer, data.bufferSize);
					}

					auto clientId = socketToId(pClientSocket);
					if (sendBytes == data.bufferSize)
					{
						// 送信コールバック
						if (mConfig.onSendData != nullptr)
						{
							mConfig.onSendData(clientId, data.buffer, data.bufferSize, sendBytes);
						}
					}
					else
					{
						// エラー
						logError("sending to client failed");
						if (mConfig.onConnectionError != nullptr)
						{
							if (!mConfig.onConnectionError(clientId, data.buffer, data.bufferSize))
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
